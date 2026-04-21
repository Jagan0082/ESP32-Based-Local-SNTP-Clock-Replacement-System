# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SNDP_server01)
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS ".")

idf_component_get_property(target_name COMPONENT_NAME)
target_link_libraries(${target_name} 
    "log"
    "nvs_flash"
    "esp_event"
    "esp_netif"
    "esp_wifi"
    "freertos"
    "lwip"  
    "esp_http_client"
    "c"
)