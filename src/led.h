// Cảnh báo bằng LED RGB on-board (WS2812, GPIO48 — verify LIVE 2026-11-07, board này sáng ĐỎ ở 48).
// Màu theo mục tiêu gần nhất: xa = xanh lá, giữa = vàng, gần = đỏ; càng gần nháy càng nhanh.
// Đang lắc máy (gyro) = xanh dương mờ, tạm ngưng cảnh báo (số liệu không tin cậy).
#pragma once
#include <Arduino.h>

#ifndef RGB_PIN
#define RGB_PIN 48
#endif
#ifndef LED_MAX_MM
#define LED_MAX_MM 6000          // ngoài tầm này coi như không có ai → tắt
#endif
#ifndef LED_SOLID_MM
#define LED_SOLID_MM 4000        // xa hơn mức này: sáng liên tục (không nháy)
#endif

static uint8_t ledBrightness = 40;         // trần độ sáng 0..255 (để đỡ chói)
static bool    ledPhase = false;           // pha nháy on/off
static uint32_t ledNextMs = 0;
static int16_t ledLastR = -1, ledLastG = -1, ledLastB = -1;

static inline uint8_t ledScale(uint8_t c) { return (uint16_t)c * ledBrightness / 255; }

// Chỉ ghi ra WS2812 khi màu đổi (tránh spam RMT mỗi vòng loop → đỡ nhấp nháy, đỡ tốn CPU).
static void ledSet(uint8_t r, uint8_t g, uint8_t b) {
  r = ledScale(r); g = ledScale(g); b = ledScale(b);
  if (r == ledLastR && g == ledLastG && b == ledLastB) return;
  ledLastR = r; ledLastG = g; ledLastB = b;
  neopixelWrite(RGB_PIN, r, g, b);
}

// mm -> màu (gần=đỏ, giữa=vàng, xa=xanh lá). Chưa nhân độ sáng.
static void ledColorFor(uint16_t mm, uint8_t &r, uint8_t &g, uint8_t &b) {
  float f = mm >= LED_MAX_MM ? 1.0f : (float)mm / LED_MAX_MM;   // 0 gần .. 1 xa
  b = 0;
  if (f < 0.5f) { r = 255; g = (uint8_t)(255 * (f / 0.5f)); }               // đỏ → vàng
  else          { r = (uint8_t)(255 * (1.0f - (f - 0.5f) / 0.5f)); g = 255; } // vàng → xanh lá
}

// Gọi mỗi vòng loop. nearestMm = 0xFFFF nếu không có mục tiêu hợp lệ.
static void ledTick(uint32_t now, uint16_t nearestMm, bool stale, bool shaking) {
  if (shaking) { ledSet(0, 0, 60); return; }                     // đang lắc → xanh dương mờ
  if (stale || nearestMm == 0xFFFF || nearestMm > LED_MAX_MM) {  // không có ai → tắt
    ledSet(0, 0, 0); return;
  }
  uint8_t r, g, b;
  ledColorFor(nearestMm, r, g, b);
  if (nearestMm > LED_SOLID_MM) { ledSet(r, g, b); return; }      // xa → sáng liên tục

  // Gần → nháy, chu kỳ theo khoảng cách: 300mm→~120ms (dồn), 4000mm→~900ms (thưa).
  long p = map((long)nearestMm, 300, LED_SOLID_MM, 120, 900);
  uint32_t half = (uint32_t)constrain(p, 120L, 900L) / 2;
  if (now >= ledNextMs) { ledPhase = !ledPhase; ledNextMs = now + half; }
  if (ledPhase) ledSet(r, g, b);
  else          ledSet(0, 0, 0);
}
