# TASKS

## Xong
- [x] Parser LD2450 streaming + test native (6 case: frame mẫu datasheet, rác, frame bị cắt, footer sai, resync, 3 mục tiêu)
- [x] Firmware v0: UART 256000 → CSV qua USB serial (build OK, chưa chạy trên board)

- [x] Bản web trên điện thoại (env `s3_web`): WiFi AP + trang radar, cảnh báo, bíp theo khoảng cách, chọn tầm, log CSV (mock-test OK)

## Mở
- [ ] Flash `s3_web` lên ESP32-S3 của W: định danh LIVE (esptool MAC) trước, verify boot log + radar thật + trang trên điện thoại
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

## v2 (sau khi bản cầm tay chạy ổn)
- [ ] Chế độ cảm biến đặt xa qua ESP-NOW (frame ~30B, ~10Hz). Cần xa hơn mới dùng LoRa, chỉnh 918,4–923 MHz ≤25mW (VN); kiểm tần số LIVE cả 2 đầu
- [ ] Cân nhắc gộp LiDAR + mmWave trên cùng màn hình
