#pragma once

#include <Arduino.h>
#include <driver/gpio.h>

namespace config {

constexpr char kPortalApName[] = "PlaneRadar-Setup";
constexpr char kPortalIp[] = "192.168.4.1";
constexpr char kPortalHostname[] = "plane-radar";
constexpr char kPortalHostUrl[] = "plane-radar.local";

constexpr unsigned long kWifiConnectAttemptMs = 15000;
constexpr uint8_t kWifiConnectAttempts = 3;
constexpr unsigned long kWifiPortalTimeoutSec = 0;
constexpr unsigned long kWifiConnectingFrameMs = 50;
constexpr unsigned long kWifiDownGraceMs = 4000;
constexpr unsigned long kWifiReconnectIntervalMs = 15000;

// BOOT button on GPIO9.
constexpr gpio_num_t kBootPin = GPIO_NUM_9;
constexpr unsigned long kBootResetHoldMs = 3000UL;
constexpr unsigned long kBootTapMinMs = 40UL;

// External button for changing the radar range.
constexpr gpio_num_t kBtnRangePin = GPIO_NUM_2;
constexpr unsigned long kBtnDebounceMs = 50UL;

// External button for selecting the next aircraft.
constexpr gpio_num_t kBtnSelectPin = GPIO_NUM_4;

// External button for increasing the backlight brightness.
constexpr gpio_num_t kBtnBrightUpPin = GPIO_NUM_0;

// External button for decreasing the backlight brightness.
constexpr gpio_num_t kBtnBrightDownPin = GPIO_NUM_1;

// External button for rotating the display.
constexpr gpio_num_t kBtnRotatePin = GPIO_NUM_5;

// Five backlight brightness levels, from lowest to highest.
constexpr uint8_t kBrightnessLevels[] = {16, 30, 90, 150, 200};
constexpr int kBrightnessSteps = 5;

// Information panel layout.
constexpr int kInfoPanelY = 172;
constexpr int kInfoPanelHeight = 148;
constexpr int kInfoPanelPad = 6;

// Waveshare ESP32-C6-LCD-1.47 display pinout.
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_21;
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_14;
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_15;
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_6;
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_7;
constexpr gpio_num_t kDisplayPinBl = GPIO_NUM_22;

constexpr int kDisplayWidth = 172;
constexpr int kDisplayHeight = 320;
constexpr uint32_t kDisplaySpiWriteHz = 40000000;
constexpr bool kDisplayInvert = true;
constexpr bool kDisplayRgbOrder = false;

// Default radar position: Berlin, near BER airport.
constexpr double kDefaultRadarLat = 52.3676;
constexpr double kDefaultRadarLon = 13.5033;

constexpr unsigned long kAdsbFetchIntervalMs = 3000;
constexpr float kAdsbFetchRadiusScale = 1.0f;
constexpr bool kAdsbShowGroundAircraft = false;

constexpr uint16_t kColorBlack = 0x0000;
constexpr uint16_t kColorYellow = 0xFFE0;
constexpr uint16_t kTextOnYellow = kColorBlack;
constexpr uint16_t kTextOnBlack = 0xFFFF;

}  // namespace config
