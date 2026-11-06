// Phát hiện lia/lắc máy từ tốc độ quay gyro — manual LD2450: radar rung lắc thì đo sai.
// Hysteresis: bật khi |ω| >= onDps; khi đang lắc, |ω| >= offDps vẫn tính là lắc;
// tắt khi |ω| < offDps liên tục holdMs (chờ radar ổn định lại). Ngưỡng là điểm khởi đầu, chỉnh thực địa.
#pragma once
#include <math.h>
#include <stdint.h>

// ±250 dps trên số 16-bit có dấu → 32768/250 LSB mỗi °/s (suy từ full-scale; datasheet ghi danh định 131.2)
static const float SHAKE_LSB_PER_DPS_250 = 32768.0f / 250.0f;

inline float shake_mag_dps(int16_t x, int16_t y, int16_t z, float lsbPerDps) {
  float fx = x / lsbPerDps, fy = y / lsbPerDps, fz = z / lsbPerDps;
  return sqrtf(fx * fx + fy * fy + fz * fz);
}

struct ShakeDetector {
  float onDps = 30.0f;
  float offDps = 15.0f;
  uint32_t holdMs = 300;
  bool shaking = false;
  uint32_t lastHiMs = 0;

  bool update(float dps, uint32_t nowMs) {
    if (dps >= onDps || (shaking && dps >= offDps)) {
      shaking = true;
      lastHiMs = nowMs;
    } else if (shaking && (uint32_t)(nowMs - lastHiMs) >= holdMs) {
      shaking = false;
    }
    return shaking;
  }
  void reset() { shaking = false; }
};
