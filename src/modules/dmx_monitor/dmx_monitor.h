#pragma once
#include <Arduino.h>
#include "../../ui/display.h"
#include "../../ui/keyboard.h"

// ============================================================
//  DMX Monitor — sACN/Art-Net universe sniffer
//  Full implementation: task Z-030
// ============================================================

namespace DmxMonitor {

inline void run() {
    Display::clear();
    Display::header("DMX Monitor");
    Display::centerText("Coming soon — Z-030", 60, 1, Display::COLOR_MUTED);
    Display::status("ESC=back");

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        delay(10);
    }
}

} // namespace DmxMonitor
