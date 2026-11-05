// Native tests: command builder, ACK parser, post-filter. Run: pio test -e native
#include <string.h>
#include <unity.h>
#include "ld2450_cmd.h"
#include "ld2450_filter.h"

void setUp() {}
void tearDown() {}

// Byte vectors cross-checked with ld2450-proto 0.2.0 and Stevee87 RD-03D sender (CMD_ENABLE/CMD_MULTI/CMD_END)
void test_build_enable() {
  uint8_t out[32];
  const uint8_t val[2] = {0x01, 0x00};
  const uint8_t exp[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
  TEST_ASSERT_EQUAL(sizeof(exp), ld2450_build_cmd(LD2450_CMD_ENABLE_CFG, val, 2, out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

void test_build_multi_and_end() {
  uint8_t out[32];
  const uint8_t m[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x90, 0x00, 0x04, 0x03, 0x02, 0x01};
  const uint8_t e[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
  TEST_ASSERT_EQUAL(12, ld2450_build_cmd(LD2450_CMD_MULTI, nullptr, 0, out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(m, out, 12);
  TEST_ASSERT_EQUAL(12, ld2450_build_cmd(LD2450_CMD_END_CFG, nullptr, 0, out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(e, out, 12);
}

static bool feed(Ld2450AckParser &p, const uint8_t *d, size_t n) {
  bool got = false;
  for (size_t i = 0; i < n; i++) got |= p.feed(d[i]);
  return got;
}

void test_ack_query_mode_single() {
  // from ld2450-proto test parse_ack_tracking_mode_single, with junk prefix
  const uint8_t a[] = {0x55, 0xCC, 0xFD, 0xFC, 0xFB, 0xFA, 0x06, 0x00, 0x91, 0x01, 0x00, 0x00, 0x01, 0x00,
                       0x04, 0x03, 0x02, 0x01};
  Ld2450AckParser p;
  TEST_ASSERT_TRUE(feed(p, a, sizeof(a)));
  TEST_ASSERT_EQUAL_HEX16(0x0091, p.cmd());
  TEST_ASSERT_TRUE(p.ok());
  TEST_ASSERT_EQUAL(1, p.value16());
}

void test_ack_end_cfg_and_bad_footer() {
  const uint8_t a[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFE, 0x01, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
  Ld2450AckParser p;
  TEST_ASSERT_TRUE(feed(p, a, sizeof(a)));
  TEST_ASSERT_EQUAL_HEX16(0x00FE, p.cmd());
  uint8_t bad[sizeof(a)];
  memcpy(bad, a, sizeof(a));
  bad[sizeof(a) - 1] = 0x00;
  TEST_ASSERT_FALSE(feed(p, bad, sizeof(bad)));
  TEST_ASSERT_TRUE(feed(p, a, sizeof(a)));  // recovers
}

static Ld2450Target T(int16_t x, int16_t y, int16_t s = 0) {
  Ld2450Target t;
  t.x = x; t.y = y; t.speed = s; t.resolution = 320; t.valid = true;
  return t;
}

void test_filter_range() {
  Ld2450Frame f;
  f.t[0] = T(0, 50);     // 5 cm: too close
  f.t[1] = T(0, 6500);   // 6.5 m: beyond range
  f.t[2] = T(300, 2000); // ok
  ld2450_filter(f, false);
  TEST_ASSERT_FALSE(f.t[0].valid);
  TEST_ASSERT_FALSE(f.t[1].valid);
  TEST_ASSERT_TRUE(f.t[2].valid);
  Ld2450Frame g;
  g.t[0] = T(500, -300);  // phía sau radar
  ld2450_filter(g, false);
  TEST_ASSERT_FALSE(g.t[0].valid);
}

void test_filter_cluster_on_off() {
  Ld2450Frame f;
  f.t[0] = T(0, 2000, -10);
  f.t[1] = T(400, 2300, -50);  // 0.5 m away: same person (arm)
  f.t[2] = T(-2000, 2000);     // far away: other person
  Ld2450Frame g = f;
  ld2450_filter(f, true);
  TEST_ASSERT_TRUE(f.t[0].valid);
  TEST_ASSERT_FALSE(f.t[1].valid);
  TEST_ASSERT_TRUE(f.t[2].valid);
  TEST_ASSERT_EQUAL_INT16(200, f.t[0].x);
  TEST_ASSERT_EQUAL_INT16(2150, f.t[0].y);
  TEST_ASSERT_EQUAL_INT16(-50, f.t[0].speed);  // keep larger magnitude
  ld2450_filter(g, false);
  TEST_ASSERT_EQUAL(3, g.count());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_build_enable);
  RUN_TEST(test_build_multi_and_end);
  RUN_TEST(test_ack_query_mode_single);
  RUN_TEST(test_ack_end_cfg_and_bad_footer);
  RUN_TEST(test_filter_range);
  RUN_TEST(test_filter_cluster_on_off);
  return UNITY_END();
}
