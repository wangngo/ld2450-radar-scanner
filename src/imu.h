// BMI160 (CJMCU-160) qua I2C bằng driver chính hãng Bosch (lib/bmi160, BSD-3).
// Nối dây: SDA=GPIO8, SCL=GPIO9, CS→3V3 (ép I2C), SAO→GND (addr 0x68).
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "bmi160.h"
#include "shake_detect.h"

#ifndef IMU_SDA_PIN
#define IMU_SDA_PIN 8
#endif
#ifndef IMU_SCL_PIN
#define IMU_SCL_PIN 9
#endif

static struct bmi160_dev imuDev;
static bool imuOk = false;
static ShakeDetector shake;
static float imuDps = 0;            // |ω| lần đọc gần nhất
static uint32_t imuLastMs = 0;
static uint8_t imuFails = 0;
static const uint32_t IMU_PERIOD_MS = 10;  // 100 Hz, khớp ODR gyro

static int8_t imuRead(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return BMI160_E_COM_FAIL;
  if (Wire.requestFrom((int)addr, (int)len) != len) return BMI160_E_COM_FAIL;
  for (uint16_t i = 0; i < len; i++) data[i] = (uint8_t)Wire.read();
  return BMI160_OK;
}

static int8_t imuWrite(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(data, len);
  return Wire.endTransmission() == 0 ? BMI160_OK : BMI160_E_COM_FAIL;
}

static void imuDelay(uint32_t ms) { delay(ms); }

static void imuBegin() {
  Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN, 400000);
  imuDev.id = BMI160_I2C_ADDR;  // 0x68 (SAO→GND)
  imuDev.intf = BMI160_I2C_INTF;
  imuDev.read = imuRead;
  imuDev.write = imuWrite;
  imuDev.delay_ms = imuDelay;
  imuDev.read_write_len = 32;
  int8_t r = bmi160_init(&imuDev);  // đọc reg 0x00, phải = 0xD1
  if (r != BMI160_OK) {
    Serial.printf("# bmi160: không thấy chip (rslt %d, chip_id 0x%02X) — chạy tiếp không có gyro\n", r, imuDev.chip_id);
    return;
  }
  imuDev.gyro_cfg.odr = BMI160_GYRO_ODR_100HZ;
  imuDev.gyro_cfg.range = BMI160_GYRO_RANGE_250_DPS;
  imuDev.gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;
  imuDev.gyro_cfg.power = BMI160_GYRO_NORMAL_MODE;
  imuDev.accel_cfg.power = BMI160_ACCEL_SUSPEND_MODE;  // chỉ cần gyro
  r = bmi160_set_sens_conf(&imuDev);
  imuOk = r == BMI160_OK;
  Serial.printf("# bmi160 chip 0x%02X %s\n", imuDev.chip_id, imuOk ? "ok, gyro 100Hz ±250dps" : "cấu hình lỗi");
}

// Gọi mỗi vòng loop; tự giới hạn 100 Hz. Mất chip 10 lần liền → tắt gyro, bỏ cờ lắc.
static void imuTick(uint32_t now) {
  if (!imuOk || now - imuLastMs < IMU_PERIOD_MS) return;
  imuLastMs = now;
  struct bmi160_sensor_data g;
  if (bmi160_get_sensor_data(BMI160_GYRO_SEL, nullptr, &g, &imuDev) != BMI160_OK) {
    if (++imuFails >= 10) {
      imuOk = false;
      imuDps = 0;
      shake.reset();
      Serial.println("# bmi160: mất kết nối I2C, tắt phát hiện lắc");
    }
    return;
  }
  imuFails = 0;
  imuDps = shake_mag_dps(g.x, g.y, g.z, SHAKE_LSB_PER_DPS_250);
  shake.update(imuDps, now);
}
