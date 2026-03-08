#include <unity.h>
#include <cstdint>

#include "../../../src/utils/wifi_helpers.h"

using namespace WiFiHelpers;

// ============================================================
//  rssiColor() tests
// ============================================================

void test_rssi_strong_signal() {
    // -30 dBm — excellent signal
    TEST_ASSERT_EQUAL_UINT32(COLOR_OK, rssiColor(-30));
}

void test_rssi_good_signal() {
    // -55 dBm — good signal, still above -60
    TEST_ASSERT_EQUAL_UINT32(COLOR_OK, rssiColor(-55));
}

void test_rssi_boundary_minus_59() {
    // -59 dBm is > -60, should be green
    TEST_ASSERT_EQUAL_UINT32(COLOR_OK, rssiColor(-59));
}

void test_rssi_boundary_minus_60() {
    // -60 dBm is NOT > -60, should be amber
    TEST_ASSERT_EQUAL_UINT32(COLOR_WARN, rssiColor(-60));
}

void test_rssi_medium_signal() {
    // -65 dBm — medium signal
    TEST_ASSERT_EQUAL_UINT32(COLOR_WARN, rssiColor(-65));
}

void test_rssi_boundary_minus_74() {
    // -74 dBm is > -75, should be amber
    TEST_ASSERT_EQUAL_UINT32(COLOR_WARN, rssiColor(-74));
}

void test_rssi_boundary_minus_75() {
    // -75 dBm is NOT > -75, should be red
    TEST_ASSERT_EQUAL_UINT32(COLOR_ERROR, rssiColor(-75));
}

void test_rssi_weak_signal() {
    // -85 dBm — weak signal
    TEST_ASSERT_EQUAL_UINT32(COLOR_ERROR, rssiColor(-85));
}

void test_rssi_very_weak() {
    // -100 dBm — barely detectable
    TEST_ASSERT_EQUAL_UINT32(COLOR_ERROR, rssiColor(-100));
}

void test_rssi_zero() {
    // 0 dBm — theoretical maximum
    TEST_ASSERT_EQUAL_UINT32(COLOR_OK, rssiColor(0));
}

// ============================================================
//  Test runner
// ============================================================

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_rssi_strong_signal);
    RUN_TEST(test_rssi_good_signal);
    RUN_TEST(test_rssi_boundary_minus_59);
    RUN_TEST(test_rssi_boundary_minus_60);
    RUN_TEST(test_rssi_medium_signal);
    RUN_TEST(test_rssi_boundary_minus_74);
    RUN_TEST(test_rssi_boundary_minus_75);
    RUN_TEST(test_rssi_weak_signal);
    RUN_TEST(test_rssi_very_weak);
    RUN_TEST(test_rssi_zero);

    return UNITY_END();
}
