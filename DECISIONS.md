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

## 2026-11-07 — UWB "soi người sau vách": chốt X4M200, PARK sang Phase-2
Hỏi: thêm RCWL-0515 có cho LD2450 xuyên tường không → KHÔNG (2.7GHz Doppler, chỉ 1-bit chuyển động, vô hướng,
hại nhiều hơn lợi cho máy cầm tay). Hướng xuyên vách khả thi duy nhất = **radar xung UWB (IR-UWB) Novelda XeThru
X4** (7.29/8.748 GHz), bắt được presence + khoảng cách + nhịp thở sau tường 20–30cm (trừ kim loại).
Quyết định:
- **Mua X4M200** (respiration, firmware onboard) — KHÔNG X4M300 (presence) và KHÔNG X4M03 (raw, phải tự xử lý hết).
- **PARK sang Phase-2**, không gắn vào máy LD2450 hiện tại: khác bài toán (xuyên vách + nhịp thở, KHÔNG định vị
  2D), + blocker nguồn hàng (X4M200 hiếm, đời cũ, ~$45 eBay). Chốt mua được rồi mới thiết kế tiếp.
- Nối ESP32 qua **UART2 GPIO4/5** @115200, 2 radar độc lập, KHÔNG fuse toạ độ; UI khối riêng + toggle chế độ.
  ⚠️ Verify IO voltage module (có thể 1.8V → cần level shifter).
- Repo tham khảo XPengZhao/Respiratory-monitoring: học khung XEP + thuật toán (slow-time→bandpass→FFT), **NO
  LICENSE → không copy code**. Chi tiết: `docs/uwb-option.md`.
- Bỏ qua: DW3000/U1 (ranging tag, không soi người), Acconeer A121 (60GHz, không xuyên tường), RCWL-0515/0516.
