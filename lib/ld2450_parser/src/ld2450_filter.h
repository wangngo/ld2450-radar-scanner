// Lọc sau parse: bỏ mục tiêu ngoài tầm, tuỳ chọn gộp 2 mục tiêu gần nhau (tay + thân cùng 1 người).
// Ý tưởng gộp < 1m lấy từ repo Stevee87 (RD-03D); giữ speed có độ lớn lớn hơn.
#pragma once
#include <stdlib.h>
#include "ld2450_parser.h"

static const float LD2450_MIN_MM = 100.0f;      // gần hơn 0,1 m: nhiễu
static const float LD2450_MAX_MM = 6000.0f;     // manual: tầm tối đa 6 m
static const float LD2450_CLUSTER_MM = 1000.0f; // khoảng cách gộp

inline void ld2450_filter(Ld2450Frame &f, bool cluster) {
  for (int i = 0; i < 3; i++) {
    Ld2450Target &t = f.t[i];
    if (!t.valid) continue;
    float d = hypotf((float)t.x, (float)t.y);
    if (d < LD2450_MIN_MM || d > LD2450_MAX_MM) t.valid = false;
  }
  if (!cluster) return;
  for (int i = 0; i < 3; i++) {
    Ld2450Target &a = f.t[i];
    if (!a.valid) continue;
    for (int j = i + 1; j < 3; j++) {
      Ld2450Target &b = f.t[j];
      if (!b.valid) continue;
      if (hypotf((float)(a.x - b.x), (float)(a.y - b.y)) >= LD2450_CLUSTER_MM) continue;
      a.x = (int16_t)((a.x + b.x) / 2);
      a.y = (int16_t)((a.y + b.y) / 2);
      if (abs(b.speed) > abs(a.speed)) a.speed = b.speed;
      b.valid = false;
    }
  }
}
