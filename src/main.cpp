// LD2450 → ESP32-S3 → WiFi AP → trang radar trên điện thoại (http://192.168.4.1).
// Đồng thời in CSV mục tiêu ra USB serial. Màn hình gắn liền (Waveshare): bước sau.
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "ld2450_parser.h"
#include "web_page.h"

#ifndef RADAR_RX_PIN
#define RADAR_RX_PIN 18  // nối với chân TX của LD2450
#endif
#ifndef RADAR_TX_PIN
#define RADAR_TX_PIN 17  // nối với chân RX của LD2450
#endif
#ifndef AP_SSID
#define AP_SSID "LD2450-Radar"
#endif
#ifndef AP_PASS
#define AP_PASS "radar2450"
#endif

static const uint32_t RADAR_BAUD = 256000;
static const uint32_t STALE_MS = 1000;

static Ld2450Parser parser;
static WebServer server(80);
static uint32_t lastFrameMs = 0;

static void handleData() {
  const Ld2450Frame &f = parser.frame();
  bool stale = millis() - lastFrameMs > STALE_MS;
  char buf[200];
  int n = snprintf(buf, sizeof(buf), "{\"n\":%lu,\"stale\":%s,\"t\":[", (unsigned long)parser.frames(),
                   stale ? "true" : "false");
  bool first = true;
  for (int i = 0; i < 3 && !stale; i++) {
    const Ld2450Target &t = f.t[i];
    if (!t.valid) continue;
    n += snprintf(buf + n, sizeof(buf) - n, "%s[%d,%d,%d,%d]", first ? "" : ",", i + 1, t.x, t.y, t.speed);
    first = false;
  }
  snprintf(buf + n, sizeof(buf) - n, "]}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buf);
}

void setup() {
  Serial.begin(115200);
  Serial1.setRxBufferSize(1024);
  Serial1.begin(RADAR_BAUD, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/", [] { server.send_P(200, "text/html; charset=utf-8", WEB_PAGE); });
  server.on("/data", handleData);
  server.begin();

  Serial.printf("# AP %s  IP %s  radar RX=%d TX=%d\n", AP_SSID, WiFi.softAPIP().toString().c_str(),
                RADAR_RX_PIN, RADAR_TX_PIN);
  Serial.println("ts_ms,id,x_mm,y_mm,speed_cms,dist_mm,angle_deg");
}

void loop() {
  while (Serial1.available()) {
    if (!parser.feed((uint8_t)Serial1.read())) continue;
    lastFrameMs = millis();
    const Ld2450Frame &f = parser.frame();
    for (int i = 0; i < 3; i++) {
      const Ld2450Target &t = f.t[i];
      if (!t.valid) continue;
      Serial.printf("%lu,%d,%d,%d,%d,%u,%.1f\n", (unsigned long)millis(), i + 1, t.x, t.y, t.speed,
                    t.distanceMm(), t.angleDeg());
    }
  }
  server.handleClient();
}
