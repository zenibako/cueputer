#pragma once

// ============================================================
//  Menu navigation — pure-logic scroll/selection (no display)
//  Extracted for testability on native platform.
// ============================================================

namespace MenuNav {

// ——— Scroll state management ————————————————————————
// Holds the selection and viewport offset for a list.
struct ScrollState {
    int selected;
    int offset;
    int itemCount;
    int visibleRows;

    ScrollState(int items, int visible)
        : selected(0), offset(0), itemCount(items), visibleRows(visible) {}

    // Move selection up. Returns true if state changed.
    bool moveUp() {
        if (selected <= 0) return false;
        selected--;
        if (selected < offset) offset = selected;
        return true;
    }

    // Move selection down. Returns true if state changed.
    bool moveDown() {
        if (selected >= itemCount - 1) return false;
        selected++;
        if (selected >= offset + visibleRows) {
            offset = selected - visibleRows + 1;
        }
        return true;
    }
};

// ——— Scrollbar geometry ————————————————————————————
// Calculates the scrollbar thumb position and height (in pixels).
struct ScrollbarGeom {
    int barH;
    int barY;
};

inline ScrollbarGeom calcScrollbar(int itemCount, int visibleRows,
                                   int rowH, int startY, int offset) {
    if (itemCount <= visibleRows) return {0, startY};
    int trackH = visibleRows * rowH;
    int barH   = (trackH * visibleRows) / itemCount;
    int barY   = startY + (offset * trackH) / itemCount;
    return {barH, barY};
}

} // namespace MenuNav
