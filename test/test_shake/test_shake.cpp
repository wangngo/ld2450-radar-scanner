// Native tests: phát hiện lia/lắc máy từ gyro (hysteresis + giữ cờ). Run: pio test -e native
#include <unity.h>
#include "shake_detect.h"

void setUp() {}
void tearDown() {}

void test_mag_from_raw() {
  // ±250 dps → 32768/250 LSB per dps
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 30.0f, shake_mag_dps(3932, 0, 0, SHAKE_LSB_PER_DPS_250));
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, shake_mag_dps(-3932, 5243, 0, SHAKE_LSB_PER_DPS_250));  // 3-4-5
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, shake_mag_dps(0, 0, 0, SHAKE_LSB_PER_DPS_250));
}

void test_still_stays_off() {
  ShakeDetector d;
  for (uint32_t t = 0; t < 2000; t += 10) TEST_ASSERT_FALSE(d.update(5.0f, t));  // nhiễu cầm tay nhẹ
  TEST_ASSERT_FALSE(d.update(29.0f, 2000));                                     // dưới ngưỡng bật
}

void test_on_then_hysteresis_then_hold() {
  ShakeDetector d;  // on 30, off 15, hold 300ms
  TEST_ASSERT_TRUE(d.update(40.0f, 1000));
  TEST_ASSERT_TRUE(d.update(20.0f, 1100));   // giữa off..on: vẫn lắc, làm mới mốc
  TEST_ASSERT_TRUE(d.update(5.0f, 1200));    // đã dưới off nhưng chưa đủ 300ms từ 1100
  TEST_ASSERT_TRUE(d.update(5.0f, 1399));
  TEST_ASSERT_FALSE(d.update(5.0f, 1400));   // 300ms sau lần cuối >= off
  TEST_ASSERT_FALSE(d.update(20.0f, 1500));  // đang yên: 20 < on → không bật lại
}

void test_millis_wraparound() {
  ShakeDetector d;
  TEST_ASSERT_TRUE(d.update(40.0f, 0xFFFFFF00u));
  TEST_ASSERT_TRUE(d.update(1.0f, 0x00000010u));   // mới 0x110 = 272ms
  TEST_ASSERT_FALSE(d.update(1.0f, 0x00000040u));  // 0x140 = 320ms
}

void test_reset_clears() {
  ShakeDetector d;
  d.update(40.0f, 0);
  d.reset();
  TEST_ASSERT_FALSE(d.shaking);
  TEST_ASSERT_FALSE(d.update(1.0f, 10));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_mag_from_raw);
  RUN_TEST(test_still_stays_off);
  RUN_TEST(test_on_then_hysteresis_then_hold);
  RUN_TEST(test_millis_wraparound);
  RUN_TEST(test_reset_clears);
  return UNITY_END();
}
