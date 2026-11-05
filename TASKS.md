# TASKS

## Xong
- [x] Parser LD2450 streaming + test native (6 case: frame mẫu datasheet, rác, frame bị cắt, footer sai, resync, 3 mục tiêu)
- [x] Firmware v0: UART 256000 → CSV qua USB serial (build OK, chưa chạy trên board)

## Mở
- [ ] W mua hàng: LD2450, Waveshare ESP32-S3-Touch-LCD-2.8 (240×320), LiPo 1S MX1.25, boost 5V (nếu cần), microSD ≤16GB
- [ ] Khi nhận board: định danh LIVE (esptool MAC+OUI), tra schematic: header có 5V không, có chân đo áp pin không, pin LCD/touch/SD/audio
- [ ] Chụp frame thật từ UART → thêm vào test; verify đơn vị mm + dấu speed bằng thước/đi lại gần
- [ ] Hiển thị: LovyanGFX, radar quạt 120° vòng 1/2/4/6m, chấm + vệt, bảng số liệu, thanh trạng thái (`src/main.cpp`)
- [ ] Cảnh báo: chạm đặt bán kính vùng, chớp đỏ + bíp qua chip audio
- [ ] Log CSV lên SD
- [ ] Đo thời lượng pin, làm vỏ
