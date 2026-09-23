![Alt text](https://github.com/richcj10/-ESP32PLC/blob/esp32-s3/Logo/ESPPLC.png "Logo")
# ESP32PLC - Home automation made easy! 
### This is a open source project created in my free time to solve the "How do I connect this to that? problem.
### Hardware:
*ESP32S3 
*TFT / OLED Display
*RGB Status
*Temp / Humididity of device
*Joystick Menu


## Getting Started

### Required repositories
This project depends on a separate repository that PlatformIO does **not** download automatically. You need to clone it yourself before building:

| Repository | Purpose | Expected location |
|---|---|---|
| [ESP32ModbusMaster](https://github.com/richcj10/ESP32ModbusMaster) | Modbus master library used for remote I/O (`src/Remote/`) | `C:\Repo\ESP32ModbusMaster` |

Clone both repos side by side:
```
cd C:\Repo
git clone https://github.com/richcj10/-ESP32PLC.git
git clone https://github.com/richcj10/ESP32ModbusMaster.git
```

If you clone `ESP32ModbusMaster` somewhere else, update these two entries in `ESP32PLC/platformio.ini` to point at your location:
```ini
build_flags =
	-I "c:\Repo\ESP32ModbusMaster\src"
lib_extra_dirs = c:\Repo\ESP32ModbusMaster
```

### Tools you need
- [VS Code](https://code.visualstudio.com/) with the **PlatformIO IDE** extension (the workspace recommends it). Don't install the `pioarduino` extension alongside it, because the two conflict.
- **Git** on your PATH. `scripts/version.py` runs `git describe` at build time to add the firmware and web version to the build. Without Git the version shows as `dev`.
- **Python 3** (PlatformIO installs its own copy). You only need it on your PATH to run `tools/make_config.py`.
- **ccache** (optional) speeds up rebuilds. `scripts/ccache.py` uses it if it's found (`scoop install ccache` or `choco install ccache`), and the build works fine without it.

### Libraries downloaded automatically by PlatformIO
These are listed under `lib_deps` in `platformio.ini`. PlatformIO downloads them on the first build, so you don't need to install anything:

| Library | Version | Used for |
|---|---|---|
| TFT_eSPI (bodmer) | ^2.4.61 | ST7789 TFT display |
| Adafruit SSD1306 / GFX / BusIO | ^2.4.6 / ^1.10.10 / ^1.8.3 | OLED display option |
| PNGdec (bitbank2) | ^1.0.1 | Logo / image drawing |
| QRCode (ricmoo) | ^0.0.1 | On-screen QR codes |
| FastLED | 3.5.0 | RGB status LED / LED strip module |
| ArduinoJson | 6.17 | Config and web API JSON (v6 API, don't upgrade to v7) |
| PubSubClient (knolleary) | ^2.8.0 | MQTT |
| AsyncTCP (husarnet fork, GitHub) | latest | Async networking |
| ESPAsyncWebServer (me-no-dev, GitHub) | latest | Web UI |

The ESP32 Arduino core (`espressif32` platform) and the toolchain are also downloaded automatically. The first build can take several minutes.

### Libraries included in this repo
These are in `ESP32PLC/lib/` and need no setup: `ModBusBLMaster`, `SimpleModbusSlave`, `OneWire-Stickbreaker`, `LEDStripModule`, and `FireworksModule`.

### Build configuration
Everything is set in `ESP32PLC/platformio.ini` (`env:esp32s3`):
- **Board:** ESP32-S3 with **8 MB flash + PSRAM** (`partitions_8mb.csv`, `-DBOARD_HAS_PSRAM`).
- **Display:** `-DDISPLAY_TFT` is the default. Swap it for `-DDISPLAY_OLED` if you have OLED hardware. The TFT pins and driver are set with `-D` flags, so you don't need to edit TFT_eSPI's `User_Setup.h`.
- **Modules:** `-DMOD_LEDSTRIP` and `-DMOD_FIREWORKS` are optional. Comment them out to leave those modules out of the build (see `MODULE_ARCHITECTURE.md`).

### Building and flashing
1. Open the `ESP32PLC` folder in VS Code and build the `esp32s3` environment.
2. **First flash over USB:** `platformio.ini` defaults to OTA upload (`upload_protocol = espota`) with a fixed `upload_port` IP. For the first flash, comment out those two lines and set `upload_port` to your board's COM port. After that you can switch back to OTA with the device's IP address or hostname.
3. **Upload the filesystem:** the web UI and config files in `ESP32PLC/data/` go on a separate LittleFS partition. Run **PlatformIO → Upload Filesystem Image** once, and again whenever you change `data/`.
4. **Optional:** generate WiFi/MQTT config files into `data/` before uploading the filesystem:
   ```
   python tools/make_config.py all
   ```
