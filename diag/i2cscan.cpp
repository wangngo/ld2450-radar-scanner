// Chẩn đoán I2C cho BMI160 — log ra UART0 (CDC_ON_BOOT=0) để đọc qua cổng usbserial.
#include <Arduino.h>
#include <Wire.h>
#define SDA_PIN 8
#define SCL_PIN 9
static uint8_t rd(uint8_t addr, uint8_t reg){
  Wire.beginTransmission(addr); Wire.write(reg);
  if(Wire.endTransmission(false)!=0) return 0xFE;
  Wire.requestFrom((int)addr,1);
  return Wire.available()?Wire.read():0xFF;
}
void setup(){
  Serial.begin(115200); delay(800);
  Serial.println("\n# === I2C SCAN GPIO8(SDA)/GPIO9(SCL) ===");
  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  int found=0;
  for(uint8_t a=1;a<127;a++){
    Wire.beginTransmission(a);
    if(Wire.endTransmission()==0){ Serial.printf("# thiet bi tai 0x%02X\n",a); found++; }
  }
  if(!found) Serial.println("# KHONG thay thiet bi I2C nao (kiem SDA/SCL/nguon/CS)");
  // BMI160 chip_id reg 0x00 phai = 0xD1
  Serial.printf("# 0x68 reg0x00 = 0x%02X (SAO->GND)\n", rd(0x68,0x00));
  Serial.printf("# 0x69 reg0x00 = 0x%02X (SAO->3V3)\n", rd(0x69,0x00));
  Serial.println("# xong. 0xD1 = BMI160 OK.");
}
void loop(){ delay(2000); Serial.println("# (nhan RST neu muon quet lai)"); }
