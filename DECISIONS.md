# DECISIONS

## 2026-11-05 — Tự viết parser LD2450 thay vì dùng thư viện RBEGamer/HLK-LD2450
Đã xem source RBEGamer (commit a670c87). Lý do không dùng:
1. Kiểm footer `rec_buf[i+28/29]` mà không kiểm `i+29 < len` → đọc vượt dữ liệu thật (có thể vượt cả buffer 256B).
2. `readBytes()` timeout 10ms → frame bị cắt giữa 2 lần đọc thì mất.
3. Gán `distance = resolution`, nhưng trường đó là distance-gate resolution, không phải khoảng cách.
Parser riêng chỉ ~60 dòng, đọc từng byte, test native được. Cách giải mã bit dấu đối chiếu với thư viện
RBEGamer và crate Rust `ld2450-proto` 0.2.0 (frame mẫu lấy từ datasheet). Cả hai cách cho cùng kết quả.

## 2026-11-05 — Board: Waveshare ESP32-S3-Touch-LCD-2.8 (240×320)
Có khe TF, sạc LiPo, chip audio + loa, cảm ứng, PSRAM 8MB, đủ cả 4 yêu cầu (radar/số liệu/cảnh báo/log).
LilyGo T-Display-S3 gọn hơn nhưng không có SD + loa.
