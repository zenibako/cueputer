// ============================================================
//  Cueputer — Theater tech multitool firmware
//  Target: M5Stack Cardputer ADV (ESP32-S3)
// ============================================================
#include <M5Cardputer.h>
#include <WiFi.h>
#include <vector>
#include <algorithm>

#include "ui/display.h"
#include "ui/menu.h"
#include "ui/keyboard.h"
#include "net/wifi_manager.h"
#include "config/settings.h"

// Modules
#include "modules/osc_tester/osc_tester.h"
#include "modules/midi_bridge/midi_bridge.h"
#include "modules/audio_tools/audio_tools.h"
#include "modules/dmx_monitor/dmx_monitor.h"
#include "modules/show_companion/show_companion.h"

// ——— WiFi menu ——————————————————————————————————————
void wifiMenu() {
    Menu::MenuScreen wm("WiFi");
    wm.addItem("Status",         "show IP & RSSI",  []() { WiFiManager::showStatus(); });
    wm.addItem("Scan & Connect", "find networks",   []() { WiFiManager::scanAndConnect(); });
    wm.addItem("Forget Network", "clear saved WiFi",[]() {
        WiFiManager::forget();
        Display::status("WiFi credentials cleared", Display::COLOR_WARN);
        delay(1500);
    });
    wm.run();
}

// ——— System info screen ——————————————————————————————
void systemInfo() {
    Display::clear();
    Display::header("System Info");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(Display::COLOR_FG, Display::COLOR_BG);

    int y = 22;
    auto row = [&](const char* label, const String& val) {
        M5.Display.setCursor(4, y);
        M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
        M5.Display.print(label);
        M5.Display.setTextColor(Display::COLOR_FG, Display::COLOR_BG);
        M5.Display.print(val);
        y += 14;
    };

    row("FW:      ", "Cueputer v0.1.0");
    row("Board:   ", "Cardputer ADV");
    row("Chip:    ", ESP.getChipModel());
    row("CPU:     ", String(ESP.getCpuFreqMHz()) + " MHz");
    row("RAM:     ", String(ESP.getFreeHeap() / 1024) + " KB free");
    row("Flash:   ", String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");

    Display::status("ESC=back");

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        delay(10);
    }
}

// ——— Main menu ———————————————————————————————————————
void mainMenu() {
    Menu::MenuScreen menu("CUEPUTER");

    menu.addItem("OSC Tester",     "send/recv OSC",   []() { OscTester::run(); });
    menu.addItem("MIDI Bridge",    "BLE MIDI",        []() { MidiBridge::run(); });
    menu.addItem("Audio Tools",    "tone/SPL",        []() { AudioTools::run(); });
    menu.addItem("DMX Monitor",    "sACN/ArtNet",     []() { DmxMonitor::run(); });
    menu.addItem("Show Companion", "cue browser",     []() { ShowCompanion::run(); });
    menu.addItem("WiFi",           "network mgmt",    []() { wifiMenu(); });
    menu.addItem("System Info",    "chip/mem info",   []() { systemInfo(); });

    menu.run();
}

// ============================================================
//  setup()
// ============================================================
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    // Set display brightness from saved settings
    Settings::Config saved = Settings::load();
    M5.Display.setBrightness(saved.brightness);

    // Boot splash
    Display::splash();

    // Attempt WiFi auto-connect
    Display::clear();
    Display::header("Cueputer");
    Display::centerText("Connecting to WiFi...", 60, 1, Display::COLOR_MUTED);

    if (WiFiManager::connectSaved()) {
        char buf[48];
        snprintf(buf, sizeof(buf), "WiFi: %s", WiFi.SSID().c_str());
        Display::status(buf, Display::COLOR_OK);
    } else {
        Display::status("WiFi: not connected", Display::COLOR_MUTED);
    }
    delay(800);
}

// ============================================================
//  loop()
// ============================================================
void loop() {
    mainMenu();
    // mainMenu() returns only if user presses ESC on the root menu
    // Just loop back into it
}
