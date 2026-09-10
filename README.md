# ESP32 Plane Radar — Waveshare ESP32-C6 LCD 1.47" Edition

A display adaptation and feature-enhanced fork of [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar).

This version keeps the original idea — a compact ESP32-based ADS-B radar displaying nearby aircraft — and adapts it for the **Waveshare ESP32-C6-LCD-1.47**, featuring a 1.47-inch rectangular ST7789 display with 172 × 320 pixels. It also adds a selectable aircraft information panel and several hardware controls for brightness, display rotation, range selection, and aircraft selection.

> This repository is an independent fork. The original project and its authorship remain credited to [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar).

## What it does

The firmware connects to Wi-Fi, retrieves live ADS-B aircraft data from adsb.fi, and displays a radar view centered on the configured location (standard is berlin BER!). An information panel provides detailed data for a selected aircraft.

## Changes from the original

### Hardware and display

- Support for the **Waveshare ESP32-C6-LCD-1.47** board.
- Support for the rectangular **ST7789 display** with a resolution of 172 × 320 pixels.
- Board-specific SPI, reset, data/command, chip-select, and backlight pin assignments.
- Display inversion and RGB-order settings adapted for the Waveshare hardware.
- PWM-controlled display backlight.
- Runtime display rotation in 90-degree steps.
- Layout code that adapts the information panel to portrait and landscape orientation.

### Additional controls

The original BOOT button remains available for next plane, range selection, display rotation and the long-press reset function. Additional buttons can be connected between the configured GPIO and GND:

| Function | GPIO | Behaviour |
|---|---:|---|
| Range | GPIO 2 | Select the next radar range preset |
| Aircraft selection | GPIO 4 | Select the next aircraft |
| Brightness up | GPIO 0 | Increase backlight brightness |
| Brightness down | GPIO 1 | Decrease backlight brightness |
| Rotation | GPIO 5 | Rotate the display by 90 degrees |

All additional buttons use internal pull-ups and software debouncing (meaning you can simply add any buttons without extra hardware, it will work perfectly).

### Brightness control

The display backlight can be adjusted through five brightness levels, if you use the pinout and solder on a button for that. If not you cant change it. The default startup level is 16% percent.

The current implementation uses the following 0-255 PWM values:

- 16
- 30
- 90
- 150
- 200

There is no 255 brightness as that overheats and can damage/kill the panel.

### Aircraft information panel

The original radar view is extended with an information panel for the currently selected aircraft. Pressing the aircraft-selection button (or the boot button once) cycles through all aircraft currently received by the ADS-B service.

The panel displays:

- Callsign
- Aircraft type
- Registration
- Altitude
- Ground speed in knots
- Heading
- Aircraft category
- Current selection and total aircraft count

If no aircraft are available, the panel displays `No aircraft in range`.

### Aircraft category labels

ADS-B category codes are translated into readable labels where known. Examples include:

- `A1` — Light
- `A2` — Small
- `A3` — Large
- `A5` — Heavy
- `A6` — High performance
- `A7` — Rotorcraft
- `B1` — Glider
- `B2` — Airship
- `B3` — Parachute
- `B6` — UAV
- `B7` — Space

Unknown or unsupported category codes are displayed as received.

### Update interval and default location

- ADS-B updates are requested every 3 seconds instead of the original approximately 5-second interval.
- The default radar location is set to Berlin, Germany, near BER Airport (you can always change that through reseting it with holding the boot-button for longer than sec):
  - Latitude: `52.3676`
  - Longitude: `13.5033`
- The ADS-B fetch-radius scale is explicitly configurable through `kAdsbFetchRadiusScale`.

## Display and board configuration

The relevant settings are located in `include/config.h`.

```cpp
constexpr int kDisplayWidth = 172;
constexpr int kDisplayHeight = 320;
constexpr uint32_t kDisplaySpiWriteHz = 40000000;
constexpr bool kDisplayInvert = true;
constexpr bool kDisplayRgbOrder = false;
```

The Waveshare ESP32-C6-LCD-1.47 uses the following display connections:

| Display signal | ESP32-C6 GPIO |
|---|---:|
| LCD_RST | GPIO 21 |
| LCD_CS | GPIO 14 |
| LCD_DC | GPIO 15 |
| SDA / MOSI | GPIO 6 |
| SCL / SCLK | GPIO 7 |
| Backlight | GPIO 22 |

## Building and uploading

The project is intended to be built with PlatformIO. Open the project in PlatformIO, verify the selected environment and board configuration, then build and upload the firmware.

```bash
pio run
pio run -t upload
pio device monitor
```

The serial monitor uses 115200 baud.

## Dependencies

The project is based on the dependencies used by the original project, including:

- LovyanGFX
- WiFiManager
- ArduinoJson

## Project status

This is a hardware-specific alternative version and is primarily intended for the Waveshare ESP32-C6-LCD-1.47. Other displays or ESP32 boards may require changes to the display driver, pin mapping, layout, and PlatformIO configuration.

The firmware is still under active development. Hardware-specific details, UI layout, and controls may change as testing continues.

## Credits

This project is based on the original work by **MatixYo**:

- Original project: [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar)
- Original author: [MatixYo](https://github.com/MatixYo)

Please refer to the original repository for the initial project concept, original radar implementation, and upstream history.

## License

This fork should retain the license of the original project and all required copyright and attribution notices. The upstream repository currently includes an MIT license; verify that your fork retains the original `LICENSE` file unchanged and add your own attribution for modifications where appropriate.
