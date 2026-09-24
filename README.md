# Handheld OpenSpool RFID Tag Reader & Writer (CYD ESP32-2432S028)

A dedicated handheld device for reading, writing, customizing, and cloning **OpenSpool 1.0** RFID/NFC tags for 3D printing filaments using the **Cheap Yellow Display (CYD)** and a **PN532 NFC Module**.

---

## Features

- 📱 **Portrait Ergonomic UI (240 x 320)** designed for one-handed scanner operation.
- 🎨 **Live Filament Color Swatch**: Instant preview in actual hex color with brand badge, material name, and nozzle/bed temperature ranges.
- ✍️ **Tag Writer & Customizer**:
  - Material selector (PLA, PLA-Silk, PLA-CF, PETG, ABS, ASA, TPU-95A, PC, PA-CF, PVA).
  - Brand selector (Bambu Lab, Polymaker, eSun, Sunlu, Overture, Elegoo, Prusament, Generic).
  - 16-color quick-touch palette.
  - Temperature steppers (+/- 5°C).
- ⚡ **Batch Tag Cloner**: Read any spool once and write to multiple blank NTAG215 stickers with auto-counter.
- 🔊 **Audio & Visual Feedback**: RGB status LED and piezo buzzer melodies for scan, write success, error, and touch clicks.

---

## Hardware Wiring

### 1. PN532 DIP Switch Configuration (HSU / UART Mode)
Set the onboard mode switches on the PN532 module:
- **Switch 1 (SEL0):** `OFF` (or `0`)
- **Switch 2 (SEL1):** `OFF` (or `0`)

### 2. Wiring Connections

| CYD (ESP32-2432S028) | PN532 Module (HSU Mode) | Note |
| :--- | :--- | :--- |
| **VCC** (+3.3V or +5V) | **VCC** | Connect to CN1 or 3V3/5V pad on CYD |
| **GND** (Ground) | **GND** | Connect to CN1 or GND pad |
| **GPIO 22** (TX2) | **RXD** | CYD UART2 Transmit to PN532 Receive |
| **GPIO 35** (RX2) | **TXD** | CYD UART2 Receive (Input-only pin on ESP32) |

> **Why this wiring?**
> Connecting the PN532 via UART on `GPIO 22` and `GPIO 35` ensures completely independent communication with **zero SPI bus conflicts** with the display or touch controller, and avoids I2C clock stretching timing issues.

---

## Installation & Flashing

### Using PlatformIO (VS Code / CLI)

1. Clone or copy this folder:
   ```bash
   cd openspool-cyd-reader
   ```
2. Connect your CYD board via USB-C / micro-USB.
3. Build and Upload:
   ```bash
   pio run --target upload
   ```
4. Open the Serial Monitor (115200 baud):
   ```bash
   pio device monitor -b 115200
   ```

---

## OpenSpool Data Format

Tags written by this device follow the standard [OpenSpool 1.0 specification](https://github.com/spuder/OpenSpool) stored on **NTAG215** or **NTAG216** tags as an NDEF `application/json` record:

```json
{
  "protocol": "openspool",
  "version": "1.0",
  "brand": "Bambu Lab",
  "type": "PLA-Basic",
  "color_hex": "00AE42",
  "min_temp": "210",
  "max_temp": "230",
  "bed_min_temp": "45",
  "bed_max_temp": "60",
  "weight": "1000",
  "spool_id": ""
}
```

---

## Assembly 

Complete handheld Bill of Materials, with Amazon links:
1. ["CYD" ESP32-2432S028](https://amzn.to/4hlUx3n)
2. [PN532 NFC Module](https://amzn.to/4xKQxzx)
3. [Power switch](https://amzn.to/4xJvs8F)
4. [Li-ion Battery](https://amzn.to/4d0tskO)
5. [Battery controller and DC-DC step-up boost module](https://amzn.to/4jj8O3c)
6. [Speaker/Buzzer](https://amzn.to/46IcT9P)
7. [M3x3mm Heat set inserts](https://amzn.to/46IdAzX) 
8. [M3x12mm screws](https://amzn.to/4dwQdgo) 

## 📜 License & Credits

- **Author**: Jason Burrell ([jaybeeunix.github.io](https://jaybeeunix.github.io))
- **AI Development Assistant**: [Antigravity](https://antigravity.google/)

Distributed under the GNU General Public License v3.0 (GPL-3.0). See [LICENSE](LICENSE) for full details.