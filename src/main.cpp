// LD2450 → ESP32-S3 → WiFi AP → trang radar trên điện thoại (http://192.168.4.1).
// Đồng thời in CSV mục tiêu ra USB serial. Màn hình gắn liền (Waveshare): bước sau.
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "ld2450_cmd.h"
#include "ld2450_filter.h"
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
static Ld2450AckParser ack;
static Ld2450Frame shown;  // frame sau lọc, dùng cho web + CSV
static WebServer server(80);
static uint32_t lastFrameMs = 0;
static bool clusterOn = false;  // mặc định TẮT: gộp làm sai thống kê slot + mục tiêu là bắt người đứng yên

// ── Cấu hình radar lúc boot: bật cfg → hỏi chế độ → (nếu single) chuyển multi → kết thúc cfg ──
enum CfgStep { CFG_WAIT_BOOT, CFG_ENABLE, CFG_QUERY, CFG_MULTI, CFG_END, CFG_DONE, CFG_FAIL };
static CfgStep cfgStep = CFG_WAIT_BOOT;
static uint32_t cfgSentMs = 0;
static uint8_t cfgTries = 0;
static uint16_t radarMode = 0;  // 1 single, 2 multi, 0 chưa biết
static const uint32_t CFG_BOOT_DELAY_MS = 1500, CFG_TIMEOUT_MS = 600;
static const uint8_t CFG_MAX_TRIES = 3;

static void sendCmd(uint16_t word) {
  static const uint8_t ONE[2] = {0x01, 0x00};
  uint8_t out[16];
  size_t n = ld2450_build_cmd(word, word == LD2450_CMD_ENABLE_CFG ? ONE : nullptr,
                              word == LD2450_CMD_ENABLE_CFG ? 2 : 0, out);
  Serial1.write(out, n);
  cfgSentMs = millis();
}

static uint16_t stepCmd(CfgStep s) {
  switch (s) {
    case CFG_ENABLE: return LD2450_CMD_ENABLE_CFG;
    case CFG_QUERY: return LD2450_CMD_QUERY_MODE;
    case CFG_MULTI: return LD2450_CMD_MULTI;
    default: return LD2450_CMD_END_CFG;
  }
}

static void cfgGoto(CfgStep s) {
  cfgStep = s;
  cfgTries = 0;
  if (s != CFG_DONE && s != CFG_FAIL) sendCmd(stepCmd(s));
}

static void cfgOnAck() {
  if (cfgStep == CFG_DONE || cfgStep == CFG_FAIL || ack.cmd() != stepCmd(cfgStep)) return;
  if (!ack.ok()) { Serial.printf("# radar cfg: lệnh 0x%04X bị từ chối\n", ack.cmd()); cfgGoto(CFG_END); return; }
  switch (cfgStep) {
    case CFG_ENABLE: cfgGoto(CFG_QUERY); break;
    case CFG_QUERY:
      radarMode = ack.value16();
      Serial.printf("# radar mode hiện tại: %s\n", radarMode == 2 ? "multi" : radarMode == 1 ? "single" : "?");
      cfgGoto(radarMode == 2 ? CFG_END : CFG_MULTI);
      break;
    case CFG_MULTI: radarMode = 2; Serial.println("# radar đã chuyển sang multi-target"); cfgGoto(CFG_END); break;
    case CFG_END: cfgStep = CFG_DONE; Serial.println("# radar cfg xong"); break;
    default: break;
  }
}

static void cfgTick() {
  uint32_t now = millis();
  if (cfgStep == CFG_WAIT_BOOT) { if (now > CFG_BOOT_DELAY_MS) cfgGoto(CFG_ENABLE); return; }
  if (cfgStep == CFG_FAIL) {
    // END không được ACK: nếu radar im (có thể kẹt ở chế độ cfg) thì cứ 5s gửi lại END
    if (now - lastFrameMs > STALE_MS && now - cfgSentMs > 5000) sendCmd(LD2450_CMD_END_CFG);
    return;
  }
  if (cfgStep == CFG_DONE || now - cfgSentMs < CFG_TIMEOUT_MS) return;
  if (++cfgTries >= CFG_MAX_TRIES) {
    Serial.printf("# radar cfg: không nhận ACK cho 0x%04X, bỏ qua (radar vẫn chạy cấu hình gốc)\n", stepCmd(cfgStep));
    if (cfgStep == CFG_END) { cfgStep = CFG_FAIL; return; }
    cfgGoto(CFG_END);  // luôn cố thoát chế độ cfg để radar tiếp tục gửi dữ liệu
    return;
  }
  sendCmd(stepCmd(cfgStep));
}

static void handleData() {
  bool stale = millis() - lastFrameMs > STALE_MS;
  char buf[240];
  int n = snprintf(buf, sizeof(buf), "{\"n\":%lu,\"stale\":%s,\"mode\":%u,\"cluster\":%s,\"t\":[",
                   (unsigned long)parser.frames(), stale ? "true" : "false", radarMode, clusterOn ? "true" : "false");
  bool first = true;
  for (int i = 0; i < 3 && !stale; i++) {
    const Ld2450Target &t = shown.t[i];
    if (!t.valid) continue;
    n += snprintf(buf + n, sizeof(buf) - n, "%s[%d,%d,%d,%d]", first ? "" : ",", i + 1, t.x, t.y, t.speed);
    first = false;
  }
  snprintf(buf + n, sizeof(buf) - n, "]}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buf);
}

static void handleCfg() {
  if (server.hasArg("cluster")) clusterOn = server.arg("cluster") == "1";
  server.send(200, "application/json", clusterOn ? "{\"cluster\":true}" : "{\"cluster\":false}");
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);  // cắm PC mà không mở terminal: không để mỗi lần in chờ 100ms làm kẹt loop
  Serial1.setRxBufferSize(1024);
  Serial1.begin(RADAR_BAUD, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/", [] { server.send_P(200, "text/html; charset=utf-8", WEB_PAGE); });
  server.on("/data", handleData);
  server.on("/cfg", handleCfg);
  server.begin();

  Serial.printf("# AP %s  IP %s  radar RX=%d TX=%d\n", AP_SSID, WiFi.softAPIP().toString().c_str(),
                RADAR_RX_PIN, RADAR_TX_PIN);
  Serial.println("ts_ms,id,x_mm,y_mm,speed_cms,dist_mm,angle_deg");
}

void loop() {
  while (Serial1.available()) {
    uint8_t b = (uint8_t)Serial1.read();
    if (ack.feed(b)) cfgOnAck();
    if (!parser.feed(b)) continue;
    lastFrameMs = millis();
    shown = parser.frame();
    ld2450_filter(shown, clusterOn);
    for (int i = 0; i < 3; i++) {
      const Ld2450Target &t = shown.t[i];
      if (!t.valid) continue;
      Serial.printf("%lu,%d,%d,%d,%d,%u,%.1f\n", (unsigned long)millis(), i + 1, t.x, t.y, t.speed,
                    t.distanceMm(), t.angleDeg());
    }
  }
  cfgTick();
  server.handleClient();
}
