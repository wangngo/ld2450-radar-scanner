# CHANGELOG

## 2026-11-06
- Định danh gyro: board tím = **BMI160 (CJMCU-160)**, neo theo nhãn chân mặt sau (OCS/INT2/INT1/SCX/SDX + VIN/3V3/GND/SCL/SDA/CS/SAO). Chưa đọc được mã laser chip → verify cuối = I2C scan + Chip ID reg 0x00 = 0xD1.
- `docs/wiring.html`: thêm mục "Bố trí vật lý — radar & gyro" (mặt cắt 4 lớp: anten ra trước → đế cứng radar+gyro → tấm chắn kim loại → ESP32+pin+tay). Cập nhật bảng gyro: thêm **CS→3V3** (ép I2C, CS thấp=SPI) + **SAO→GND** (addr 0x68) + mục quét I2C trước khi hàn thêm.
- `CLAUDE.md`: điền định danh BMI160 + gotcha CS/SAO; note N16R8 có PSRAM octal chiếm GPIO33–37 (cấm dùng).
- Chốt board chính: giữ ESP32-S3-N16R8 (W quyết, không chuyển NodeMCU). Pin s3_web (8/9/17/18) đều ngoài vùng PSRAM + strapping → an toàn.
- Khởi động track KiCad: cài KiCad 10.0.8 (cask), kế hoạch dựng schematic + PCB carrier và tiến tới KiCad MCP (fork tool có sẵn) — đang chờ advisor chốt MCP nào.

## 2026-11-05
- Sửa theo review advisor: gộp <1m mặc định TẮT; fetch timeout 1s + xoá mục tiêu khi mất kết nối (tránh báo người giả);
  Hz tính từ số frame radar; `Serial.setTxTimeoutMs(0)`; END cfg gửi lại mỗi 5s khi radar im; lọc y<=0;
  ghim `espressif32@6.9.0` (= Arduino core 2.0.17, docs cũ ghi nhầm 3.x).
- Lúc boot: hỏi chế độ radar (0x0091), chuyển multi-target nếu cần; lọc 0,1–6m; gộp mục tiêu <1m (bật/tắt trên web qua `/cfg`). Test native 12/12.
- Web: thống kê bám liên tục/mất dấu, vòng mờ khi mất dấu, làm mượt EMA.
- Bản web: ESP32-S3 phát WiFi AP, trang radar trên điện thoại (poll /data 10Hz), env `s3_web` RX=18/TX=17.
- Thêm vào plan: bíp theo khoảng cách, chọn tầm 3/4/6m, v2 cảm biến đặt xa qua ESP-NOW (tham khảo dự án LD2450+LoRa+Pi).
- Khởi tạo repo: parser LD2450 + test native, firmware v0 in CSV, docs/PLAN.md.
