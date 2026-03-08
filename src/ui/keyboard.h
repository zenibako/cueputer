#pragma once
#include <M5Unified.h>

// ============================================================
//  Keyboard — TCA8418 I2C abstraction for Cardputer ADV
//  M5Unified wraps the TCA8418 — use M5.Keyboard or raw events.
// ============================================================

namespace Keyboard {

// Virtual keys for menu/app navigation
enum Key : uint8_t {
    NONE = 0,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    ENTER,
    BACK,       // ESC / delete / back
    TAB,
    CHAR,       // printable character — call getChar() for value
};

struct Event {
    Key     key;
    char    ch;     // valid when key == CHAR
    bool    held;   // held down (repeat)
};

// ——— Internal state ——————————————————————————————————
static char _lastChar = 0;
static uint32_t _holdTimer = 0;
static const uint32_t HOLD_REPEAT_MS = 300;

// Map M5Cardputer keyboard to abstract keys
inline Event poll() {
    M5.update();
    Event ev = {NONE, 0, false};

    // M5Unified Cardputer keyboard: M5.Keyboard provides character and special key access
    if (!M5.Keyboard.isChange()) return ev;

    auto kb = M5.Keyboard;

    // Navigation keys mapped from Cardputer ADV function row
    if (kb.isKeyPressed('/') || kb.isKeyPressed('\x1B')) {
        ev.key = BACK;
    } else if (kb.isKeyPressed('\n') || kb.isKeyPressed('\r')) {
        ev.key = ENTER;
    } else if (kb.isKeyPressed('\t')) {
        ev.key = TAB;
    } else if (kb.isKeyPressed('i') && kb.isKeyPressed(0)) {
        // Fn+i = up (customize to taste)
        ev.key = UP;
    } else if (kb.isKeyPressed('k') && kb.isKeyPressed(0)) {
        ev.key = DOWN;
    } else if (kb.isKeyPressed('j') && kb.isKeyPressed(0)) {
        ev.key = LEFT;
    } else if (kb.isKeyPressed('l') && kb.isKeyPressed(0)) {
        ev.key = RIGHT;
    } else {
        // Raw character
        auto state = kb.keysState();
        for (int i = 0; i < 6; i++) {
            if (state.hid_keys[i] != 0) {
                char c = kb.getCharFromKeycode(state.hid_keys[i],
                                               state.modifier.bits.left_shift ||
                                               state.modifier.bits.right_shift);
                if (c >= 32 && c < 127) {
                    ev.key = CHAR;
                    ev.ch  = c;
                    break;
                }
            }
        }
    }

    return ev;
}

// ——— Wait for a single keypress ——————————————————————
inline Event waitKey(uint32_t timeoutMs = 0) {
    uint32_t start = millis();
    while (true) {
        Event ev = poll();
        if (ev.key != NONE) return ev;
        if (timeoutMs && (millis() - start) > timeoutMs) return ev;
        delay(10);
    }
}

// ——— Read a full string (for text input fields) ——————
inline String readLine(int x, int y, int maxLen = 32,
                       uint32_t fgColor = 0xFFFFFF, uint32_t bgColor = 0x0D0D0D) {
    String buf = "";
    M5.Display.setCursor(x, y);
    M5.Display.setTextColor(fgColor, bgColor);
    M5.Display.setTextSize(1);

    while (true) {
        Event ev = waitKey();
        if (ev.key == ENTER) break;
        if (ev.key == BACK && buf.length() > 0) {
            buf.remove(buf.length() - 1);
        } else if (ev.key == CHAR && (int)buf.length() < maxLen) {
            buf += ev.ch;
        }
        // Redraw input area
        M5.Display.fillRect(x, y - 1, maxLen * 6 + 8, 10, bgColor);
        M5.Display.setCursor(x, y);
        M5.Display.print(buf);
        M5.Display.print('_');
    }
    return buf;
}

} // namespace Keyboard
