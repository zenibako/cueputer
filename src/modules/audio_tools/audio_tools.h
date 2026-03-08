#pragma once
#include <Arduino.h>
#include "../../ui/display.h"
#include "../../ui/keyboard.h"

// ============================================================
//  Audio Tools — tone gen, SPL meter, speaker check
//  Full implementation: task Z-029
// ============================================================

namespace AudioTools {

inline void run() {
    Display::clear();
    Display::header("Audio Tools");
    Display::centerText("Coming soon — Z-029", 60, 1, Display::COLOR_MUTED);
    Display::status("ESC=back");

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        delay(10);
    }
}

} // namespace AudioTools
