// Streaming byte-by-byte parser for HLK-LD2450 target frames (30 bytes):
//   AA FF 03 00 | 3 x [x(2) y(2) speed(2) resolution(2)] little-endian | 55 CC
// Signed fields use bit15 as sign flag: 1 = positive, 0 = negative (magnitude in bits 0-14).
// Units: x,y mm; speed cm/s; resolution = distance-gate resolution mm (NOT distance).
// No Arduino dependency -> testable natively.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <math.h>

inline int16_t ld2450_decode(uint16_t raw) {
  int16_t mag = (int16_t)(raw & 0x7FFF);
  return (raw & 0x8000) ? mag : (int16_t)-mag;
}

struct Ld2450Target {
  int16_t x = 0, y = 0, speed = 0;
  uint16_t resolution = 0;
  bool valid = false;
  uint16_t distanceMm() const { return (uint16_t)lroundf(hypotf((float)x, (float)y)); }
  // degrees, 0 = straight ahead, negative = left
  float angleDeg() const { return atan2f((float)x, (float)y) * 57.29578f; }
};

struct Ld2450Frame {
  Ld2450Target t[3];
  int count() const { return t[0].valid + t[1].valid + t[2].valid; }
};

class Ld2450Parser {
 public:
  static const size_t FRAME_LEN = 30;

  // Feed one byte; returns true when a complete valid frame was just decoded.
  bool feed(uint8_t b) {
    static const uint8_t HDR[4] = {0xAA, 0xFF, 0x03, 0x00};
    if (len_ < 4) {
      if (b == HDR[len_]) {
        buf_[len_++] = b;
      } else {
        len_ = (b == HDR[0]) ? 1 : 0;
        if (len_) buf_[0] = b;
      }
      return false;
    }
    buf_[len_++] = b;
    if (len_ < FRAME_LEN) {
      // resync: a fresh header appearing mid-payload means the previous frame was truncated
      if (len_ >= 8 && buf_[len_ - 4] == 0xAA && buf_[len_ - 3] == 0xFF &&
          buf_[len_ - 2] == 0x03 && buf_[len_ - 1] == 0x00) {
        len_ = 4;
      }
      return false;
    }
    len_ = 0;
    if (buf_[28] != 0x55 || buf_[29] != 0xCC) return false;
    for (int i = 0; i < 3; i++) {
      const uint8_t *p = buf_ + 4 + i * 8;
      Ld2450Target &t = frame_.t[i];
      uint16_t rx = p[0] | (p[1] << 8), ry = p[2] | (p[3] << 8), rs = p[4] | (p[5] << 8);
      t.resolution = p[6] | (p[7] << 8);
      t.x = ld2450_decode(rx);
      t.y = ld2450_decode(ry);
      t.speed = ld2450_decode(rs);
      t.valid = (rx | ry | rs | t.resolution) != 0;
    }
    ++frames_;
    return true;
  }

  const Ld2450Frame &frame() const { return frame_; }
  uint32_t frames() const { return frames_; }

 private:
  uint8_t buf_[FRAME_LEN] = {};
  size_t len_ = 0;
  Ld2450Frame frame_;
  uint32_t frames_ = 0;
};
