#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "lwip/sockets.h"

#define TAG "TIME_PUBLISHER"

// --- Wi-Fi Credentials ---
#define WIFI_SSID "NissiRnD" 
#define WIFI_PASS "Nissi@A006"

// --- LAPTOP NTP SERVER & CLOUD CONFIGURATION ---
// Laptop IP found via ipconfig (192.168.0.21)
#define LAPTOP_NTP_SERVER_IP    "192.168.0.21" 
#define NTP_PORT                123

// Secure Cloud Endpoint (HTTPS)
#define CLOUD_ENDPOINT_URL "https://nissiconnect.com/project/timesync" 

#define NTP_TIMESTAMP_DELTA 2208988800ULL 

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;


// ----------------------------------------------------------------------
// Time Fetching (NTP Client Logic)
// ----------------------------------------------------------------------

static time_t fetch_laptop_time(void)
{
    int sock = -1;
    struct sockaddr_in serv_addr;
    uint8_t tx_buf[48] = {0b11100011}; 
    uint8_t rx_buf[48];
    time_t unix_time = 0;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket.");
        return 0;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(NTP_PORT);
    if (inet_pton(AF_INET, LAPTOP_NTP_SERVER_IP, &serv_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid LAPTOP_NTP_SERVER_IP");
        goto cleanup;
    }

    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sendto(sock, tx_buf, sizeof(tx_buf), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    int len = recvfrom(sock, rx_buf, sizeof(rx_buf), 0, NULL, NULL);

    if (len < 48) {
        ESP_LOGE(TAG, "Error: Did not receive full NTP response (len=%d)", len);
        goto cleanup;
    }

    uint32_t ntp_time_net;
    memcpy(&ntp_time_net, &rx_buf[40], 4);
    uint32_t ntp_time = ntohl(ntp_time_net);

    unix_time = (time_t)(ntp_time - NTP_TIMESTAMP_DELTA);

    struct tm timeinfo;
    gmtime_r(&unix_time, &timeinfo);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S UTC", &timeinfo);
    ESP_LOGI(TAG, "Fetched time from Laptop: %s", time_str);

cleanup:
    if (sock != -1) {
        close(sock);
    }
    return unix_time;
}


// ----------------------------------------------------------------------
// Cloud Publishing (HTTP Client Logic)
// ----------------------------------------------------------------------

static esp_err_t send_time_to_cloud(time_t unix_time)
{
    // Fix: %lld is used for 64-bit time_t
    ESP_LOGI(TAG, "Publishing time to cloud: %lld", unix_time); 
    
    // Initialize HTTP Client
    esp_http_client_config_t config = {
        .url = CLOUD_ENDPOINT_URL,
        
        // --- FINAL COMPATIBILITY HTTPS CONFIGURATION FIX ---
        // 1. Explicitly set CA certificate to NULL (bypasses verification for testing)
        .cert_pem = NULL, 
        
        // 2. Use the standard skip flag
        .skip_cert_common_name_check = true, 
        // --- END FINAL FIX ---
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // Construct JSON payload: using %lld (Fix for format warning)
    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"device_id\": \"esp32_sntp_client\", \"unix_time\": %lld}",
             unix_time);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Perform the HTTP request
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Success. Status code: %d", status_code);
    } else {
        ESP_LOGE(TAG, "HTTP POST Failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}


// ----------------------------------------------------------------------
// Main Application Task (Fetch and Publish Loop)
// ----------------------------------------------------------------------

static void publisher_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    
    while (1) {
        time_t current_time = fetch_laptop_time();
        
        if (current_time > 0) {
            send_time_to_cloud(current_time);
        } else {
            ESP_LOGW(TAG, "Failed to fetch time from Laptop. Skipping publish attempt.");
        }

        // Wait 30 seconds before fetching and publishing again
        vTaskDelay(pdMS_TO_TICKS(30000)); 
    }
}


// ----------------------------------------------------------------------
// Wi-Fi and Initialization
// ----------------------------------------------------------------------

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "Wi-Fi disconnected. Retrying connection...");
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi connected. IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_event_group = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for Wi-Fi connection to %s...", WIFI_SSID);

    xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT,
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);

    xTaskCreate(publisher_task, "publisher_task", 8192, NULL, 5, NULL);
}