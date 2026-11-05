# ld2450-radar-scanner — CLAUDE.md

Máy scan radar **cầm tay, chạy pin**: radar 24GHz HLK-LD2450 + ESP32-S3 có màn hình (dự kiến dùng Waveshare
ESP32-S3-Touch-LCD-2.8, bản 240×320 ST7789). Phát hiện người phía trước, vẽ radar quạt + số liệu từng mục tiêu,
cảnh báo khi có người vào vùng, ghi log CSV lên SD. Plan đầy đủ: `docs/PLAN.md`.

## Trạng thái hiện tại (2026-11-05)
- **Bản web chạy ngay (env `s3_web`)**: W đã có LD2450 + ESP32-S3. ESP32 phát WiFi `LD2450-Radar` / `radar2450`,
  điện thoại mở `http://192.168.4.1` để xem radar quạt, bảng số liệu, vùng cảnh báo (chớp đỏ), bíp theo khoảng cách
  (WebAudio, phải bấm nút để bật), ghi log CSV tải về điện thoại. Nguồn: sạc dự phòng qua USB.
  Đã kiểm bằng mock server + Chrome headless; **CHƯA chạy trên board + radar thật**.
- Nối dây `s3_web`: LD2450 5V→5V, GND→GND, TX→GPIO18, RX→GPIO17.
- Bản màn hình gắn liền Waveshare (`waveshare_s3_28`): chờ mua board, pin theo wiki **chưa verify**.
- Parser + lệnh cấu hình + bộ lọc tự viết (`lib/ld2450_parser`), 12 test native pass. Máy trạng thái cấu hình radar lúc boot **chưa chạy trên radar thật**.

## Định danh thiết bị (điền khi hàng về — đọc LIVE, không tin nhãn)
- ESP32-S3: MAC = ? (esptool `chip_id`, tra OUI Espressif), flash/PSRAM = ?
- LD2450: chữ in trên module = ?, firmware radar = ?

## Lệnh
- Test parser: `pio test -e native`
- Build: `pio run -e s3_web` · Flash: `pio run -e s3_web -t upload` · Log: `pio device monitor`

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
