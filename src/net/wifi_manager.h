#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <M5Unified.h>
#include "../config/settings.h"
#include "../ui/display.h"
#include "../ui/keyboard.h"

// ============================================================
//  WiFi Manager — scan, connect, save credentials to NVS
// ============================================================

namespace WiFiManager {

static const uint32_t CONNECT_TIMEOUT_MS = 15000;

// ——— Quick connect from saved creds ——————————————————
inline bool connectSaved() {
    Settings::Config cfg = Settings::load();
    if (cfg.wifiSSID.isEmpty()) return false;

    WiFi.begin(cfg.wifiSSID.c_str(), cfg.wifiPass.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > CONNECT_TIMEOUT_MS) return false;
        delay(200);
    }
    return true;
}

// ——— Connect to a specific SSID ————————————————————
inline bool connect(const String& ssid, const String& pass, bool save = true) {
    WiFi.begin(ssid.c_str(), pass.c_str());
    uint32_t start = millis();

    Display::status("Connecting...", Display::COLOR_WARN);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > CONNECT_TIMEOUT_MS) {
            Display::status("Connection failed", Display::COLOR_ERROR);
            delay(1500);
            return false;
        }
        delay(200);
    }

    if (save) {
        Settings::saveWifi(ssid, pass);
    }

    char buf[48];
    snprintf(buf, sizeof(buf), "IP: %s", WiFi.localIP().toString().c_str());
    Display::status(buf, Display::COLOR_OK);
    delay(1000);
    return true;
}

// ——— Scan for networks and present selection UI ——————
inline void scanAndConnect() {
    Display::clear();
    Display::header("WiFi Manager");
    Display::centerText("Scanning...", 60, 1, Display::COLOR_MUTED);

    int n = WiFi.scanNetworks();

    if (n == 0) {
        Display::status("No networks found", Display::COLOR_WARN);
        delay(2000);
        return;
    }

    // Sort by RSSI (descending) — simple bubble sort for small n
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (WiFi.RSSI(j) < WiFi.RSSI(j + 1)) {
                // swap
                String ssidTmp = WiFi.SSID(j);
                int32_t rssiTmp = WiFi.RSSI(j);
                // Note: WiFi scan results can't be reordered directly,
                // so we track an index array instead
            }
        }
    }

    // Build index array sorted by RSSI
    std::vector<int> idx(n);
    for (int i = 0; i < n; i++) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return WiFi.RSSI(a) > WiFi.RSSI(b);
    });

    // Display list
    int selected  = 0;
    int offset    = 0;
    const int VIS = 5;

    auto drawList = [&]() {
        Display::clear();
        Display::header("WiFi — Select Network");
        for (int i = 0; i < VIS && (offset + i) < n; i++) {
            int ni  = idx[offset + i];
            int y   = 18 + i * 18;
            bool sel = (offset + i) == selected;
            M5.Display.fillRect(0, y, Display::W, 16,
                                sel ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.setTextSize(1);
            M5.Display.setCursor(4, y + 3);
            M5.Display.setTextColor(sel ? Display::COLOR_ACCENT : Display::COLOR_FG,
                                    sel ? 0x1A3A5C : Display::COLOR_BG);
            // Trim long SSIDs
            String ssid = WiFi.SSID(ni);
            if (ssid.length() > 22) ssid = ssid.substring(0, 22) + "..";
            M5.Display.print(ssid);
            // RSSI indicator
            int rssi = WiFi.RSSI(ni);
            char rssiStr[8];
            snprintf(rssiStr, sizeof(rssiStr), "%ddB", rssi);
            int rx = Display::W - strlen(rssiStr) * 6 - 4;
            M5.Display.setCursor(rx, y + 3);
            uint32_t rc = rssi > -60 ? Display::COLOR_OK :
                          rssi > -75 ? Display::COLOR_WARN : Display::COLOR_ERROR;
            M5.Display.setTextColor(rc, sel ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.print(rssiStr);
        }
        Display::status("ENT=connect  ESC=back  i/k=nav");
    };

    drawList();

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;

        if (ev.key == Keyboard::UP && selected > 0) {
            selected--;
            if (selected < offset) offset = selected;
            drawList();
        } else if (ev.key == Keyboard::DOWN && selected < n - 1) {
            selected++;
            if (selected >= offset + VIS) offset = selected - VIS + 1;
            drawList();
        } else if (ev.key == Keyboard::ENTER) {
            int ni = idx[selected];
            String ssid = WiFi.SSID(ni);
            bool isOpen  = WiFi.encryptionType(ni) == WIFI_AUTH_OPEN;

            // Ask for password
            String pass = "";
            if (!isOpen) {
                Display::clear();
                Display::header("WiFi — Password");
                M5.Display.setTextColor(Display::COLOR_FG, Display::COLOR_BG);
                M5.Display.setTextSize(1);
                M5.Display.setCursor(4, 20);
                M5.Display.print(ssid);
                M5.Display.setCursor(4, 40);
                M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
                M5.Display.print("Password:");
                pass = Keyboard::readLine(4, 56);
            }

            connect(ssid, pass, true);
            return;
        }

        delay(10);
    }
}

// ——— Show current connection status ——————————————————
inline void showStatus() {
    Display::clear();
    Display::header("WiFi Status");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(Display::COLOR_FG, Display::COLOR_BG);

    if (WiFi.status() == WL_CONNECTED) {
        M5.Display.setCursor(4, 24);
        M5.Display.setTextColor(Display::COLOR_OK, Display::COLOR_BG);
        M5.Display.print("Connected");
        M5.Display.setTextColor(Display::COLOR_FG, Display::COLOR_BG);
        M5.Display.setCursor(4, 40);
        M5.Display.print("SSID: ");
        M5.Display.print(WiFi.SSID());
        M5.Display.setCursor(4, 54);
        M5.Display.print("IP:   ");
        M5.Display.print(WiFi.localIP().toString());
        M5.Display.setCursor(4, 68);
        M5.Display.print("RSSI: ");
        M5.Display.print(WiFi.RSSI());
        M5.Display.print(" dBm");
    } else {
        M5.Display.setCursor(4, 40);
        M5.Display.setTextColor(Display::COLOR_ERROR, Display::COLOR_BG);
        M5.Display.print("Not connected");
    }

    Display::status("ESC=back  R=rescan");

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        if (ev.key == Keyboard::CHAR && (ev.ch == 'r' || ev.ch == 'R')) {
            scanAndConnect();
            return;
        }
        delay(10);
    }
}

// ——— Disconnect and forget saved creds ——————————————
inline void forget() {
    WiFi.disconnect(true);
    Preferences prefs;
    prefs.begin("cueputer", false);
    prefs.remove("wifi_ssid");
    prefs.remove("wifi_pass");
    prefs.end();
}

} // namespace WiFiManager
