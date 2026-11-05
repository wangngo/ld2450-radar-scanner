// Native tests for the LD2450 streaming parser. Run: pio test -e native
#include <unity.h>
#include "ld2450_parser.h"

// Example frame from the HLK-LD2450 protocol doc (as quoted in ld2450-proto crate):
// target1 x=-782mm y=1713mm speed=-16cm/s resolution=320mm, targets 2/3 empty.
static const uint8_t DOC_FRAME[30] = {
    0xAA, 0xFF, 0x03, 0x00,
    0x0E, 0x03, 0xB1, 0x86, 0x10, 0x00, 0x40, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x55, 0xCC};

static int feedAll(Ld2450Parser &p, const uint8_t *d, size_t n) {
  int frames = 0;
  for (size_t i = 0; i < n; i++) frames += p.feed(d[i]) ? 1 : 0;
  return frames;
}

void setUp() {}
void tearDown() {}

void test_decode_sign() {
  TEST_ASSERT_EQUAL_INT16(782, ld2450_decode(0x830E));
  TEST_ASSERT_EQUAL_INT16(-782, ld2450_decode(0x030E));
  TEST_ASSERT_EQUAL_INT16(0, ld2450_decode(0x0000));
}

void test_doc_frame() {
  Ld2450Parser p;
  TEST_ASSERT_EQUAL(1, feedAll(p, DOC_FRAME, 30));
  const Ld2450Frame &f = p.frame();
  TEST_ASSERT_TRUE(f.t[0].valid);
  TEST_ASSERT_EQUAL_INT16(-782, f.t[0].x);
  TEST_ASSERT_EQUAL_INT16(1713, f.t[0].y);
  TEST_ASSERT_EQUAL_INT16(-16, f.t[0].speed);
  TEST_ASSERT_EQUAL_UINT16(320, f.t[0].resolution);
  TEST_ASSERT_FALSE(f.t[1].valid);
  TEST_ASSERT_FALSE(f.t[2].valid);
  TEST_ASSERT_EQUAL(1, f.count());
  // distance computed from x,y (resolution is NOT distance)
  TEST_ASSERT_UINT16_WITHIN(1, 1883, f.t[0].distanceMm());
}

void test_garbage_and_split_frames() {
  Ld2450Parser p;
  const uint8_t junk[] = {0xFF, 0xAA, 0x00, 0x55, 0xAA, 0xFF, 0x13};
  TEST_ASSERT_EQUAL(0, feedAll(p, junk, sizeof(junk)));
  // first half, then second half (simulates UART read boundaries)
  TEST_ASSERT_EQUAL(0, feedAll(p, DOC_FRAME, 13));
  TEST_ASSERT_EQUAL(1, feedAll(p, DOC_FRAME + 13, 17));
  TEST_ASSERT_EQUAL(2, feedAll(p, DOC_FRAME, 30) + feedAll(p, DOC_FRAME, 30));
}

void test_bad_footer_rejected() {
  Ld2450Parser p;
  uint8_t bad[30];
  for (int i = 0; i < 30; i++) bad[i] = DOC_FRAME[i];
  bad[29] = 0x00;
  TEST_ASSERT_EQUAL(0, feedAll(p, bad, 30));
  // parser recovers on the next good frame
  TEST_ASSERT_EQUAL(1, feedAll(p, DOC_FRAME, 30));
}

void test_header_inside_payload_resyncs() {
  // A frame truncated after the header, immediately followed by a full frame
  Ld2450Parser p;
  TEST_ASSERT_EQUAL(0, feedAll(p, DOC_FRAME, 8));
  TEST_ASSERT_EQUAL(1, feedAll(p, DOC_FRAME, 30));
}

void test_three_targets_positive_x() {
  uint8_t f[30];
  for (int i = 0; i < 30; i++) f[i] = DOC_FRAME[i];
  // target2: x=+500 (0x81F4), y=+2000 (0x87D0), speed=+30 (0x801E), res=360
  const uint8_t t2[8] = {0xF4, 0x81, 0xD0, 0x87, 0x1E, 0x80, 0x68, 0x01};
  // target3: x=0, y=+600 (0x8258), speed=0, res=360
  const uint8_t t3[8] = {0x00, 0x80, 0x58, 0x82, 0x00, 0x00, 0x68, 0x01};
  for (int i = 0; i < 8; i++) { f[12 + i] = t2[i]; f[20 + i] = t3[i]; }
  Ld2450Parser p;
  TEST_ASSERT_EQUAL(1, feedAll(p, f, 30));
  TEST_ASSERT_EQUAL(3, p.frame().count());
  TEST_ASSERT_EQUAL_INT16(500, p.frame().t[1].x);
  TEST_ASSERT_EQUAL_INT16(2000, p.frame().t[1].y);
  TEST_ASSERT_EQUAL_INT16(30, p.frame().t[1].speed);
  TEST_ASSERT_EQUAL_INT16(0, p.frame().t[2].x);
  TEST_ASSERT_EQUAL_INT16(600, p.frame().t[2].y);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_decode_sign);
  RUN_TEST(test_doc_frame);
  RUN_TEST(test_garbage_and_split_frames);
  RUN_TEST(test_bad_footer_rejected);
  RUN_TEST(test_header_inside_payload_resyncs);
  RUN_TEST(test_three_targets_positive_x);
  return UNITY_END();
}
