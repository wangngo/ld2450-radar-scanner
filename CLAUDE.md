# ld2450-radar-scanner — CLAUDE.md

Máy scan radar **cầm tay, chạy pin**: radar 24GHz HLK-LD2450 + ESP32-S3 có màn hình (dự kiến dùng Waveshare
ESP32-S3-Touch-LCD-2.8, bản 240×320 ST7789). Phát hiện người phía trước, vẽ radar quạt + số liệu từng mục tiêu,
cảnh báo khi có người vào vùng, ghi log CSV lên SD. Plan đầy đủ: `docs/PLAN.md`.

## Trạng thái hiện tại (2026-11-07)
- **Bản web (env `s3_web`) — ĐÃ CHẠY TRÊN PHẦN CỨNG THẬT (2026-11-07).** W hàn LD2450↔ESP32-S3, flash OK, verify qua
  điện thoại: AP `LD2450-Radar`/`radar2450` → `http://192.168.4.1`, web mở được + **chấm mục tiêu nhảy theo người đi**
  → mối hàn UART (TX→GPIO18, RX→GPIO17 chéo) + 5V/GND đúng, parser + WiFi AP + web chạy thật. Nguồn: USB.
  Log serial (qua bản `s3_log` CDC off) xác nhận: `# radar cfg xong` + mode **multi** (LD2450 nhận lệnh Hi-Link) +
  `# bmi160 chip 0xD1 ok` + radar **11Hz**. **Gyro BMI160 đã hàn + chạy thật** (0x68, SAO→GND).
  CÒN verify: chiều dấu speed (lại gần/ra xa) đo thực địa; chỉnh ngưỡng lắc thực địa.
- Nối dây `s3_web`: LD2450 5V→5V, GND→GND, TX→GPIO18, RX→GPIO17.
- Bản màn hình gắn liền Waveshare (`waveshare_s3_28`): chờ mua board, pin theo wiki **chưa verify**.
- **Carrier PCB v1** (`hardware/gen_carrier.py`, DRC 0/0, Gerber sẵn) — CHƯA đặt in: chờ kiểm chiều gương BMI160 + LD2450 pitch + in 1:1 (TASKS.md).
- **Gyro BMI160**: driver Bosch + phát hiện lắc (web làm mờ mục tiêu khi lắc). Build OK, test 17/17, **chưa chạy trên chip thật**.
- **Board đục lỗ**: sơ đồ hàn `docs/perfboard.html` + schematic `hardware/carrier/carrier.kicad_sch` (ERC 0). Sinh lại: `python3 hardware/gen_schematic.py && python3 hardware/gen_perfboard.py` (script tự so netlist).
- Parser + lệnh cấu hình + bộ lọc tự viết (`lib/ld2450_parser`), 12 test native pass. Máy trạng thái cấu hình radar lúc boot **chưa chạy trên radar thật**.

## Định danh thiết bị (điền khi hàng về — đọc LIVE, không tin nhãn)
- ESP32-S3 (verify LIVE 2026-11-06, esptool v5.3.1, cổng `/dev/cu.usbserial-0001`): QFN56 **rev v0.2**, 40MHz xtal. **MAC `1c:db:d4:43:ef:78`** → OUI `1C:DB:D4` = **Espressif** (tra nmap-mac-prefixes, khớp → silicon thật). Flash **16MB** (mfr 0x68, quad) + PSRAM **8MB** embedded → **đúng N16R8**. PSRAM octal chiếm GPIO33–37 → cấm dùng. Board form-factor = **ESP32-S3-DevKitC-1 v1.1 official** (W xác nhận; esptool không suy ra được form-factor, neo theo lời W + cần row-to-row từ bản vẽ Espressif hoặc đo thước kẹp cho footprint).
- LD2450: chữ in trên module = ?, firmware radar = ?
- **LED RGB on-board = WS2812 ở GPIO48** (verify LIVE 2026-11-07: nháy thử 48/38 → board sáng ĐỎ ở 48, tức mapping v1.0 dù form-factor ghi v1.1). Dùng `neopixelWrite(48,r,g,b)`. Cảnh báo ở `src/led.h`.
- Motor rung (chưa gắn mạch): ERM trần **2.5–4V, 70mA** → phải có NPN S8050 + 1kΩ base + diode 1N4148, cấp 3V3, PWM LEDC. Chưa chốt GPIO.
- Gyro = **BMI160** (board tím CJMCU-160). Neo: nhãn chân mặt sau `OCS/INT2/INT1/SCX/SDX` + `VIN/3V3/GND/SCL/SDA/CS/SAO` = chữ ký pinout BMI160 (IMU 6 trục). Chữ laser trên chip chưa đọc được → verify cuối = I2C scan + Chip ID reg `0x00` phải = `0xD1`.
  - Chiều: mặt linh kiện LÊN + VIN hướng xa → hàng 7 (VIN…SAO) bên TRÁI, hàng 5 bên phải, hàng 5 nằm giữa (OCS ngang 3V3). Nhãn chữ in ở mặt KHÔNG có chip (ngược gương).
  - I2C: VIN→3V3, GND→G, SDA→GPIO8, SCL→GPIO9. **CS→3V3 (ép I2C, CS thấp=SPI)**, **SAO→GND (addr 0x68)**. Board *có thể* đã pull-up sẵn (ảnh 2 mặt 2026-11-06 thấy 4 điện trở **392 = 3.9kΩ**, ghi chú cũ "103" có thể khác lô; chưa rõ nối chân nào → đo ohm tới 3V3) → quét I2C trước, trống mới hàn thêm. OCS/SCX/SDX/INT1/INT2 bỏ trống.
  - **VERIFY LIVE 2026-11-07**: chip_id reg 0x00 = **0xD1** → BMI160 thật, chạy OK. **GOTCHA: SAO thả nổi → module tự kéo lên → ra địa chỉ `0x69`** (không phải 0x68). Phải nối **SAO→GND** mới về `0x68` khớp firmware (`imu.h` hardcode 0x68). Quét bằng `diag/i2cscan.cpp` (env `i2cscan`).

## Lệnh
- Test parser: `pio test -e native`
- Build: `pio run -e s3_web` · Flash: `pio run -e s3_web -t upload` · Log: `pio device monitor`
- **Đọc log qua cổng usbserial (CH340)**: bản `s3_web` (CDC=1) log chỉ ra cổng USB native. Muốn đọc qua cổng CH340 → flash env **`s3_log`** (= s3_web, `CDC_ON_BOOT=0`) rồi monitor. Reset board không cần bấm nút: toggle RTS trong pyserial (`p.dtr=False; p.rts=True; sleep; p.rts=False`).
- **Chẩn đoán I2C/gyro**: env **`i2cscan`** (`diag/i2cscan.cpp`) — quét bus GPIO8/9, đọc chip_id 0x68 & 0x69. Dùng khi web báo `gyro ✗`.
- **Test chân LED RGB**: env **`led_test`** (`diag/led_test.cpp`) — nháy GPIO48(đỏ)/38(xanh) để tìm chân LED on-board thật.

## Gotcha
- Trường thứ 4 của mỗi target là **distance resolution**, KHÔNG phải khoảng cách → khoảng cách = hypot(x,y).
- Số có dấu dùng bit15 làm cờ dấu (1 = dương), không phải bù 2.
- Đơn vị x/y mm, speed cm/s, 10 frame/s: **đã xác nhận theo manual HLK V1.00** (`docs/datasheets/`). Manual chỉ nói
  bit15=1 là "speed dương", không nói dương = lại gần hay ra xa → **vẫn phải thử thực địa**.
- Manual có lỗi tính: ví dụ ghi resolution "0x40+0x01*256 = 360mm", thực ra bằng 320. Test dùng 320.
- Manual: nguồn 5V, nguồn cấp phải chịu được **>200mA**; IO 3.3V; tầm 6m, góc ngang ±60°, góc dọc ±35°;
  chỉ có 4 chân 5V/GND/Tx/Rx, không có chân OUT.
- Manual: **radar bị rung lắc sẽ ảnh hưởng kết quả đo**, và **búp sóng phía sau** có thể bắt chuyển động sau lưng radar
  → cầm tay thì người cầm nằm ngay sau radar: cần **tấm kim loại chắn mặt sau** + bù/bỏ dữ liệu khi lia máy (gyro).
- Manual: nhiều radar 24GHz không được đặt chĩa thẳng vào nhau, càng xa nhau càng tốt (liên quan nếu thêm LD2410B).
- Trang shop thegioimodule (LD2450, 250k) chép mô tả của LD2410: ghi góc 60°, 5–12V, có chân OUT. Đừng tin, theo manual.
