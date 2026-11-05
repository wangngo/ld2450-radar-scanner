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
- Parser tự viết (`lib/ld2450_parser`) + 6 test native pass.

## Định danh thiết bị (điền khi hàng về — đọc LIVE, không tin nhãn)
- ESP32-S3: MAC = ? (esptool `chip_id`, tra OUI Espressif), flash/PSRAM = ?
- LD2450: chữ in trên module = ?, firmware radar = ?

## Lệnh
- Test parser: `pio test -e native`
- Build: `pio run -e s3_web` · Flash: `pio run -e s3_web -t upload` · Log: `pio device monitor`

## Gotcha
- Trường thứ 4 của mỗi target là **distance resolution**, KHÔNG phải khoảng cách → khoảng cách = hypot(x,y).
- Số có dấu dùng bit15 làm cờ dấu (1 = dương), không phải bù 2.
- Nghĩa dấu speed (dương = lại gần hay ra xa?) và đơn vị mm: theo ld2450-proto/RBEGamer, **chưa verify thực địa**.
