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

## 2026-11-05 — Giữ thiết kế cầm tay một khối; bíp theo khoảng cách + chọn tầm đo
Tham khảo một bài cộng đồng: LD2450 → ESP32-S3 → LoRa → Raspberry Pi + Pygame. Tác giả nói phần khó nhất là đường
truyền LoRa/UART (từng lệch 868 vs 915 MHz), và ngoài 4m lưới + âm thanh khó dùng.
Quyết định:
- Giữ máy một khối, không có khâu truyền không dây.
- Thêm âm thanh theo khoảng cách và cho chọn tầm 3/4/6m.
- Chế độ cảm biến đặt xa để sang v2, ưu tiên ESP-NOW (không cần module thêm). Nếu dùng LoRa thì phải ở
  918,4–923 MHz, công suất ≤25mW theo quy định VN (868 đã bỏ, 915 trùng GSM900).
- Giao diện là radar quét theo phong cách riêng, không chép giao diện phim.

## 2026-11-05 — Bản web: WiFi AP + HTTP poll, không dùng WebSocket
Dùng WebServer có sẵn trong Arduino core, điện thoại fetch `/data` 100ms một lần. Không cần thư viện ngoài, trễ
~100ms là đủ dùng. Log CSV lưu trên điện thoại (tải file) thay cho thẻ SD. Âm thanh dùng WebAudio trên điện thoại.
Chân UART chọn GPIO18/17 thay cho 43/44, vì trên DevKitC 43/44 nối với chip USB-UART.

## 2026-11-05 — Tham khảo repo Stevee87 (RD-03D): lấy gì, bỏ gì
Repo `Arduino-ESP32-Radarproject`, `Radar-project-Uconsole`, `Tactical-Radar-System...`, `Northworks...` dùng
Ai-Thinker RD-03D, định dạng frame + mã hoá dấu giống LD2450. Lấy: lệnh multi-target lúc boot, gộp mục tiêu < 1m,
lọc ngoài tầm. KHÔNG lấy "ghost filter" (ẩn mục tiêu đứng yên 2s): đi ngược mục tiêu phát hiện người đứng yên.
IMU trong repo Tactical chỉ để vẽ la bàn, không bù chuyển động radar → phần bù gyro phải tự làm.
Tác giả cũng ghi: radar bị di chuyển thì độ chính xác giảm mạnh (khớp manual LD2450).

## 2026-11-05 — Review advisor: gộp mục tiêu mặc định TẮT
Gộp <1m làm slot biến mất → UI đếm "mất dấu" ảo, chấm nằm giữa 2 người; ý tưởng gốc từ RD-03D, chưa có bằng chứng
LD2450 tách tay/thân thành 2 mục tiêu. Giữ tính năng (nút bật trên web) nhưng mặc định tắt cho tới khi test thật.
