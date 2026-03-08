#pragma once
#include <Arduino.h>
#include <Preferences.h>

// ============================================================
//  Settings — Persistent NVS storage for Cueputer
// ============================================================

namespace Settings {

// NVS namespace
static const char* NVS_NS = "cueputer";

// WiFi keys
static const char* KEY_WIFI_SSID = "wifi_ssid";
static const char* KEY_WIFI_PASS = "wifi_pass";

// OSC keys
static const char* KEY_OSC_HOST = "osc_host";
static const char* KEY_OSC_PORT = "osc_port";
static const int   DEFAULT_OSC_PORT = 8000;

// Display
static const char* KEY_BRIGHTNESS = "brightness";
static const int   DEFAULT_BRIGHTNESS = 128;

// ——— Runtime defaults (not persisted unless written)
struct Config {
    String wifiSSID;
    String wifiPass;
    String oscHost;
    int    oscPort;
    int    brightness;
};

inline Config load() {
    Preferences prefs;
    prefs.begin(NVS_NS, true); // read-only
    Config cfg;
    cfg.wifiSSID   = prefs.getString(KEY_WIFI_SSID, "");
    cfg.wifiPass   = prefs.getString(KEY_WIFI_PASS, "");
    cfg.oscHost    = prefs.getString(KEY_OSC_HOST, "");
    cfg.oscPort    = prefs.getInt(KEY_OSC_PORT, DEFAULT_OSC_PORT);
    cfg.brightness = prefs.getInt(KEY_BRIGHTNESS, DEFAULT_BRIGHTNESS);
    prefs.end();
    return cfg;
}

inline void save(const Config& cfg) {
    Preferences prefs;
    prefs.begin(NVS_NS, false); // read-write
    prefs.putString(KEY_WIFI_SSID, cfg.wifiSSID);
    prefs.putString(KEY_WIFI_PASS, cfg.wifiPass);
    prefs.putString(KEY_OSC_HOST, cfg.oscHost);
    prefs.putInt(KEY_OSC_PORT, cfg.oscPort);
    prefs.putInt(KEY_BRIGHTNESS, cfg.brightness);
    prefs.end();
}

inline void saveWifi(const String& ssid, const String& pass) {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(KEY_WIFI_SSID, ssid);
    prefs.putString(KEY_WIFI_PASS, pass);
    prefs.end();
}

inline void clear() {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.clear();
    prefs.end();
}

} // namespace Settings
