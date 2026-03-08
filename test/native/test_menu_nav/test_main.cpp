#include <unity.h>

#include "../../../src/utils/menu_nav.h"

using namespace MenuNav;

// ============================================================
//  ScrollState::moveUp() / moveDown() tests
// ============================================================

void test_initial_state() {
    ScrollState s(8, 6);
    TEST_ASSERT_EQUAL(0, s.selected);
    TEST_ASSERT_EQUAL(0, s.offset);
}

void test_move_up_at_top_is_noop() {
    ScrollState s(8, 6);
    bool changed = s.moveUp();
    TEST_ASSERT_FALSE(changed);
    TEST_ASSERT_EQUAL(0, s.selected);
    TEST_ASSERT_EQUAL(0, s.offset);
}

void test_move_down_basic() {
    ScrollState s(8, 6);
    bool changed = s.moveDown();
    TEST_ASSERT_TRUE(changed);
    TEST_ASSERT_EQUAL(1, s.selected);
    TEST_ASSERT_EQUAL(0, s.offset);
}

void test_move_down_at_bottom_is_noop() {
    ScrollState s(3, 6);
    s.moveDown();
    s.moveDown();
    // Now at index 2 (last item)
    bool changed = s.moveDown();
    TEST_ASSERT_FALSE(changed);
    TEST_ASSERT_EQUAL(2, s.selected);
}

void test_move_up_from_middle() {
    ScrollState s(8, 6);
    s.moveDown(); // sel=1
    s.moveDown(); // sel=2
    bool changed = s.moveUp();
    TEST_ASSERT_TRUE(changed);
    TEST_ASSERT_EQUAL(1, s.selected);
}

void test_scroll_down_past_visible() {
    ScrollState s(10, 3);  // 10 items, 3 visible
    // Move down to index 3 (first off-screen item)
    for (int i = 0; i < 3; i++) s.moveDown();
    // selected=3, should scroll: offset = 3 - 3 + 1 = 1
    TEST_ASSERT_EQUAL(3, s.selected);
    TEST_ASSERT_EQUAL(1, s.offset);
}

void test_scroll_continues_down() {
    ScrollState s(10, 3);
    for (int i = 0; i < 6; i++) s.moveDown();
    // selected=6, offset should be 6-3+1=4
    TEST_ASSERT_EQUAL(6, s.selected);
    TEST_ASSERT_EQUAL(4, s.offset);
}

void test_scroll_up_adjusts_offset() {
    ScrollState s(10, 3);
    // Scroll down to selected=5, offset=3
    for (int i = 0; i < 5; i++) s.moveDown();
    TEST_ASSERT_EQUAL(5, s.selected);
    TEST_ASSERT_EQUAL(3, s.offset);

    // Scroll up to selected=2 — offset should follow
    for (int i = 0; i < 3; i++) s.moveUp();
    TEST_ASSERT_EQUAL(2, s.selected);
    TEST_ASSERT_EQUAL(2, s.offset);
}

void test_few_items_no_scroll() {
    ScrollState s(2, 6);  // fewer items than visible rows
    s.moveDown();
    TEST_ASSERT_EQUAL(1, s.selected);
    TEST_ASSERT_EQUAL(0, s.offset);  // no scrolling needed
    s.moveDown(); // noop
    TEST_ASSERT_EQUAL(1, s.selected);
}

void test_single_item() {
    ScrollState s(1, 6);
    TEST_ASSERT_FALSE(s.moveDown());
    TEST_ASSERT_FALSE(s.moveUp());
    TEST_ASSERT_EQUAL(0, s.selected);
    TEST_ASSERT_EQUAL(0, s.offset);
}

// ============================================================
//  calcScrollbar() tests
// ============================================================

void test_scrollbar_no_scroll_needed() {
    auto g = calcScrollbar(3, 6, 18, 18, 0);
    // itemCount <= visibleRows → barH=0
    TEST_ASSERT_EQUAL(0, g.barH);
}

void test_scrollbar_at_top() {
    // 10 items, 6 visible, rowH=18, startY=18, offset=0
    auto g = calcScrollbar(10, 6, 18, 18, 0);
    int trackH = 6 * 18;  // 108
    int expectedH = (trackH * 6) / 10;  // 64
    int expectedY = 18;
    TEST_ASSERT_EQUAL(expectedH, g.barH);
    TEST_ASSERT_EQUAL(expectedY, g.barY);
}

void test_scrollbar_scrolled_down() {
    // offset=4
    auto g = calcScrollbar(10, 6, 18, 18, 4);
    int trackH = 108;
    int expectedH = (trackH * 6) / 10;  // 64
    int expectedY = 18 + (4 * trackH) / 10;  // 18 + 43 = 61
    TEST_ASSERT_EQUAL(expectedH, g.barH);
    TEST_ASSERT_EQUAL(expectedY, g.barY);
}

// ============================================================
//  Test runner
// ============================================================

int main() {
    UNITY_BEGIN();

    // ScrollState
    RUN_TEST(test_initial_state);
    RUN_TEST(test_move_up_at_top_is_noop);
    RUN_TEST(test_move_down_basic);
    RUN_TEST(test_move_down_at_bottom_is_noop);
    RUN_TEST(test_move_up_from_middle);
    RUN_TEST(test_scroll_down_past_visible);
    RUN_TEST(test_scroll_continues_down);
    RUN_TEST(test_scroll_up_adjusts_offset);
    RUN_TEST(test_few_items_no_scroll);
    RUN_TEST(test_single_item);

    // calcScrollbar
    RUN_TEST(test_scrollbar_no_scroll_needed);
    RUN_TEST(test_scrollbar_at_top);
    RUN_TEST(test_scrollbar_scrolled_down);

    return UNITY_END();
}
