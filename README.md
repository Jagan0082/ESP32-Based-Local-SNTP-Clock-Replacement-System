# ⏱ ESP32-Based Local SNTP Clock Replacement System

## 📌 Overview

Designed and developed a real-time clock synchronization system using ESP32 to replace conventional SNTP clocks in substation environments.

The system distributes precise timing signals to relays and control systems with sub-millisecond accuracy, ensuring reliable and deterministic operation in industrial applications.

To replace expensive standard SNTP clock, this is a cost-efficient solution to make it automated, connected to the web and servers to receive alerts remotely.

---

## 🚀 Key Features

* ⏱ Sub-millisecond time synchronization accuracy
* 📡 UDP-based time distribution (low latency)
* ⚡ ISR-driven signal generation for precise timing
* 🔁 FreeRTOS-based task scheduling
* 🔌 GPIO + Timer-based output for relay synchronization
* 📶 Local network-based clock system (no internet dependency)

---

## 🧠 Core Embedded Concepts

* Real-time scheduling using FreeRTOS
* Interrupt-driven architecture (ISR)
* Hardware timer configuration for precision timing
* UDP socket programming (ESP-IDF / LwIP)
* Deterministic signal generation using GPIO

---

## ⚙️ System Architecture

### 🔹 Components

* ESP32 (Master Time Controller)
* Relay / Control Systems (Slaves)
* Local Network (UDP Communication)

### 🔹 Communication Flow

1. Master ESP32 maintains accurate system time
2. Time packets sent over UDP to network nodes
3. ISR + Timer generates synchronized output signals
4. Relays receive precise timing triggers

---

## 🔌 Hardware Details

* ESP32 (ESP-IDF)
* GPIO for relay signal output
* Hardware Timers for precision
* Optional external RTC for backup

---

## 📊 Performance

* ⏱ Synchronization Accuracy: < 1 ms
* ⚡ Low latency communication using UDP
* 🔄 Stable operation under continuous load
* 🧪 Tested with relay-trigger simulation

---

## 🛠 Implementation Details

* UDP socket communication using LwIP
* Timer interrupts for periodic signal generation
* FreeRTOS tasks:

  * Time sync task
  * UDP transmit task
  * Signal generation task

---

## 📄 Use Case

* Substation relay synchronization
* Industrial automation timing systems
* Distributed embedded control systems

---

## 🔮 Future Improvements

* PTP (Precision Time Protocol) implementation
* Hardware RTC + GPS synchronization
* CAN/Ethernet-based industrial integration
* Fault-tolerant distributed clock system

---

## 👨‍💻 Author

**Jagan K**
Embedded Systems & Firmware Engineer

📧 [jaganjck28@gmail.com](mailto:jaganjck28@gmail.com)
🔗 https://linkedin.com/in/jagan-k-668328367

---

## ⚠️ Note

This project is designed for industrial-grade timing applications where deterministic behavior and precise synchronization are critical.
