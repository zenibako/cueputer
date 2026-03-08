#pragma once
#include <Arduino.h>
#include "../../ui/display.h"
#include "../../ui/keyboard.h"

// ============================================================
//  BLE MIDI Bridge — keyboard → BLE MIDI + OSC bridge mode
//  Full implementation: task Z-028
// ============================================================

namespace MidiBridge {

inline void run() {
    Display::clear();
    Display::header("MIDI Bridge");
    Display::centerText("Coming soon — Z-028", 60, 1, Display::COLOR_MUTED);
    Display::status("ESC=back");

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        delay(10);
    }
}

} // namespace MidiBridge
