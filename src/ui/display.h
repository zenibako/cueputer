#pragma once
#include <M5Unified.h>

// ============================================================
//  Display helpers — Cueputer 1.14" 240×135 LCD
// ============================================================

namespace Display {

// Palette
static const uint32_t COLOR_BG      = 0x0D0D0D;  // near-black
static const uint32_t COLOR_FG      = 0xFFFFFF;  // white
static const uint32_t COLOR_ACCENT  = 0x00C8FF;  // cyan-ish theater blue
static const uint32_t COLOR_MUTED   = 0x888888;  // dim
static const uint32_t COLOR_WARN    = 0xFFAA00;  // amber
static const uint32_t COLOR_ERROR   = 0xFF3333;  // red
static const uint32_t COLOR_OK      = 0x44FF88;  // green

// Screen dimensions
static const int W = 240;
static const int H = 135;

// ——— Boot splash ——————————————————————————————————————
inline void splash() {
    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(COLOR_ACCENT, COLOR_BG);
    M5.Display.setCursor(60, 40);
    M5.Display.print("CUEPUTER");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(COLOR_MUTED, COLOR_BG);
    M5.Display.setCursor(70, 70);
    M5.Display.print("theater tech multitool");

    M5.Display.setCursor(98, 95);
    M5.Display.setTextColor(COLOR_MUTED, COLOR_BG);
    M5.Display.print("v0.1.0");
    delay(1500);
}

// ——— Clear to background ——————————————————————————————
inline void clear() {
    M5.Display.fillScreen(COLOR_BG);
}

// ——— Header bar ———————————————————————————————————————
inline void header(const char* title, bool wifiConnected = false) {
    M5.Display.fillRect(0, 0, W, 16, 0x1A1A2E);
    M5.Display.setTextColor(COLOR_ACCENT, 0x1A1A2E);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(4, 4);
    M5.Display.print(title);
    if (wifiConnected) {
        M5.Display.setCursor(W - 20, 4);
        M5.Display.setTextColor(COLOR_OK, 0x1A1A2E);
        M5.Display.print("WiFi");
    }
}

// ——— Status line at bottom ——————————————————————————
inline void status(const char* msg, uint32_t color = COLOR_MUTED) {
    M5.Display.fillRect(0, H - 12, W, 12, 0x111111);
    M5.Display.setTextColor(color, 0x111111);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(4, H - 10);
    M5.Display.print(msg);
}

// ——— Center text ———————————————————————————————————
inline void centerText(const char* text, int y, int textSize = 1,
                       uint32_t color = COLOR_FG) {
    M5.Display.setTextSize(textSize);
    int len   = strlen(text) * 6 * textSize;
    int x     = (W - len) / 2;
    M5.Display.setTextColor(color, COLOR_BG);
    M5.Display.setCursor(x, y);
    M5.Display.print(text);
}

// ——— Progress bar ————————————————————————————————————
inline void progressBar(int x, int y, int w, int h,
                        float pct, uint32_t barColor = COLOR_ACCENT) {
    M5.Display.drawRect(x, y, w, h, COLOR_MUTED);
    int filled = (int)(pct * (w - 2));
    if (filled > 0) {
        M5.Display.fillRect(x + 1, y + 1, filled, h - 2, barColor);
    }
}

} // namespace Display
