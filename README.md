# 🔄 ESP32 OTA Update System

A wireless firmware update system for the **ESP32 DevKit** built with **ESP-IDF**. This project (`esp32_ota`) enables Over-The-Air firmware updates over WiFi — no USB cable needed after the initial flash.

---

## 📋 Table of Contents

- [About](#about)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [How OTA Works](#how-ota-works)
- [Partition Table](#partition-table)
- [Configuration](#configuration)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)

---

## 📖 About

This project implements **Over-The-Air (OTA)** firmware updates on the ESP32 DevKit using the **ESP-IDF native OTA API**. The device connects to a WiFi network and downloads new firmware from an HTTP server, flashing it to the inactive OTA partition safely. If the update fails, the device automatically rolls back to the previous working firmware.

---

## ✨ Features

- ✅ **WiFi Connectivity** — Connects to a local 2.4GHz WiFi network
- ✅ **HTTP OTA Updates** — Downloads firmware from an HTTP server wirelessly
- ✅ **Dual Partition Support** — Safe OTA with automatic rollback on failure
- ✅ **Custom Partition Table** — Optimized layout for 4MB flash
- ✅ **ESP-IDF Native OTA** — Uses Espressif's official `esp_https_ota` component
- ✅ **Serial Monitoring** — Real-time logs via UART for debugging

---

## 🛠️ Hardware Requirements

| Component | Details |
|-----------|---------|
| Board | ESP32 DevKit V1 |
| Flash Size | 4MB (required) |
| WiFi | 2.4GHz network |
| USB Cable | For initial flash only |

---

## 💻 Software Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) v5.x or later
- CMake 3.16 or later
- Python 3.x
- An HTTP server to host firmware binaries

---

## 📁 Project Structure

```
ota-update-system/
├── main/                    # Main application source code
├── build/                   # Auto-generated build output (do not edit)
├── CMakeLists.txt           # Top-level CMake build config (project: esp32_ota)
├── partitions.csv           # Custom partition table for 4MB flash
├── sdkconfig                # Full ESP-IDF project configuration
├── sdkconfig.defaults       # Default SDK settings (OTA + flash size)
└── README.md
```

---

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/Badhusha8/ota-update-system.git
cd ota-update-system
```

### 2. Set Up ESP-IDF Environment

```bash
. $HOME/esp/esp-idf/export.sh
```

### 3. Configure WiFi and OTA Server

Run menuconfig to set your WiFi credentials and OTA server URL:

```bash
idf.py menuconfig
```

Navigate to **Example Configuration** and set:
- WiFi SSID
- WiFi Password
- OTA firmware URL (e.g., `http://192.168.1.100:8070/esp32_ota.bin`)

### 4. Build the Project

```bash
idf.py build
```

### 5. Initial Flash via USB

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

After this first flash, all future updates can be done wirelessly via OTA.

---

## ⚙️ How OTA Works

```
ESP32 Boots
     │
     ▼
Connect to WiFi
     │
     ▼
Send HTTP GET request to OTA server
     │
     ▼
Download firmware .bin file
     │
     ▼
Write to inactive OTA partition (ota_0 or ota_1)
     │
     ▼
Verify integrity → Set boot partition → Reboot
     │
     ▼
✅ Running new firmware
     │
     ▼ (if boot fails)
🔄 Rollback to previous firmware
```

1. ESP32 boots and connects to WiFi
2. It sends an HTTP request to your firmware server
3. The firmware binary is downloaded and written to the **inactive** OTA partition
4. After verification, ESP32 reboots into the new firmware
5. If the new firmware crashes on boot, it rolls back automatically

---

## 🗂️ Partition Table

The project uses a custom `partitions.csv` optimized for 4MB flash with dual OTA support:

| Partition | Type | Purpose |
|-----------|------|---------|
| nvs | Data | Non-volatile storage (WiFi credentials, etc.) |
| otadata | Data | Tracks which OTA partition to boot from |
| ota_0 | App | First OTA slot |
| ota_1 | App | Second OTA slot (for updates) |

The `otadata` partition keeps track of which slot has the latest valid firmware, enabling safe rollback.

---

## ⚙️ Configuration

Key settings from `sdkconfig.defaults`:

| Config Key | Value | Description |
|------------|-------|-------------|
| `CONFIG_PARTITION_TABLE_CUSTOM` | y | Use custom partitions.csv |
| `CONFIG_ESPTOOLPY_FLASHSIZE_4MB` | y | Target 4MB flash chip |
| `CONFIG_ESP_HTTPS_OTA_ALLOW_HTTP` | y | Allow plain HTTP OTA (no TLS required) |

> ⚠️ **Note:** `ALLOW_HTTP` is enabled for development convenience. For production, use HTTPS with a valid certificate.

---

## 📡 Usage

### Host the Firmware Binary

After building, serve the firmware from your computer:

```bash
cd build
python3 -m http.server 8070
```

Make sure your ESP32 and computer are on the **same WiFi network**.

### Monitor Serial Output

```bash
idf.py -p /dev/ttyUSB0 monitor
```

You should see logs like:
```
I (1234) OTA: Connecting to WiFi...
I (2345) OTA: Connected! Starting OTA update...
I (5678) OTA: OTA successful, rebooting...
```

---

## 🔧 Troubleshooting

| Problem | Solution |
|---------|----------|
| WiFi not connecting | Double-check SSID and password in menuconfig |
| OTA download fails | Verify server IP, port, and that both devices are on same network |
| Flash size mismatch | Ensure you're using a 4MB flash ESP32 |
| Port not found | Try `/dev/ttyUSB1` or check `ls /dev/tty*` |
| Rollback on every boot | Check new firmware boots correctly before marking valid |

---

## 👤 Author

**Ibrahim Badhusha M H**
- GitHub: [@Badhusha8](https://github.com/Badhusha8)

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).
