#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include <functional>
#include <vector>
#include "display.h"
#include "keyboard.h"

// ============================================================
//  Menu — Scrollable, keyboard-driven menu system
// ============================================================

namespace Menu {

using Action = std::function<void()>;

struct Item {
    const char* label;
    const char* subtitle;   // optional short description
    Action      action;
};

// ——— Main menu renderer ——————————————————————————————
class MenuScreen {
public:
    MenuScreen(const char* title) : _title(title), _selected(0), _offset(0) {}

    void addItem(const char* label, const char* subtitle, Action action) {
        _items.push_back({label, subtitle, action});
    }

    void run() {
        _selected = 0;
        _offset   = 0;
        _draw();

        while (true) {
            Keyboard::Event ev = Keyboard::poll();
            switch (ev.key) {
                case Keyboard::UP:
                    if (_selected > 0) {
                        _selected--;
                        if (_selected < _offset) _offset = _selected;
                        _draw();
                    }
                    break;

                case Keyboard::DOWN:
                    if (_selected < (int)_items.size() - 1) {
                        _selected++;
                        if (_selected >= _offset + VISIBLE_ROWS) {
                            _offset = _selected - VISIBLE_ROWS + 1;
                        }
                        _draw();
                    }
                    break;

                case Keyboard::ENTER:
                    if (_items[_selected].action) {
                        _items[_selected].action();
                        // Redraw after returning from module
                        _draw();
                    }
                    break;

                case Keyboard::BACK:
                    return;  // exit menu

                default:
                    break;
            }
            delay(10);
        }
    }

private:
    static const int VISIBLE_ROWS = 6;  // lines that fit between header and status bar
    static const int ROW_H        = 18;
    static const int START_Y      = 18;

    const char*       _title;
    std::vector<Item> _items;
    int               _selected;
    int               _offset;

    void _draw() {
        Display::clear();
        Display::header(_title);

        for (int i = 0; i < VISIBLE_ROWS && (_offset + i) < (int)_items.size(); i++) {
            int idx  = _offset + i;
            int y    = START_Y + i * ROW_H;
            bool sel = (idx == _selected);

            // Highlight selected row
            M5.Display.fillRect(0, y, Display::W, ROW_H - 2,
                                sel ? 0x1A3A5C : Display::COLOR_BG);

            M5.Display.setTextSize(1);
            M5.Display.setCursor(6, y + 3);
            M5.Display.setTextColor(sel ? Display::COLOR_ACCENT : Display::COLOR_FG,
                                    sel ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.print(_items[idx].label);

            if (_items[idx].subtitle && strlen(_items[idx].subtitle) > 0) {
                M5.Display.setTextColor(Display::COLOR_MUTED,
                                        sel ? 0x1A3A5C : Display::COLOR_BG);
                // right-align subtitle
                int subW = strlen(_items[idx].subtitle) * 6;
                M5.Display.setCursor(Display::W - subW - 4, y + 3);
                M5.Display.print(_items[idx].subtitle);
            }
        }

        // Scrollbar
        if ((int)_items.size() > VISIBLE_ROWS) {
            int barH    = (VISIBLE_ROWS * ROW_H * VISIBLE_ROWS) / _items.size();
            int barY    = START_Y + (_offset * VISIBLE_ROWS * ROW_H) / _items.size();
            M5.Display.fillRect(Display::W - 3, START_Y, 3,
                                VISIBLE_ROWS * ROW_H, 0x222222);
            M5.Display.fillRect(Display::W - 3, barY, 3, barH, Display::COLOR_MUTED);
        }

        Display::status("ENT=select  ESC=back  i/k=nav");
    }
};

} // namespace Menu
