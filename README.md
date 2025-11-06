https://thingspeak.mathworks.com/channels/3139856
# Elderly-Health-Wearable
Group Members: 
CHIRAG GUNTI 22BCT0133 --> Hardware and Integration  
SUYASH PANDEY 22BCT0111 --> Set-up and Interfacing
SAKETH S 22BCT0266 --> Software and Dev

---

# Smart Wearable Elderly Care & Health Monitoring System  

A compact IoT-based wearable that continuously monitors **heart rate**, **SpO₂**, and **motion (fall detection)**.  
Built using the **ESP32-WROOM-32 N8R8**, integrating sensors, cloud support, and intelligent alerts for elderly safety.

---

## Overview
This wearable monitors vital signs and movement to detect abnormal heart activity, low oxygen levels, or sudden motion (falls).  
Data is sent wirelessly for cloud logging and remote caregiver alerts.

---

## Hardware Components

| Component | Description | Purpose |
|------------|-------------|----------|
| **ESP32-WROOM-32 N8R8** | Dual-core Wi-Fi + Bluetooth MCU | Main controller |
| **MAX30102** | Heart-rate & SpO₂ optical sensor | Measures pulse & oxygen levels |
| **MPU6050** | 3-axis accelerometer + gyroscope | Fall & motion detection |
| **TP4056** | Li-ion charger module | Safely charges battery |
| **Li-ion Pouch Battery (50 mAh)** | Power source | Lightweight supply |
| **Slide Switch (1P2T)** | Power control | Turns device ON/OFF |
| **CP2102 USB-UART** | Programming interface | Flash firmware |
| **Capacitors (0.1 µF, 10 µF, 100 µF)** | Power smoothing | Reduce voltage noise |
| **28 AWG Silicone Wire** | Flexible wiring | Wearable interconnects |

---

## Software Requirements

| Software | Purpose |
|-----------|----------|
| **Arduino IDE (v2.0+)** | Programming environment |
| **ESP32 Board Package (Espressif)** | Board definitions |
| **Libraries** |  |
|  • `Wire.h` – I²C communication |  |
|  • `Adafruit_MPU6050` or `MPU6050` | Motion data |
|  • `SparkFun MAX3010x` or `Adafruit MAX30105` | Heart/SpO₂ readings |
|  • `ArduinoJson` | Data formatting |
|  • `WiFi.h`, `HTTPClient.h` | Cloud connectivity |

---

## Wiring Summary

| ESP32 Pin | Connect To | Function |
|------------|-------------|----------|
| 3.3 V | Sensor VCC | Power |
| GND | Sensor GND / TP4056 OUT– | Ground |
| IO6 | SDA (MPU6050 + MAX30102) | I²C Data |
| IO15 | SCL (MPU6050 + MAX30102) | I²C Clock |
| GPIO43 (TX0) | CP2102 RXD | UART TX |
| GPIO44 (RX0) | CP2102 TXD | UART RX |
| EN | CP2102 DTR (via 0.1 µF cap) | Auto reset |
| GPIO0 | CP2102 RTS (via 0.1 µF cap) | Auto bootloader |

---

## Power Chain
- 100 µF between TP4056 OUT+ and OUT–  
- 10 µF between ESP32 3.3 V and GND  
- 0.1 µF near sensor VCC–GND  

---

## Features
- Real-time heart-rate & SpO₂ monitoring  
- Fall detection via MPU6050  
- Wireless Wi-Fi/BLE data transfer  
- Compact, battery-powered wearable  
- Auto-reset firmware upload support  

---

## Getting Started
1. Open **Arduino IDE** → add  
   `https://dl.espressif.com/dl/package_esp32_index.json` to *Additional Boards Manager URLs*.  
2. Install **ESP32 by Espressif Systems**.  
3. Select: **Board → ESP32 Dev Module**.  
4. Connect CP2102 (TX-RX-GND + optional DTR/RTS).  
5. Ensure battery switch ON → Click **Upload**.  

---

## Cloud Integration and Testing (Upcoming)
Cloud data logging and dashboard visualization to be added using MQTT or REST APIs.

---

