// HLK-LD2450 command frames: FD FC FB FA | len(2 LE) | word(2 LE) | value... | 04 03 02 01
// ACK: same envelope, word = cmd | 0x0100, then status(2, 0 = ok), then extra data.
// Command words per ld2450-proto 0.2.0 (khớp byte lệnh repo Stevee87 cho RD-03D).
#pragma once
#include <stddef.h>
#include <stdint.h>

enum : uint16_t {
  LD2450_CMD_ENABLE_CFG = 0x00FF,  // value 0x0001
  LD2450_CMD_END_CFG = 0x00FE,
  LD2450_CMD_SINGLE = 0x0080,
  LD2450_CMD_MULTI = 0x0090,
  LD2450_CMD_QUERY_MODE = 0x0091,  // ACK extra: 1 = single, 2 = multi
};

inline size_t ld2450_build_cmd(uint16_t word, const uint8_t *val, size_t vlen, uint8_t *out) {
  static const uint8_t HDR[4] = {0xFD, 0xFC, 0xFB, 0xFA}, FTR[4] = {0x04, 0x03, 0x02, 0x01};
  size_t n = 0, len = 2 + vlen;
  for (int i = 0; i < 4; i++) out[n++] = HDR[i];
  out[n++] = len & 0xFF; out[n++] = len >> 8;
  out[n++] = word & 0xFF; out[n++] = word >> 8;
  for (size_t i = 0; i < vlen; i++) out[n++] = val[i];
  for (int i = 0; i < 4; i++) out[n++] = FTR[i];
  return n;
}

class Ld2450AckParser {
 public:
  // Feed one byte; true when a complete ACK frame was parsed.
  bool feed(uint8_t b) {
    static const uint8_t HDR[4] = {0xFD, 0xFC, 0xFB, 0xFA}, FTR[4] = {0x04, 0x03, 0x02, 0x01};
    if (n_ < 4) {
      if (b == HDR[n_]) buf_[n_++] = b;
      else { n_ = (b == HDR[0]); if (n_) buf_[0] = b; }
      return false;
    }
    buf_[n_++] = b;
    if (n_ == 6) {
      len_ = buf_[4] | (buf_[5] << 8);
      if (len_ < 4 || len_ + 10 > sizeof(buf_)) { n_ = 0; return false; }
    }
    if (n_ < 6 || n_ < (size_t)len_ + 10) return false;
    n_ = 0;
    for (int i = 0; i < 4; i++)
      if (buf_[6 + len_ + i] != FTR[i]) return false;
    cmd_ = (buf_[6] | (buf_[7] << 8)) & ~0x0100;
    status_ = buf_[8] | (buf_[9] << 8);
    value_ = len_ >= 6 ? (buf_[10] | (buf_[11] << 8)) : 0;
    return true;
  }
  uint16_t cmd() const { return cmd_; }
  bool ok() const { return status_ == 0; }
  uint16_t value16() const { return value_; }

 private:
  uint8_t buf_[48] = {};
  size_t n_ = 0;
  uint16_t len_ = 0, cmd_ = 0, status_ = 0, value_ = 0;
};
