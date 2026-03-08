#pragma once
#include <M5Cardputer.h>

// ============================================================
//  Keyboard — TCA8418 I2C abstraction for Cardputer ADV
//  Uses M5Cardputer library for keyboard access.
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
    M5Cardputer.update();
    Event ev = {NONE, 0, false};

    // M5Cardputer.Keyboard provides keyboard state access
    if (!M5Cardputer.Keyboard.isChange()) return ev;

    auto& state = M5Cardputer.Keyboard.keysState();

    // Navigation keys
    if (state.del) {
        ev.key = BACK;
    } else if (state.enter) {
        ev.key = ENTER;
    } else if (state.tab) {
        ev.key = TAB;
    } else if (M5Cardputer.Keyboard.isKeyPressed('\x1B')) {
        ev.key = BACK;
    } else if (state.fn && M5Cardputer.Keyboard.isKeyPressed('i')) {
        // Fn+i = up
        ev.key = UP;
    } else if (state.fn && M5Cardputer.Keyboard.isKeyPressed('k')) {
        // Fn+k = down
        ev.key = DOWN;
    } else if (state.fn && M5Cardputer.Keyboard.isKeyPressed('j')) {
        // Fn+j = left
        ev.key = LEFT;
    } else if (state.fn && M5Cardputer.Keyboard.isKeyPressed('l')) {
        // Fn+l = right
        ev.key = RIGHT;
    } else if (!state.word.empty()) {
        // Printable character
        char c = state.word[0];
        if (c >= 32 && c < 127) {
            ev.key = CHAR;
            ev.ch  = c;
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
