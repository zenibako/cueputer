#include <unity.h>
#include <string>
#include <vector>

// osc_helpers.h auto-detects native build (no ARDUINO defined)
// and uses std::string as String
#include "../../../src/utils/osc_helpers.h"

using namespace OscHelpers;

// ============================================================
//  truncStr() tests
// ============================================================

void test_trunc_short_string_unchanged() {
    TEST_ASSERT_EQUAL_STRING("hi", truncStr("hi", 10).c_str());
}

void test_trunc_exact_length_unchanged() {
    TEST_ASSERT_EQUAL_STRING("hello", truncStr("hello", 5).c_str());
}

void test_trunc_long_string_truncated() {
    TEST_ASSERT_EQUAL_STRING("hel..", truncStr("hello world", 5).c_str());
}

void test_trunc_empty_string() {
    TEST_ASSERT_EQUAL_STRING("", truncStr("", 10).c_str());
}

void test_trunc_maxlen_zero() {
    TEST_ASSERT_EQUAL_STRING("", truncStr("hello", 0).c_str());
}

void test_trunc_maxlen_two() {
    // maxLen=2 means we can fit 2 chars; string "ab" fits, "abc" gets cut
    TEST_ASSERT_EQUAL_STRING("ab", truncStr("ab", 2).c_str());
    TEST_ASSERT_EQUAL_STRING("ab", truncStr("abc", 2).c_str());
}

void test_trunc_maxlen_three() {
    TEST_ASSERT_EQUAL_STRING("a..", truncStr("abcdef", 3).c_str());
}

// ============================================================
//  normalizeOscAddress() tests
// ============================================================

void test_normalize_already_prefixed() {
    TEST_ASSERT_EQUAL_STRING("/cue/go", normalizeOscAddress("/cue/go").c_str());
}

void test_normalize_missing_slash() {
    TEST_ASSERT_EQUAL_STRING("/cue/go", normalizeOscAddress("cue/go").c_str());
}

void test_normalize_empty_string() {
    TEST_ASSERT_EQUAL_STRING("/", normalizeOscAddress("").c_str());
}

void test_normalize_just_slash() {
    TEST_ASSERT_EQUAL_STRING("/", normalizeOscAddress("/").c_str());
}

void test_normalize_single_char() {
    TEST_ASSERT_EQUAL_STRING("/x", normalizeOscAddress("x").c_str());
}

// ============================================================
//  formatOscMessage() tests
// ============================================================

void test_format_no_args() {
    std::vector<OscArg> args;
    TEST_ASSERT_EQUAL_STRING("/go", formatOscMessage("/go", args).c_str());
}

void test_format_int_arg() {
    std::vector<OscArg> args = {{ARG_INT, 42, 0.0f, "", false}};
    TEST_ASSERT_EQUAL_STRING("/cue 42", formatOscMessage("/cue", args).c_str());
}

void test_format_float_arg() {
    std::vector<OscArg> args = {{ARG_FLOAT, 0, 3.14159f, "", false}};
    std::string result = formatOscMessage("/level", args);
    TEST_ASSERT_EQUAL_STRING("/level 3.14", result.c_str());
}

void test_format_string_arg() {
    std::vector<OscArg> args = {{ARG_STRING, 0, 0.0f, "hello", false}};
    TEST_ASSERT_EQUAL_STRING("/msg hello", formatOscMessage("/msg", args).c_str());
}

void test_format_bool_true() {
    std::vector<OscArg> args = {{ARG_BOOL, 0, 0.0f, "", true}};
    TEST_ASSERT_EQUAL_STRING("/flag T", formatOscMessage("/flag", args).c_str());
}

void test_format_bool_false() {
    std::vector<OscArg> args = {{ARG_BOOL, 0, 0.0f, "", false}};
    TEST_ASSERT_EQUAL_STRING("/flag F", formatOscMessage("/flag", args).c_str());
}

void test_format_mixed_args() {
    std::vector<OscArg> args = {
        {ARG_INT,    1,    0.0f,    "",      false},
        {ARG_FLOAT,  0,    2.50f,   "",      false},
        {ARG_STRING, 0,    0.0f,    "test",  false},
        {ARG_BOOL,   0,    0.0f,    "",      true},
    };
    TEST_ASSERT_EQUAL_STRING("/multi 1 2.50 test T",
                             formatOscMessage("/multi", args).c_str());
}

// ============================================================
//  formatArgDisplay() tests
// ============================================================

void test_arg_display_int() {
    OscArg a = {ARG_INT, 99, 0.0f, "", false};
    TEST_ASSERT_EQUAL_STRING("[i] 99", formatArgDisplay(a).c_str());
}

void test_arg_display_float() {
    OscArg a = {ARG_FLOAT, 0, 1.5f, "", false};
    TEST_ASSERT_EQUAL_STRING("[f] 1.50", formatArgDisplay(a).c_str());
}

void test_arg_display_string() {
    OscArg a = {ARG_STRING, 0, 0.0f, "world", false};
    TEST_ASSERT_EQUAL_STRING("[s] world", formatArgDisplay(a).c_str());
}

void test_arg_display_bool_true() {
    OscArg a = {ARG_BOOL, 0, 0.0f, "", true};
    TEST_ASSERT_EQUAL_STRING("[b] true", formatArgDisplay(a).c_str());
}

void test_arg_display_bool_false() {
    OscArg a = {ARG_BOOL, 0, 0.0f, "", false};
    TEST_ASSERT_EQUAL_STRING("[b] false", formatArgDisplay(a).c_str());
}

void test_arg_display_negative_int() {
    OscArg a = {ARG_INT, -42, 0.0f, "", false};
    TEST_ASSERT_EQUAL_STRING("[i] -42", formatArgDisplay(a).c_str());
}

void test_arg_display_zero_float() {
    OscArg a = {ARG_FLOAT, 0, 0.0f, "", false};
    TEST_ASSERT_EQUAL_STRING("[f] 0.00", formatArgDisplay(a).c_str());
}

// ============================================================
//  histAppend() tests
// ============================================================

void test_hist_append_to_empty() {
    std::vector<HistoryEntry> hist;
    histAppend(hist, "TX", "/go", true);
    TEST_ASSERT_EQUAL(1, (int)hist.size());
    TEST_ASSERT_EQUAL_STRING("TX", hist[0].src.c_str());
    TEST_ASSERT_EQUAL_STRING("/go", hist[0].msg.c_str());
    TEST_ASSERT_TRUE(hist[0].sent);
}

void test_hist_append_preserves_order() {
    std::vector<HistoryEntry> hist;
    histAppend(hist, "TX", "first", true);
    histAppend(hist, "RX", "second", false);
    TEST_ASSERT_EQUAL(2, (int)hist.size());
    TEST_ASSERT_EQUAL_STRING("first", hist[0].msg.c_str());
    TEST_ASSERT_EQUAL_STRING("second", hist[1].msg.c_str());
    TEST_ASSERT_TRUE(hist[0].sent);
    TEST_ASSERT_FALSE(hist[1].sent);
}

void test_hist_append_evicts_oldest_at_max() {
    std::vector<HistoryEntry> hist;
    int maxH = 3;  // small cap for testing
    for (int i = 0; i < 3; i++) {
        histAppend(hist, "TX", "msg" + std::to_string(i), true, maxH);
    }
    TEST_ASSERT_EQUAL(3, (int)hist.size());

    // Adding a 4th should evict msg0
    histAppend(hist, "TX", "msg3", true, maxH);
    TEST_ASSERT_EQUAL(3, (int)hist.size());
    TEST_ASSERT_EQUAL_STRING("msg1", hist[0].msg.c_str());
    TEST_ASSERT_EQUAL_STRING("msg3", hist[2].msg.c_str());
}

void test_hist_append_many_evictions() {
    std::vector<HistoryEntry> hist;
    int maxH = 2;
    for (int i = 0; i < 10; i++) {
        histAppend(hist, "TX", "m" + std::to_string(i), true, maxH);
    }
    TEST_ASSERT_EQUAL(2, (int)hist.size());
    TEST_ASSERT_EQUAL_STRING("m8", hist[0].msg.c_str());
    TEST_ASSERT_EQUAL_STRING("m9", hist[1].msg.c_str());
}

// ============================================================
//  Test runner
// ============================================================

int main() {
    UNITY_BEGIN();

    // trunc
    RUN_TEST(test_trunc_short_string_unchanged);
    RUN_TEST(test_trunc_exact_length_unchanged);
    RUN_TEST(test_trunc_long_string_truncated);
    RUN_TEST(test_trunc_empty_string);
    RUN_TEST(test_trunc_maxlen_zero);
    RUN_TEST(test_trunc_maxlen_two);
    RUN_TEST(test_trunc_maxlen_three);

    // normalizeOscAddress
    RUN_TEST(test_normalize_already_prefixed);
    RUN_TEST(test_normalize_missing_slash);
    RUN_TEST(test_normalize_empty_string);
    RUN_TEST(test_normalize_just_slash);
    RUN_TEST(test_normalize_single_char);

    // formatOscMessage
    RUN_TEST(test_format_no_args);
    RUN_TEST(test_format_int_arg);
    RUN_TEST(test_format_float_arg);
    RUN_TEST(test_format_string_arg);
    RUN_TEST(test_format_bool_true);
    RUN_TEST(test_format_bool_false);
    RUN_TEST(test_format_mixed_args);

    // formatArgDisplay
    RUN_TEST(test_arg_display_int);
    RUN_TEST(test_arg_display_float);
    RUN_TEST(test_arg_display_string);
    RUN_TEST(test_arg_display_bool_true);
    RUN_TEST(test_arg_display_bool_false);
    RUN_TEST(test_arg_display_negative_int);
    RUN_TEST(test_arg_display_zero_float);

    // histAppend
    RUN_TEST(test_hist_append_to_empty);
    RUN_TEST(test_hist_append_preserves_order);
    RUN_TEST(test_hist_append_evicts_oldest_at_max);
    RUN_TEST(test_hist_append_many_evictions);

    return UNITY_END();
}
