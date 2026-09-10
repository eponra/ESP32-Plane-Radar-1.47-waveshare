#include "ui/status_screens.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <cmath>
#include <cstdio>
#include <cstddef>
#include <cstring>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"

namespace fonts = lgfx::v1::fonts;

namespace {

constexpr int kLineGap = 6;
static inline int displayCenterX() {
  return tft.width() / 2;
}

static inline int displayCenterY() {
  return tft.height() / 2;
}

constexpr int kSpinnerDotCount = 10;
constexpr int kSpinnerRadius = 113;
constexpr int kSpinnerDotRadius = 2;
constexpr int kSpinnerEraseRadius = 4;
constexpr float kSpinnerStepDeg = 6.0f;

struct SpinnerDot {
  int x = 0;
  int y = 0;
  bool drawn = false;
};

char s_connecting_ssid[33];
char s_ssid_line[33];
constexpr int kConnectingTextMaxWidthPx = 220;
float s_spinner_angle_deg = -90.0f;
SpinnerDot s_spinner_dots[kSpinnerDotCount];
bool s_connecting_text_drawn = false;

constexpr auto& kGfxTitle = fonts::FreeSans18pt7b;
constexpr auto& kGfxBody = fonts::FreeSans12pt7b;
constexpr auto& kGfxDetail = fonts::Font2;
constexpr auto& kPortalGfxTitle = fonts::FreeSansBold18pt7b;
constexpr auto& kPortalGfxBody = fonts::FreeSansBold12pt7b;
constexpr auto& kPortalGfxEmphasis = fonts::FreeSansBold18pt7b;
constexpr auto& kConnectingGfxDetail = fonts::FreeSans9pt7b;

struct TextLine {
  const char* text;
  float vlw_size;
  const lgfx::GFXfont* gfx_font;
};

int lineHeightGfx(const lgfx::GFXfont* font) {
  displayFontSetBitmap(tft, font);
  return tft.fontHeight();
}

int lineHeightVlw(float size) {
  displayFontSetSmoothSize(tft, size);
  return tft.fontHeight();
}

void applyLineStyle(const TextLine& line) {
  if (displayFontIsSmooth()) {
    displayFontSetSmoothSize(tft, line.vlw_size);
  } else {
    displayFontSetBitmap(tft, line.gfx_font);
  }
}

void drawTextBlock(uint16_t bg,
                  uint16_t fg,
                  const TextLine* lines,
                  size_t count,
                  bool alignToTop = false) {
  tft.fillScreen(bg);
  tft.setTextColor(fg, bg);
  tft.setTextDatum(textdatum_t::middle_center);

  int totalHeight = 0;

  for (size_t i = 0; i < count; ++i) {
    if (displayFontIsSmooth()) {
      totalHeight += lineHeightVlw(lines[i].vlw_size);
    } else {
      totalHeight += lineHeightGfx(lines[i].gfx_font);
    }

    if (i + 1 < count) {
      totalHeight += kLineGap;
    }
  }

  const int topMargin = alignToTop ? 0 : (tft.height() - totalHeight) / 2;
  int y = topMargin;

  for (size_t i = 0; i < count; ++i) {
    applyLineStyle(lines[i]);

    const int lineHeight = displayFontIsSmooth()
                               ? lineHeightVlw(lines[i].vlw_size)
                               : lineHeightGfx(lines[i].gfx_font);

    tft.drawString(lines[i].text,
                   tft.width() / 2,
                   y + lineHeight / 2);

    y += lineHeight + kLineGap;
  }
}

constexpr float kConnectingDetailVlw = 0.92f;

void applyConnectingDetailStyle() {
  if (displayFontIsSmooth()) {
    displayFontSetSmoothSize(tft, kConnectingDetailVlw);
  } else {
    displayFontSetBitmap(tft, &kConnectingGfxDetail);
  }
}

/** SSID on one line; truncate with … if wider than kConnectingTextMaxWidthPx. */
void fitSsidLine() {
  strncpy(s_ssid_line, s_connecting_ssid, sizeof(s_ssid_line) - 1);
  s_ssid_line[sizeof(s_ssid_line) - 1] = '\0';
  applyConnectingDetailStyle();
  if (tft.textWidth(s_ssid_line) <= kConnectingTextMaxWidthPx) {
    return;
  }
  const size_t len = strlen(s_connecting_ssid);
  for (size_t n = len; n > 0; --n) {
    snprintf(s_ssid_line, sizeof(s_ssid_line), "%.*s…", static_cast<int>(n),
             s_connecting_ssid);
    if (tft.textWidth(s_ssid_line) <= kConnectingTextMaxWidthPx) {
      return;
    }
  }
  strncpy(s_ssid_line, "…", sizeof(s_ssid_line) - 1);
  s_ssid_line[sizeof(s_ssid_line) - 1] = '\0';
}

void drawConnectingText() {
  tft.fillScreen(config::kColorBlack);

  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(config::kTextOnBlack, config::kColorBlack);

  applyConnectingDetailStyle();

  const int detailHeight = tft.fontHeight();
  const int totalHeight = detailHeight * 2 + kLineGap;
  const int blockTop = (tft.height() - totalHeight) / 2;

  constexpr int kPanelPadY = 8;

  tft.fillRect(
      tft.width() / 2 - kConnectingTextMaxWidthPx / 2,
      blockTop - kPanelPadY,
      kConnectingTextMaxWidthPx,
      totalHeight + kPanelPadY * 2,
      config::kColorBlack);

  int y = blockTop;

  tft.drawString("Connecting to",
                 tft.width() / 2,
                 y + detailHeight / 2);

  y += detailHeight + kLineGap;

  tft.drawString(s_ssid_line,
                 tft.width() / 2,
                 y + detailHeight / 2);

  s_connecting_text_drawn = true;
}

void eraseSpinnerDots() {
  for (int i = 0; i < kSpinnerDotCount; ++i) {
    if (!s_spinner_dots[i].drawn) {
      continue;
    }
    tft.fillCircle(s_spinner_dots[i].x, s_spinner_dots[i].y, kSpinnerEraseRadius,
                   config::kColorBlack);
    s_spinner_dots[i].drawn = false;
  }
}

void drawSpinnerDots() {
  constexpr float kDegToRad = 0.01745329252f;
  const float head_rad = s_spinner_angle_deg * kDegToRad;

  for (int i = 0; i < kSpinnerDotCount; ++i) {
    const float a = head_rad - static_cast<float>(i) * (6.283185307f / kSpinnerDotCount);
    const int centerX = tft.width() / 2;
    const int centerY = tft.height() / 2;

    const int x = centerX + static_cast<int>(
        std::lround(std::cos(a) * kSpinnerRadius));

    const int y = centerY + static_cast<int>(
        std::lround(std::sin(a) * kSpinnerRadius));

    const int fade = 255 - i * 22;
    const uint16_t color = tft.color565(0, fade, 0);
    tft.fillSmoothCircle(x, y, kSpinnerDotRadius, color);

    s_spinner_dots[i].x = x;
    s_spinner_dots[i].y = y;
    s_spinner_dots[i].drawn = true;
  }
}

}  // namespace

void statusScreenConnectingBegin(const char* ssid) {
  const char* name = (ssid != nullptr && ssid[0] != '\0') ? ssid : "network";
  strncpy(s_connecting_ssid, name, sizeof(s_connecting_ssid) - 1);
  s_connecting_ssid[sizeof(s_connecting_ssid) - 1] = '\0';
  fitSsidLine();
  s_spinner_angle_deg = -90.0f;
  for (auto& dot : s_spinner_dots) {
    dot.drawn = false;
  }
  s_connecting_text_drawn = false;
  drawConnectingText();
  drawSpinnerDots();
}

void statusScreenConnectingTick() {
  if (!s_connecting_text_drawn) {
    drawConnectingText();
  }
  eraseSpinnerDots();
  s_spinner_angle_deg += kSpinnerStepDeg;
  if (s_spinner_angle_deg >= 270.0f) {
    s_spinner_angle_deg -= 360.0f;
  }
  drawSpinnerDots();
}

void statusScreenPortal() {
  const TextLine lines[] = {
      {"Wi-Fi setup", 1.15f, &kPortalGfxTitle},
      {"1. Join network:", 1.05f, &kPortalGfxBody},
      {config::kPortalApName, 1.12f, &kPortalGfxEmphasis},
      {"2. Open in browser:", 1.05f, &kPortalGfxBody},
      {config::kPortalHostUrl, 1.12f, &kPortalGfxEmphasis},
      {"or 192.168.4.1", 1.0f, &kPortalGfxBody},
  };
  drawTextBlock(config::kColorYellow,
              config::kTextOnYellow,
              lines,
              sizeof(lines) / sizeof(lines[0]),
              true);
}

void statusScreenConnectFailed() {
  const TextLine lines[] = {
      {"Could not connect", 1.15f, &kGfxTitle},
      {"Check Wi-Fi password", 1.0f, &kGfxBody},
      {"and signal strength.", 1.0f, &kGfxBody},
      {"Hold BOOT 3 sec", 1.0f, &kGfxBody},
      {"to reset Wi-Fi", 1.0f, &kGfxBody},
  };
  drawTextBlock(config::kColorYellow, config::kTextOnYellow, lines,
                sizeof(lines) / sizeof(lines[0]));
}

void statusScreenWifiReset() {
  const TextLine lines[] = {
      {"Wi-Fi reset", 1.15f, &kPortalGfxTitle},
      {"Restarting...", 1.05f, &kPortalGfxBody},
  };
  drawTextBlock(config::kColorYellow, config::kTextOnYellow, lines,
                sizeof(lines) / sizeof(lines[0]));
}
