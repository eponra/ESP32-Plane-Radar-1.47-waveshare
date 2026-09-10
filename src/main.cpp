#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "services/wifi_setup.h"
#include "ui/info_panel.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/status_screens.h"

namespace {

Preferences g_preferences;
constexpr char kPreferencesNamespace[] = "plane_radar";
constexpr char kRotationKey[] = "rotation";
constexpr char kBrightnessKey[] = "brightness";

bool g_radarVisible = false;
unsigned long g_wifiDisconnectedSince = 0;
unsigned long g_lastReconnect = 0;
unsigned long g_lastAdsbFetch = 0;

int g_brightnessIndex = 0;
uint8_t g_rotation = 0;

struct Button {
  gpio_num_t pin;
  bool lastRaw = true;
  bool state = true;
  bool pressed = false;
  unsigned long lastChange = 0;
};

Button g_rangeButton{config::kBtnRangePin};
Button g_selectButton{config::kBtnSelectPin};
Button g_brightnessUpButton{config::kBtnBrightUpPin};
Button g_brightnessDownButton{config::kBtnBrightDownPin};
Button g_rotateButton{config::kBtnRotatePin};

void initializeButton(Button& button) {
  pinMode(static_cast<int>(button.pin), INPUT_PULLUP);
}

void pollButton(Button& button) {
  const bool rawState = digitalRead(static_cast<int>(button.pin));
  const unsigned long now = millis();

  if (rawState != button.lastRaw) {
    button.lastChange = now;
    button.lastRaw = rawState;
  }

  if ((now - button.lastChange) >= config::kBtnDebounceMs &&
      button.state != button.lastRaw) {
    button.state = button.lastRaw;
    if (!button.state) {
      button.pressed = true;
    }
  }
}

bool consumeButtonPress(Button& button) {
  if (!button.pressed) {
    return false;
  }
  button.pressed = false;
  return true;
}

void applyBrightness() {
  const uint8_t value = config::kBrightnessLevels[g_brightnessIndex];
  ledcWrite(static_cast<int>(config::kDisplayPinBl), value);
  Serial.printf("Brightness: %d%%\n", (value * 100) / 255);
}

void increaseBrightness() {
  if (g_brightnessIndex < config::kBrightnessSteps - 1) {
    ++g_brightnessIndex;
    applyBrightness();
    g_preferences.putUChar(kBrightnessKey, g_brightnessIndex);
  }
}

void decreaseBrightness() {
  if (g_brightnessIndex > 0) {
    --g_brightnessIndex;
    applyBrightness();
    g_preferences.putUChar(kBrightnessKey, g_brightnessIndex);
  }
}

void applyRotation() {
  tft.setRotation(g_rotation);
  Serial.printf("Rotation: %d (x90deg)\n", g_rotation);
}

void rotateDisplay() {
  g_rotation = (g_rotation + 1) % 4;
  applyRotation();
  g_preferences.putUChar(kRotationKey, g_rotation);

  if (g_radarVisible) {
    ui::radarDisplayDraw();
    ui::infoPanelDraw(ui::infoPanelSelectedIndex());
  }
}

void selectNextRange() {
  ui::radar::rangeNext();
  if (g_radarVisible) {
    ui::radarDisplayDraw();
    ui::infoPanelDraw(ui::infoPanelSelectedIndex());
  }
}

void selectNextAircraft() {
  if (services::adsb::aircraftCount() == 0) {
    return;
  }
  ui::infoPanelSelectNext();
  ui::infoPanelDraw(ui::infoPanelSelectedIndex());
}

void showRadarIfConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    g_radarVisible = false;
    return;
  }

  ui::radarDisplayDraw();
  ui::infoPanelDraw(ui::infoPanelSelectedIndex());
  g_radarVisible = true;
}

void handleButtons() {
  bootButtonPollLongPress();
  pollButton(g_rangeButton);
  pollButton(g_selectButton);
  pollButton(g_brightnessUpButton);
  pollButton(g_brightnessDownButton);
  pollButton(g_rotateButton);

  if (bootButtonConsumeTripleClick()) {
    rotateDisplay();
  } else if (bootButtonConsumeDoubleClick()) {
    selectNextRange();
  } else if (bootButtonConsumeTap()) {
    selectNextAircraft();
  }

  if (consumeButtonPress(g_rangeButton)) {
    selectNextRange();
  }
  if (consumeButtonPress(g_selectButton)) {
    selectNextAircraft();
  }
  if (consumeButtonPress(g_brightnessUpButton)) {
    increaseBrightness();
  }
  if (consumeButtonPress(g_brightnessDownButton)) {
    decreaseBrightness();
  }
  if (consumeButtonPress(g_rotateButton)) {
    rotateDisplay();
  }
}

void fetchAndDraw() {
  if (!services::adsb::fetchUpdate(services::location::lat(),
                                   services::location::lon(),
                                   ui::radar::fetchRadiusKm())) {
    handleButtons();
    return;
  }

  ui::radarDisplayRefreshAircraft();
  ui::infoPanelDraw(ui::infoPanelSelectedIndex());
  handleButtons();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Plane Radar - ESP32-C6 Waveshare 1.47\"");

  g_preferences.begin(kPreferencesNamespace, false);

  g_rotation = g_preferences.getUChar(kRotationKey, 0);
  if (g_rotation > 3) {
    g_rotation = 0;
  }

  g_brightnessIndex = g_preferences.getUChar(kBrightnessKey, 0);
  if (g_brightnessIndex >= config::kBrightnessSteps) {
    g_brightnessIndex = 0;
  }

  bootButtonInit();
  initializeButton(g_rangeButton);
  initializeButton(g_selectButton);
  initializeButton(g_brightnessUpButton);
  initializeButton(g_brightnessDownButton);
  initializeButton(g_rotateButton);

  displayInit();

  ledcAttach(static_cast<int>(config::kDisplayPinBl), 5000, 8);
  applyBrightness();
  applyRotation();

  if (wifiShowsSetupScreenOnBoot()) {
    statusScreenPortal();
  }

  services::location::init();
  ui::radar::rangeInit();
  if (wifiSetupConnect()) {
    showRadarIfConnected();
  }
}

void loop() {
  handleButtons();

  if (WiFi.status() != WL_CONNECTED) {
    if (g_radarVisible) {
      g_radarVisible = false;
    }

    if (g_wifiDisconnectedSince == 0) {
      g_wifiDisconnectedSince = millis();
    }

    if (millis() - g_wifiDisconnectedSince >= config::kWifiDownGraceMs &&
        millis() - g_lastReconnect >= config::kWifiReconnectIntervalMs) {
      g_lastReconnect = millis();
      if (wifiReconnect()) {
        g_wifiDisconnectedSince = 0;
        showRadarIfConnected();
      }
    }
  } else {
    g_wifiDisconnectedSince = 0;

    if (!g_radarVisible) {
      showRadarIfConnected();
    } else if (millis() - g_lastAdsbFetch >= config::kAdsbFetchIntervalMs) {
      g_lastAdsbFetch = millis();
      fetchAndDraw();
    }
  }

  delay(10);
}
