#pragma once
#include <Arduino.h>
#include "../../ui/display.h"
#include "../../ui/keyboard.h"

// ============================================================
//  Show Companion — Cuejitsu cue list browser + OSC trigger
//  Full implementation: future task
// ============================================================

namespace ShowCompanion {

inline void run() {
    Display::clear();
    Display::header("Show Companion");
    Display::centerText("Coming soon", 60, 1, Display::COLOR_MUTED);
    Display::status("ESC=back");

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        delay(10);
    }
}

} // namespace ShowCompanion
