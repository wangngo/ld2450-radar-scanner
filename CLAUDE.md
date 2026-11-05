# ld2450-radar-scanner — CLAUDE.md

Máy scan radar **cầm tay, chạy pin**: radar 24GHz HLK-LD2450 + ESP32-S3 có màn hình (dự kiến dùng Waveshare
ESP32-S3-Touch-LCD-2.8, bản 240×320 ST7789). Phát hiện người phía trước, vẽ radar quạt + số liệu từng mục tiêu,
cảnh báo khi có người vào vùng, ghi log CSV lên SD. Plan đầy đủ: `docs/PLAN.md`.

## Trạng thái hiện tại (2026-11-05)
- **Chưa có phần cứng** (W đang mua). Mọi pin/spec board ghi theo wiki Waveshare, **CHƯA verify trên board thật**.
- Đã có: parser LD2450 tự viết (`lib/ld2450_parser`) + 6 test native pass; firmware v0 in CSV ra USB serial, build OK.
- Chưa có: hiển thị, cảnh báo, log SD (xem TASKS.md).

## Định danh thiết bị (điền khi hàng về — đọc LIVE, không tin nhãn)
- ESP32-S3: MAC = ? (esptool `chip_id`, tra OUI Espressif), flash/PSRAM = ?
- LD2450: chữ in trên module = ?, firmware radar = ?

## Lệnh
- Test parser: `pio test -e native`
- Build: `pio run -e waveshare_s3_28` · Flash: `pio run -e waveshare_s3_28 -t upload` · Log: `pio device monitor`

## Gotcha
- Trường thứ 4 của mỗi target là **distance resolution**, KHÔNG phải khoảng cách → khoảng cách = hypot(x,y).
- Số có dấu dùng bit15 làm cờ dấu (1 = dương), không phải bù 2.
- Nghĩa dấu speed (dương = lại gần hay ra xa?) và đơn vị mm: theo ld2450-proto/RBEGamer, **chưa verify thực địa**.
