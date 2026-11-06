# TASKS

## Xong
- [x] Parser LD2450 streaming + test native (6 case: frame mẫu datasheet, rác, frame bị cắt, footer sai, resync, 3 mục tiêu)
- [x] Firmware v0: UART 256000 → CSV qua USB serial (build OK, chưa chạy trên board)

- [x] Bản web trên điện thoại (env `s3_web`): WiFi AP + trang radar, cảnh báo, bíp theo khoảng cách, chọn tầm, log CSV (mock-test OK)

## Mở
- [x] Flash `s3_web` lên ESP32-S3 của W (2026-11-07): định danh LIVE OK (ESP32-S3 rev v0.2, 16MB, MAC `1c:db:d4:43:ef:78` OUI Espressif). Verify qua điện thoại: web mở được + chấm mục tiêu nhảy theo người đi. CÒN: đọc log serial qua cổng USB native để xác nhận `# radar cfg xong`/ACK + tần số ~10Hz (cách C — cắm cáp 2).
- [ ] W mua hàng: LD2450, Waveshare ESP32-S3-Touch-LCD-2.8 (240×320), LiPo 1S MX1.25, boost 5V (nếu cần), microSD ≤16GB
- [ ] Khi nhận board: định danh LIVE (esptool MAC+OUI), tra schematic: header có 5V không, có chân đo áp pin không, pin LCD/touch/SD/audio
- [ ] Chụp frame thật từ UART → thêm vào test; verify đơn vị mm + dấu speed bằng thước/đi lại gần
- [ ] Hiển thị: LovyanGFX, radar quạt 120° vòng 1/2/4/6m, chấm + vệt, bảng số liệu, thanh trạng thái (`src/main.cpp`)
- [ ] Cảnh báo: chạm đặt bán kính vùng, chớp đỏ + bíp qua chip audio
- [ ] Âm thanh theo khoảng cách: mục tiêu gần nhất càng gần → bíp càng dồn + cao (kiểu máy dò)
- [ ] Chọn tầm đo 3/4/6m (vòng lưới theo tầm); đo độ chính xác thực tế >4m trước khi chốt mặc định
- [ ] Log CSV lên SD
- [ ] Đo thời lượng pin, làm vỏ

- [ ] Checklist lần flash đầu (từ review advisor 2026-11-05):
  - Xem log qua cổng **"USB"** (native) của DevKitC, KHÔNG phải cổng "UART" (CDC_ON_BOOT=1)
  - Đặt radar trên giá cố định (tripod) trước, cầm tay sau
  - Chắn mặt sau radar bằng kim loại, test riêng xem người cầm có bị bắt thành mục tiêu không
  - Trang web phải hiện ~10 Hz "radar" (tính từ số frame), không phải tần số poll
  - Kiểm slot ID của LD2450 có giữ nguyên theo người không (cột "Bám liên tục" giả định vậy)
- [ ] Tìm tài liệu lệnh chính hãng Hi-Link ("LD2450 serial communication protocol") đối chiếu 0x00FF/0x0091/0x0090/0x00FE
- [ ] Viết test native cho máy trạng thái cấu hình (hiện chỉ nằm trong main.cpp)
- [ ] Verify trên radar thật: log serial phải có `# radar mode hiện tại` + `# radar cfg xong`; nếu ra 'không nhận ACK' thì LD2450 không hỗ trợ/khác lệnh → firmware vẫn chạy cấu hình gốc

## Học từ repo Stevee87 (RD-03D, cùng định dạng frame) — chờ W chọn
- [x] Gửi lệnh multi-target lúc boot: bật cấu hình 0x00FF → hỏi chế độ 0x0091 → nếu chưa multi thì 0x0090 → kết thúc 0x00FE (khớp crate ld2450-proto)
- [x] Gộp 2 mục tiêu cách nhau < ~0,6–1m (tay + thân cùng 1 người) — có nút bật/tắt vì 2 người đứng sát sẽ bị gộp nhầm
- [x] Bỏ mục tiêu < 0,1m hoặc > 6m (ngoài tầm theo manual)
- [ ] Kiểm chân 5V của DevKitC clone: một số board phải hàn cầu IN-OUT thì chân 5V mới có điện khi cắm USB

## Gyro BMI160 (code xong 2026-11-06, chưa chạy thật)
- [ ] Flash + xem log: phải có `# bmi160 chip 0xD1 ok` (định danh chip). Ra "không thấy chip" → I2C scan, kiểm CS→3V3/SAO→GND.
- [ ] Chỉnh ngưỡng lắc thực địa (`lib/shake_detect/src/shake_detect.h`: on 30 / off 15 °/s, hold 300ms) — cầm tay yên xem `dps` trên web, lia nhẹ xem cờ bật đúng lúc.
- [ ] Env `waveshare_s3_28` cũng gọi `imuBegin()` trên GPIO8/9 — khi có board Waveshare phải tra pin xem 8/9 có bị LCD/touch chiếm không.

## Board đục lỗ (sơ đồ xong 2026-11-06, `docs/perfboard.html`)
- [ ] Mua board 7×9cm (24×30 lỗ) + socket cái 1×22 ×2, 1×4, 1×7, 1×5 + tụ hoá 100µF (10–16V, chân 2.5mm) + tụ gốm 100nF chân cắm.
- [ ] Hàn theo trang, đo thông mạch + 4 cặp không chập (5V–GND, 3V3–GND, 5V–3V3, 5V–J1.20) trước khi cắm module.
- [ ] Kiểm mép trên board: antenna DevKitC nhô ~7.7mm trên tâm hàng 0; mép board dư thì cưa bớt.

## Carrier PCB v1 — kiểm trước khi đặt JLCPCB (từ review advisor 2026-11-06)
- [x] ~~**BMI160 chiều gương**~~ — khớp theo ảnh 2 mặt W gửi 2026-11-06 (mặt linh kiện: hàng 7 bên trái, VIN trên). Nếu ảnh là ảnh shop → soi lại 1 lần trên board thật. Gốc:: cầm module mặt linh kiện LÊN, đầu VIN hướng ra xa → hàng 7 chân phải ở BÊN TRÁI, hàng 5 bên phải (đúng layout J4 x=52 / J5 x=62.16). Ngược lại = gương, xoay 180° không cứu được → sửa `gen_carrier.py`.
- [ ] In 1:1 F.Silk/F.Fab, đặt DevKitC + BMI160 thật lên kiểm lỗ; antenna phải nằm trên notch.
- [ ] LD2450: đo pitch chân thật (giả định 2.54) + thứ tự 5V/GND/Tx/Rx trên silk module; chốt cách gắn (dây / header cong) — cắm thẳng vào J2 thì radar chĩa lên trời + đè C1.
- [ ] CJMCU-160: ảnh thấy 4 điện trở mã **392 (3.9kΩ)** + 1 IC 5 chân sát VIN (chưa đọc mã). Đo ohm SDA→3V3, SCL→3V3 lúc rút điện: ~3.9k = có pull-up, khỏi lắp R1/R2. Gốc: có pull-up SDA/SCL sẵn không (không có → lắp R1/R2 4k7), có LDO VIN→3V3 không (có → VIN 3V3 ra ~3.1V, vẫn trong dải BMI160).
- [x] Schematic KiCad + ERC 0 lỗi (`hardware/gen_schematic.py`).

## v2 (sau khi bản cầm tay chạy ổn)
- [ ] Chế độ cảm biến đặt xa qua ESP-NOW (frame ~30B, ~10Hz). Cần xa hơn mới dùng LoRa, chỉnh 918,4–923 MHz ≤25mW (VN); kiểm tần số LIVE cả 2 đầu
- [ ] Cân nhắc gộp LiDAR + mmWave trên cùng màn hình
