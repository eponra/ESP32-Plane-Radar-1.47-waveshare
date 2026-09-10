#include "ui/info_panel.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstring>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"

namespace fonts = lgfx::v1::fonts;

namespace ui {
namespace {

int s_selected = 0;

constexpr uint16_t kColBg = 0x0000;
constexpr uint16_t kColDivider = 0x2945;
constexpr uint16_t kColLabel = 0x4A69;
constexpr uint16_t kColValue = 0xFFFF;
constexpr uint16_t kColCallsign = 0xFFE0;
constexpr uint16_t kColType = 0x5DFF;
constexpr uint16_t kColAlt = 0x4FFF;
constexpr uint16_t kColHeading = 0x07E0;
constexpr uint16_t kColRegistration = 0xFD20;
constexpr uint16_t kColCategory = 0xF81F;
constexpr uint16_t kColGreen = 0x07E0;
constexpr uint16_t kColRed = 0xF800;

static inline bool isLandscape() {
  return tft.width() > tft.height();
}

static inline int panelX() {
  return isLandscape() ? config::kDisplayHeight - config::kInfoPanelHeight : 0;
}

static inline int panelY() {
  return isLandscape() ? 0 : config::kInfoPanelY;
}

static inline int panelWidth() {
  return isLandscape() ? config::kInfoPanelHeight : config::kDisplayWidth;
}

static inline int panelHeight() {
  return isLandscape() ? config::kDisplayWidth : config::kInfoPanelHeight;
}

static inline int panelPadding() {
  return config::kInfoPanelPad;
}

void setSmallFont() {
  tft.setFont(&fonts::FreeSansBold9pt7b);
  tft.setTextSize(1);
}

void setTinyFont() {
  tft.setFont(&fonts::FreeSans9pt7b);
  tft.setTextSize(1);
}

void drawRow(int y, const char* label, const char* value,
             uint16_t valueColor = kColValue) {
  setTinyFont();

  tft.setTextColor(kColLabel, kColBg);
  tft.setTextDatum(textdatum_t::top_left);
  tft.drawString(label, panelX() + panelPadding(), y);

  tft.setTextColor(valueColor, kColBg);
  tft.setTextDatum(textdatum_t::top_right);
  tft.drawString(value, panelX() + panelWidth() - panelPadding(), y);
}

void drawDivider(int y) {
  tft.drawFastHLine(panelX() + panelPadding(), y,
                    panelWidth() - panelPadding() * 2, kColDivider);
}

void drawNoAircraft() {
  tft.fillRect(panelX(), panelY(), panelWidth(), panelHeight(), kColBg);
  drawDivider(panelY() + 1);

  setTinyFont();
  tft.setTextColor(kColLabel, kColBg);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.drawString("No aircraft", panelX() + panelWidth() / 2,
                 panelY() + panelHeight() / 2 - 8);
  tft.drawString("in range", panelX() + panelWidth() / 2,
                 panelY() + panelHeight() / 2 + 8);
  tft.setTextDatum(textdatum_t::top_left);
}

void formatSpeed(char* buffer, size_t length, float groundSpeed) {
  if (groundSpeed <= 0.0f) {
    snprintf(buffer, length, "---");
  } else {
    snprintf(buffer, length, "%.0f kt", groundSpeed);
  }
}

void formatHeading(char* buffer, size_t length, float heading) {
  if (heading < 0.0f || heading > 360.0f) {
    snprintf(buffer, length, "---");
  } else {
    snprintf(buffer, length, "%.0f deg", heading);
  }
}

const char* categoryLabel(const char* category) {
  if (!category || category[0] == '\0') {
    return "----";
  }

  if (strcmp(category, "A0") == 0) return "Unknown";
  if (strcmp(category, "A1") == 0) return "Light";
  if (strcmp(category, "A2") == 0) return "Small";
  if (strcmp(category, "A3") == 0) return "Large";
  if (strcmp(category, "A4") == 0) return "HighVort";
  if (strcmp(category, "A5") == 0) return "Heavy";
  if (strcmp(category, "A6") == 0) return "HighPerf";
  if (strcmp(category, "A7") == 0) return "Rotor";
  if (strcmp(category, "B0") == 0) return "Unknown";
  if (strcmp(category, "B1") == 0) return "Glider";
  if (strcmp(category, "B2") == 0) return "Airship";
  if (strcmp(category, "B3") == 0) return "Parachute";
  if (strcmp(category, "B4") == 0) return "UltralightHG";
  if (strcmp(category, "B6") == 0) return "UAV";
  if (strcmp(category, "B7") == 0) return "Space";
  if (strcmp(category, "C1") == 0) return "Emerg.Surf";
  if (strcmp(category, "C2") == 0) return "Serv.Surf";
  if (strcmp(category, "C3") == 0) return "Fixed Obst";

  return category;
}

void drawWifiStatus(int x, int y, int size) {
  if (WiFi.status() != WL_CONNECTED) {
    tft.fillRect(x, y, size, size, kColRed);
    tft.drawLine(x, y, x + size - 1, y + size - 1, kColBg);
    tft.drawLine(x + size - 1, y, x, y + size - 1, kColBg);
    return;
  }

  const int rssi = WiFi.RSSI();
  int bars = 4;
  if (rssi < -80) {
    bars = 1;
  } else if (rssi < -70) {
    bars = 2;
  } else if (rssi < -60) {
    bars = 3;
  }

  constexpr int gap = 2;
  const int barWidth = (size - (bars - 1) * gap) / bars;
  const int step = size / 4;  // height of one "step"

  for (int i = 0; i < bars; ++i) {
    const int barX = x + i * (barWidth + gap);
    const int barY = y + (3 - i) * step;
    const int barH = (i + 1) * step;
    tft.fillRect(barX, barY, barWidth, barH, kColGreen);
  }
}

void drawAdsbStatus(int x, int y, int size) {
  if (services::adsb::aircraftCount() == 0) {
    tft.fillRect(x, y, size, size, kColRed);
    tft.drawLine(x, y, x + size - 1, y + size - 1, kColBg);
    tft.drawLine(x + size - 1, y, x, y + size - 1, kColBg);
    return;
  }

  tft.fillRect(x, y, size, size, kColGreen);
}

void drawStatusRow() {
  constexpr int iconSize = 12;
  constexpr int leftMargin = 6;
  constexpr int rightMargin = 6;
  constexpr int bottomMargin = 6;
  constexpr int textIconGap = 4;

  const int iconY = panelY() + panelHeight() - bottomMargin - iconSize;

  setTinyFont();
  tft.setTextColor(kColLabel, kColBg);
  tft.setTextDatum(textdatum_t::top_left);

  const int adsbIconX = panelX() + leftMargin;
  drawAdsbStatus(adsbIconX, iconY, iconSize);
  tft.drawString("ADS-B", adsbIconX + iconSize + textIconGap, iconY + 1);

  const int wifiIconX = panelX() + panelWidth() - rightMargin - iconSize;
  const int wifiTextX = wifiIconX - textIconGap;
  tft.setTextDatum(textdatum_t::top_right);
  tft.drawString("WIFI", wifiTextX, iconY + 1);
  drawWifiStatus(wifiIconX, iconY, iconSize);

  tft.setTextDatum(textdatum_t::top_left);
}

}  // namespace

void infoPanelClear() {
  tft.fillRect(panelX(), panelY(), panelWidth(), panelHeight(), kColBg);
  drawDivider(panelY() + 1);
}

void infoPanelDraw(int index) {
  const size_t count = services::adsb::aircraftCount();
  if (index < 0 || count == 0) {
    drawNoAircraft();
    return;
  }

  if (static_cast<size_t>(index) >= count) {
    index = 0;
  }
  s_selected = index;

  const services::adsb::Aircraft& aircraft =
      services::adsb::aircraftList()[index];
  tft.fillRect(panelX(), panelY(), panelWidth(), panelHeight(), kColBg);

  int y = panelY() + 2;
  drawDivider(y);
  y += 4;

  setSmallFont();
  const char* callsign = aircraft.callsign[0] != '\0'
                             ? aircraft.callsign
                             : "------";
  tft.setTextColor(kColCallsign, kColBg);
  tft.setTextDatum(textdatum_t::top_center);
  tft.drawString(callsign, panelX() + panelWidth() / 2, y);

  char counter[8];
  snprintf(counter, sizeof(counter), "%d/%d", index + 1,
           static_cast<int>(count));
  setTinyFont();
  tft.setTextColor(kColLabel, kColBg);
  tft.setTextDatum(textdatum_t::top_right);
  tft.drawString(counter, panelX() + panelWidth() - panelPadding(), y + 2);

  y += 20;
  drawDivider(y);
  y += 5;

  const char* type = aircraft.type[0] != '\0' ? aircraft.type : "----";
  drawRow(y, "TYPE", type, kColType);
  y += 16;

  const char* registration = aircraft.registration[0] != '\0'
                                 ? aircraft.registration
                                 : "----";
  drawRow(y, "REG", registration, kColRegistration);
  y += 16;

  const char* altitude = aircraft.alt[0] != '\0' ? aircraft.alt : "---";
  drawRow(y, "ALT", altitude, kColAlt);
  y += 16;

  char speed[16];
  formatSpeed(speed, sizeof(speed), aircraft.gs_knots);
  drawRow(y, "GS", speed);
  y += 16;

  char heading[16];
  formatHeading(heading, sizeof(heading), aircraft.nose_deg);
  drawRow(y, "HDG", heading, kColHeading);
  y += 16;

  drawRow(y, "CAT", categoryLabel(aircraft.category), kColCategory);
  drawStatusRow();

  tft.setTextDatum(textdatum_t::top_left);
}

int infoPanelSelectedIndex() {
  return s_selected;
}

void infoPanelSelectNext() {
  const size_t count = services::adsb::aircraftCount();
  if (count == 0) {
    s_selected = 0;
    return;
  }

  s_selected = (s_selected + 1) % static_cast<int>(count);
}

}  // namespace ui
