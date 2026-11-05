# Máy scan radar cầm tay LD2450 + ESP32-S3 có màn hình

## Context
W muốn một máy cầm tay chạy pin, chĩa về phía trước để phát hiện người đang chuyển động hoặc đứng yên còn thở, rồi hiển thị trên màn hình gắn liền. Màn hình cần có 4 phần: radar quạt + chấm mục tiêu, số liệu từng mục tiêu, cảnh báo, ghi log.
Hiện W chưa có phần cứng, nên plan gồm 2 giai đoạn: (A) danh sách mua, (B) firmware, làm khi hàng về.
Chưa có repo GitHub nào làm sẵn LD2450 với màn hình TFT gắn liền. Vì vậy sẽ tái dùng thư viện parser có sẵn, còn phần hiển thị tự viết, tham khảo cách vẽ radar của ShillehTek (web canvas) và Electroniclinic (RD-03D + ESP32-S3 touch).

## A. Phần cứng đề xuất (W tự đặt mua)
| Món | Lý do | Ghi chú |
|---|---|---|
| **HLK-LD2450** (24GHz) | Theo dõi tối đa 3 người: X/Y, tốc độ, khoảng cách. Dùng UART 256000 baud | Nguồn 5V, TX/RX mức 3.3V nên nối thẳng vào ESP32 được |
| **Waveshare ESP32-S3-Touch-LCD-2.8** (bản 240×320 ST7789, không phải 2.8B/2.8C tròn) | ESP32-S3R8, flash 16MB, PSRAM 8MB, cảm ứng, khe TF, mạch sạc LiPo, chip audio + loa (dùng làm cảnh báo), IMU | UART ở header SH1.0: TX=GPIO43, RX=GPIO44. SD: MISO16/MOSI17/SCK14/CS21 (theo wiki, kiểm lại theo revision board khi nhận) |
| Pin LiPo 1S ~1000–2000mAh, đầu MX1.25 | Để cầm tay | Board không kèm pin |
| Mạch boost 3.7V→5V (nếu header board không có chân 5V) | LD2450 cần 5V | **Chưa xác minh** header có 5V hay không, kiểm khi hàng về |
| Thẻ microSD ≤16GB | Ghi log | |
| Vỏ in 3D / hộp nhựa | LD2450 gắn mặt trước, màn hình mặt sau | Mặt trước radar không được che bằng kim loại |

Tham khảo khác: LilyGo T-Display-S3 gọn hơn nhưng không có khe SD, cũng không có loa, nên kém hợp với yêu cầu ghi log + cảnh báo.

## B. Firmware (khi hàng về)
Repo mới, private: `~/ld2450-radar-scanner` → GitHub `wangngo/ld2450-radar-scanner`, gồm CLAUDE.md, TASKS.md, DECISIONS.md, CHANGELOG.md.
Dùng PlatformIO + Arduino core 2.0.17 (platform espressif32@6.9.0, đã ghim), env `esp32-s3-devkitc-1` có PSRAM.

0. **Định danh thiết bị LIVE**: chạy `esptool chip_id/flash_id` để lấy MAC + kiểm OUI Espressif, rồi ghi vào CLAUDE.md repo. Đọc chữ in trên module LD2450, đọc phiên bản firmware radar qua lệnh cấu hình.
1. **Parser**: thử thư viện `RBEGamer/HLK-LD2450` trước. Viết test native (`pio test -e native`) bằng frame 30 byte thật lấy từ UART. Các điểm phải kiểm: bit dấu (top bit = dấu, không phải bù 2), đơn vị (tài liệu ghi lệch nhau mm vs cm, phải **tra manual HLK chính hãng**), speed tính bằng cm/s. Thư viện lỗi thì tự viết parser nhỏ, giữ nguyên test.
2. **Hiển thị** (LovyanGFX, sprite double-buffer trong PSRAM, ~10 fps):
   - Radar quạt 120°, các vòng 1/2/4/6m, mỗi mục tiêu một chấm màu riêng kèm vệt di chuyển ~1s.
   - Dải số liệu: mỗi mục tiêu ghi khoảng cách, góc, tốc độ, hướng tiến/lùi.
   - Thanh trạng thái: % pin (nếu board có chân đo áp pin), có SD hay không, số mục tiêu.
3. **Cảnh báo**: chạm màn hình để đặt bán kính vùng cảnh báo. Có mục tiêu đi vào vùng thì nền chớp đỏ và loa phát tiếng bíp qua chip audio có sẵn.
4. **Log**: ghi CSV lên SD (`ts,id,x,y,speed,dist`), mỗi phiên một file theo uptime/IMU. Đồng thời in ra USB serial.
5. Docs + commit theo từng phiên bản, có tag.

## Verify
- `pio test -e native`: test parser pass với các frame thật (gồm frame không có mục tiêu, có 1 và 3 mục tiêu, toạ độ âm).
- Thực địa: đứng ở 1m, 3m, 5m, đo thước đối chiếu khoảng cách hiển thị (sai số ≤ ~15cm). Đi ngang trái/phải xem góc đổi dấu đúng. Đứng yên xem còn được bám không.
- Cảnh báo kêu đúng khi bước vào vùng. File CSV trên SD đọc lại được trên Mac.
- Chạy pin liên tục, ghi lại thời lượng thực tế.

## Rủi ro đã biết
- LD2450 không xuyên tường. Quạt, rèm bay có thể gây báo nhầm. Người đứng yên lâu có thể bị mất bám (LD2450 thiên về chuyển động, LD2410 mới mạnh về đứng yên). Có thể gắn thêm LD2410 sau nếu cần.
- Chân 5V và chân đo áp pin trên board Waveshare: chưa xác minh, kiểm schematic chính hãng khi nhận board.
