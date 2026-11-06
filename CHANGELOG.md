# CHANGELOG

## 2026-11-06
- **Schematic KiCad** `hardware/carrier/carrier.kicad_sch` (sinh bằng `hardware/gen_schematic.py`, tên chân module cạnh từng chân): **ERC 0 vi phạm**, PDF `carrier-schematic.pdf`. Thêm `carrier.kicad_pro` + `sym/fp-lib-table` dùng `${KICAD10_*}` (máy chưa có bảng lib toàn cục). DRC PCB vẫn 0/0.
- **Sơ đồ hàn board đục lỗ 7×9cm** `docs/perfboard.html` (sinh bằng `hardware/gen_perfboard.py`): mặt trên + mặt dưới (lật gương), 10 dây + 4 cầu thiếc, in 1:1. Script tự đối chiếu dây với netlist trích từ schematic (test âm: chập SDA/SCL và thiếu dây UART đều bị bắt). Advisor: không blocker; đã thêm đo 5V–J1.20 (cầu 5V sát GPIO14), rút DevKitC sau khi hàn socket, rút USB trước khi cắm radar.
- **Gyro BMI160 → phát hiện lia/lắc máy**: driver chính hãng Bosch (`lib/bmi160`, BSD-3, ghim commit 252ac28) + lớp nối Wire `src/imu.h` (SDA 8 / SCL 9, addr 0x68, gyro 100Hz ±250dps).
  `lib/shake_detect`: bật ≥30°/s, giữ ≥15°/s, tắt sau 300ms yên (ngưỡng khởi đầu, chỉnh thực địa). Test native 17/17.
  `/data` thêm `imu`/`shake`/`dps`; web **làm mờ** mục tiêu + dòng "ĐANG LẮC MÁY" khi lắc (cảnh báo vùng/bíp vẫn chạy); CSV serial + web thêm cột `shake`.
  Không thấy chip → log `# bmi160: không thấy chip`, firmware chạy tiếp không gyro. Build s3_web OK; kiểm web bằng mock + Chrome headless. **Chưa chạy trên chip thật.**
- **Carrier PCB v1** (`hardware/gen_carrier.py` → `hardware/carrier/`): sinh bằng pcbnew (KiCad 10.0.8), 66×66mm 2 lớp, GND pour 2 mặt.
  DevKitC-1 theo DXF Espressif (row-to-row 22.86 xác nhận, J1 trái/J3 phải, chân 1 phía antenna); BMI160 1x7+1x5 cách 10.16, hàng 5 nằm giữa (lệch 1 bước, W xác nhận).
  Sửa theo advisor: antenna WROOM thò 6.3mm khỏi mép DevKitC → **khoét notch mép trên**, 3V3 đi dưới shield luồn giữa J3.2/J3.3; thêm silk tên chân + ANT/USB; R1/R2 4k7 kéo I2C (DNP).
  DRC (kicad-cli): **0 vi phạm, 0 chưa nối**. Gerber+drill: `hardware/carrier/carrier-gerber.zip`. Chưa có schematic (netlist nằm trong script).
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
