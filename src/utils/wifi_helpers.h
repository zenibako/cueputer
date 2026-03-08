#pragma once

// ============================================================
//  WiFi helpers — pure-logic utilities (no hardware deps)
//  Extracted for testability on native platform.
// ============================================================

#include <cstdint>

namespace WiFiHelpers {

// Color constants (duplicated from Display to avoid hardware dep)
static const uint32_t COLOR_OK    = 0x44FF88;
static const uint32_t COLOR_WARN  = 0xFFAA00;
static const uint32_t COLOR_ERROR = 0xFF3333;

// ——— RSSI to color mapping ——————————————————————————
// Maps WiFi signal strength to a severity color.
//   > -60 dBm : strong (green)
//   > -75 dBm : medium (amber)
//   <= -75 dBm: weak   (red)
inline uint32_t rssiColor(int rssi) {
    if (rssi > -60) return COLOR_OK;
    if (rssi > -75) return COLOR_WARN;
    return COLOR_ERROR;
}

} // namespace WiFiHelpers
