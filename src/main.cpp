// v0: đọc LD2450 qua UART, in mục tiêu ra USB serial. Màn hình/cảnh báo/SD: bước sau khi có board.
#include <Arduino.h>
#include "ld2450_parser.h"

// Waveshare ESP32-S3-Touch-LCD-2.8 header SH1.0: TXD=GPIO43, RXD=GPIO44 (theo wiki, CHƯA verify trên board).
// Nối: LD2450 TX -> GPIO44 (RX của ESP32), LD2450 RX -> GPIO43.
static const int RADAR_RX_PIN = 44;
static const int RADAR_TX_PIN = 43;
static const uint32_t RADAR_BAUD = 256000;

static Ld2450Parser parser;

void setup() {
  Serial.begin(115200);
  Serial1.begin(RADAR_BAUD, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  Serial.println("ts_ms,id,x_mm,y_mm,speed_cms,dist_mm,angle_deg");
}

void loop() {
  while (Serial1.available()) {
    if (!parser.feed((uint8_t)Serial1.read())) continue;
    const Ld2450Frame &f = parser.frame();
    for (int i = 0; i < 3; i++) {
      const Ld2450Target &t = f.t[i];
      if (!t.valid) continue;
      Serial.printf("%lu,%d,%d,%d,%d,%u,%.1f\n", (unsigned long)millis(), i + 1, t.x, t.y,
                    t.speed, t.distanceMm(), t.angleDeg());
    }
  }
}
