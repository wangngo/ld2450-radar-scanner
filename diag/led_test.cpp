// Test chân LED RGB on-board: nháy GPIO48 (ĐỎ) rồi GPIO38 (XANH DƯƠNG), lặp.
// W nhìn LED sáng màu gì ở pha nào → suy ra chân thật (v1.0=48, v1.1=38).
#include <Arduino.h>
void setup(){ Serial.begin(115200); delay(500); Serial.println("\n# LED test: 48=DO, 38=XANH"); }
void loop(){
  Serial.println("# GPIO48 -> DO (red)");
  neopixelWrite(48, 60, 0, 0);          // (pin, R, G, B)
  neopixelWrite(38, 0, 0, 0);
  delay(4000);
  Serial.println("# GPIO38 -> XANH DUONG (blue)");
  neopixelWrite(48, 0, 0, 0);
  neopixelWrite(38, 0, 0, 60);
  delay(4000);
}
