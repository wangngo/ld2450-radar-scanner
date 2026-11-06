# Tuỳ chọn UWB — soi người sau vách + nhịp thở (PHASE-2, đang PARK)

> Nghiên cứu 2026-11-07. Trạng thái: **PARK — chờ chốt nguồn hàng X4M200.** Không chặn máy LD2450 hiện tại.

## Vì sao có mục này
LD2450 (24 GHz) định vị người X/Y trong tầm nhìn thẳng nhưng **không xuyên tường**. Câu hỏi: thêm cảm
biến xuyên vách được không. Kết luận: dùng **radar xung UWB (IR-UWB)** là hướng duy nhất khả thi — nhưng
nó là tính năng **khác bài toán** (xuyên vách + phát hiện nhịp thở, KHÔNG định vị 2D), không "nâng cấp" LD2450.

## Vật lý (ràng buộc cứng)
- Tần số càng thấp → xuyên tường càng tốt. Hệ see-through-wall thật dùng **0.9–1.5 GHz** (suy hao qua
  gạch bê tông ~15 dB).
- **LD2450 = 24 GHz** → gần như không qua tường.
- **XeThru X4 (UWB) = 7.29 / 8.748 GHz** → "xuyên mọi vật liệu trừ kim loại", presence tới ~10m. Nghiên cứu
  thực tế bắt được **nhịp thở sau tường 20–30cm** (X4M200). Kim loại/giấy bạc chặn sạch — giới hạn cứng.
- Cái nhận được qua vách = **presence + khoảng cách (1D) + nhịp thở/chuyển động thô**. KHÔNG có X/Y, không
  "ảnh" người. Định vị 2D xuyên tường cần dàn ăng-ten Tx/Rx + DSP nặng = lớp quân sự/nghiên cứu, không có module rẻ.

## Phân biệt (kẻo mua sai đồ)
- **UWB ranging** (Qorvo DW3000/DWM3000, Apple U1): đo khoảng cách GIỮA 2 thiết bị (tag↔anchor). **KHÔNG phải
  radar**, không soi người. BỎ QUA.
- **UWB impulse radar / IR-UWB** (Novelda XeThru X4): radar đơn, nghe phản xạ. **Đây** mới là thứ cần.
- Acconeer A121 (XM125): **60 GHz** → không xuyên tường. BỎ QUA cho mục tiêu này.

## CHỐT phần cứng
- **Mua X4M200 (Respiration sensor)** — KHÔNG phải X4M300 (Presence) và KHÔNG phải X4M03 (raw).
  - X4M200 phát hiện **nhịp thở** = bằng chứng có người **kể cả đứng/nằm yên** → đúng mục tiêu "soi người sau vách".
  - Firmware respiration **onboard** → xuất thẳng `bpm / presence / distance`. ESP32 chỉ đọc, không phải xử lý RF.
- Giá/hàng (verify LIVE trước khi thiết kế tiếp): gốc $80/con @1k (2017, cũ); eBay hiện ~$45 (`X4M200-TR100`).
  Mouser/Digikey gần như hết. Kênh thực tế: eBay/AliExpress/Taobao. **VN hiếm.** ⇒ **BLOCKER: chốt mua được đã.**

## CHỐT nối ESP32-S3
- Host interface = **UART mặc định 115200 8N1** (lưu ý: "SPI 50MHz" trong datasheet là bus NỘI BỘ module↔chip X4,
  KHÔNG phải cổng host — đừng nhầm).
- 🔴 **PHẢI verify datasheet: mức điện áp IO module** (một số XeThru chạy IO **1.8V**). Nếu 1.8V → cần **level
  shifter** với ESP32 3.3V. Nguồn cấp ngốn hơn LD2450 → ảnh hưởng pin.
- Giao thức: **XeThru Module Communication Protocol (MCP)** khung nhị phân. Protocol chuẩn = Novelda
  `pymoduleconnector` (XEP). Fork parser Python để hiểu khung, viết lại C++ gọn cho ESP32.

## CHỐT kiến trúc (2 radar ĐỘC LẬP — KHÔNG fuse toạ độ)
```
ESP32-S3
├─ UART1 (Serial1) GPIO17/18  ← LD2450 @256000   [đang dùng]
├─ UART2          GPIO4/5     ← X4M200 @115200    [THÊM]  (+level shift nếu IO 1.8V)
├─ I2C   GPIO8/9              ← BMI160 gyro        [đang dùng]
├─ UART0/USB                 → log/web            [đang dùng]
└─ GPIO48                    → LED RGB            [đang dùng]
```
- GPIO4/5 rảnh, ngoài vùng PSRAM octal (33–37) + strapping → an toàn.
- ESP32-S3 có 3 UART phần cứng → vừa đủ (0 log, 1 LD2450, 2 UWB), không cần SoftwareSerial.
- UI web: **khối UWB riêng** (`SAU VÁCH: có người · nhịp thở N bpm · ~X m`), + **toggle chế độ** "Định vị
  (LD2450)" ↔ "Soi vách (UWB)". Không trộn chấm toạ độ 2 radar (vật lý khác nhau).
- Cơ khí: ăng-ten UWB hướng ra trước; UWB nhạy sau lưng → vẫn cần tấm chắn sau.

## Thuật toán nhịp thở trên ESP32 — khả thi
- 🟢 **Đường dễ (chốt): để X4M200 tự phát hiện onboard** → đọc bpm. ESP32 ≈ 0 tải. Làm cái này trước.
- 🟡 Đường khó (chỉ nếu đi X4M03 raw): chuỗi chuẩn (đã kiểm chứng qua repo tham khảo bên dưới):
  1. Thu I/Q baseband (downconversion) → biên độ + pha
  2. Ma trận slow-time × fast-time (range bin)
  3. Bám range bin đỉnh → tổng quanh đỉnh
  4. Butterworth bandpass dải thở (~0.1–0.5 Hz) + median filter
  5. FFT slow-time → tần số nhịp thở
  - Toán rất nhẹ (slow-time ~17–20 Hz, vài trăm mẫu × dăm bin) → ESP32-S3 + **esp-dsp** (FFT, FPU) kham thừa.
    Cái khó là bền với nhiễu chuyển động (paper robust dùng deep-learning = KHÔNG hợp MCU). Đối tượng yên → FFT đơn giản là đủ.

## Tham khảo
- Repo **XPengZhao/Respiratory-monitoring** (github) — PC Python + **X4M03 raw** + pymoduleconnector, chứng minh
  chuỗi slow-time→bandpass→FFT ở trên. Hằng số: FPS=17, 91 bin, 0–5m, `bin_length=8*1.5e8/23.328e9≈0.0514m`,
  cấu hình XEP: `downconversion=1, iterations=16, pulses_per_step=300, dac 949–1100, frame_area 0–5, offset 0.18`.
  ⚠️ **NO LICENSE → chỉ tham khảo học thuật, KHÔNG copy code.** Dùng Novelda pymoduleconnector làm protocol chuẩn.
- Novelda datasheets: novelda.com/technology/datasheets. X4: 7.29/8.748 GHz impulse radar SoC.
- Survey: "A Comprehensive Overview on UWB Radar" (arXiv 2402.05649). Through-wall respiration: arXiv 2012.11044.

## Hành động khi gỡ PARK (theo thứ tự rủi ro)
1. Chốt **mua được X4M200** (eBay/AliExpress/Taobao) — blocker.
2. Có hàng → tra datasheet **IO voltage** → quyết level shifter.
3. Nối UART2 GPIO4/5, đọc MCP (fork parser Python → C++ gọn).
4. Đọc bpm onboard trước; web thêm khối UWB + toggle chế độ.
