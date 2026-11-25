# ESP32-S3 + HLK-LD2450 24GHz mmWave Handheld Radar Scanner (with BMI160 Gyro Shake Compensation)

> **Handheld 24GHz mmWave multi-target human radar scanner** powered by **Hi-Link HLK-LD2450** (up to 3 simultaneous targets, 6m range, ±60° azimuth), **ESP32-S3-DevKitC-1 (N16R8)**, **Bosch BMI160 6-axis IMU** for handheld motion/shake compensation, and **on-board WS2812 RGB LED** proximity alerts.
>
> Features a zero-dependency **Wi-Fi SoftAP Web Radar UI** (`http://192.168.4.1` at ~11Hz live update), WebAudio distance-scaled Geiger beep, EMA trajectory smoothing, ghost-on-loss tracking, CSV data logging, **17 native C++ unit tests**, interactive HTML wiring/perfboard guides, and programmatic **KiCad 10 Schematic (`ERC 0`) & Carrier PCB (`DRC 0/0` + JLCPCB Gerbers)**.

---

## 1. Key Engineering Highlights

### A. Custom Stream-Safe `ld2450_parser` (Fixes 3 Bugs in Existing Libraries & Official Manual)

Why we wrote [`lib/ld2450_parser`](lib/ld2450_parser) instead of using popular Arduino libraries such as `RBEGamer/HLK-LD2450`:
1. **Fixes Out-of-Bounds (OOB) Memory Read:** Existing libraries check the frame footer at `rec_buf[i+28]` / `rec_buf[i+29]` without verifying `i + 29 < len`, causing out-of-bounds reads past the 256-byte receive buffer.
2. **Resilient to Split UART Frames at `256000` Baud:** Instead of relying on `Serial.readBytes()` with a 10ms timeout (which drops frames split across two reads), `Ld2450Parser` is a byte-by-byte streaming state machine that resynchronizes seamlessly on header (`AA FF 03 00`) and footer (`55 CC`).
3. **Correct Euclidean Distance vs. Distance-Gate Resolution (+ Official Manual Errata):**
   - Each 8-byte target record contains `(x_mm, y_mm, speed_cm_s, resolution_mm)`.
   - Existing libraries erroneously assign `target.distance = resolution`. In reality, the 4th field is the radar's *distance-gate resolution* (`320 mm`), while true distance is $\sqrt{x^2 + y^2}$ (`hypot(x_mm, y_mm)`).
   - **Official Manual Errata:** Hi-Link's `HLK-LD2450 Manual V1.00` (`docs/datasheets/HLK-LD2450-manual-V1.00.pdf`) contains an arithmetic typo in its worked example, writing `0x40 + 0x01*256 = 360 mm` when `0x0140` is actually **`320 mm`**.
4. **Boot-Time Multi-Target Configuration State Machine (`ld2450_cmd.h`):**
   - Automatically runs the Hi-Link binary command sequence on startup: `0x00FF` (Enable Config) → `0x0091` (Query Target Mode) → `0x0090` (Switch to Multi-Target if in Single mode) → `0x00FE` (End Config), with automatic retry if the radar stays silent in config mode.

### B. Handheld Motion & Shake Compensation (`lib/shake_detect` + Bosch `BMI160`)

Doppler 24GHz mmWave radars measure relative radial velocity. When a handheld scanner is tilted or panned by the operator's hand, stationary walls and objects appear as false moving targets, and the radar's rear lobe can pick up the operator behind the antenna:
- **Physical Shielding:** A metal backplate behind the LD2450 blocks rear-lobe reflections toward the operator (see cross-section in [`docs/wiring.html`](docs/wiring.html)).
- **100Hz Gyro Hysteresis Filter (`lib/shake_detect`):** Reads angular velocity magnitude $\|\omega\| = \sqrt{\omega_x^2 + \omega_y^2 + \omega_z^2}$ at 100Hz (`±250 °/s`) via the official Bosch Sensortec BMI160 driver (`lib/bmi160`, BSD-3-Clause).
  - Trips **ON** when $\|\omega\| \ge 30\text{ °/s}$, holds while $\ge 15\text{ °/s}$, and clears after **`300 ms`** of steady hold.
  - While shaking is detected, the Web UI dims radar targets and displays a shake warning banner, the CSV logger flags `shake=1`, and the on-board WS2812 RGB LED switches to dim blue instead of firing false proximity alerts.

### C. Hardware-as-Code: Programmatic KiCad 10 Schematic, PCB & Netlist-Verified Perfboard

All hardware design artifacts are generated and cross-checked deterministically via Python scripts in [`hardware/`](hardware):
- **`hardware/gen_schematic.py`** → [`hardware/carrier/carrier.kicad_sch`](hardware/carrier/carrier.kicad_sch) (**KiCad 10 ERC: 0 errors**) & [`carrier-schematic.pdf`](hardware/carrier/carrier-schematic.pdf).
- **`hardware/gen_carrier.py`** → [`hardware/carrier/carrier.kicad_pcb`](hardware/carrier/carrier.kicad_pcb) (66×66mm 2-layer Carrier PCB with top antenna notch, **KiCad 10 DRC: 0 violations, 0 unconnected**) & ready-to-order JLCPCB bundle [`carrier-gerber.zip`](hardware/carrier/carrier-gerber.zip).
- **`hardware/gen_perfboard.py`** → [`docs/perfboard.html`](docs/perfboard.html) (top & mirrored bottom soldering guide for a standard 7×9cm 24×30-hole perfboard, **automatically verified against the KiCad schematic netlist**).

---

## 2. Verified Pinout (`ESP32-S3-DevKitC-1 N16R8`)

> Interactive visual wiring diagram: open [`docs/wiring.html`](docs/wiring.html) and [`docs/perfboard.html`](docs/perfboard.html) in any browser.

| Peripheral | Module Pin | ESP32-S3 Pin | Notes & Hardware Gotchas |
| :--- | :--- | :--- | :--- |
| **HLK-LD2450** *(UART1 @ 256000)* | `5V` | `5V` | Requires **>200mA** at 5V (3.3V IO level). |
| | `GND` | `GND` | Common ground. |
| | `TX` | **`GPIO18`** *(RX1)* | Cross-over TX → RX. Avoid GPIO43/44 (used by onboard USB-UART0) and GPIO33–37 (used by Octal PSRAM on N16R8). |
| | `RX` | **`GPIO17`** *(TX1)* | Cross-over RX ← TX. |
| **Bosch BMI160** *(CJMCU-160, I2C @ 400kHz)* | `VIN` | `3V3` | Power from 3.3V rail. |
| | `GND` | `GND` | Common ground. |
| | `SDA` | **`GPIO8`** | I2C Data. |
| | `SCL` | **`GPIO9`** | I2C Clock. |
| | `CS` | **`3V3`** | **Must tie to `3V3`** to select I2C mode (`CS` low selects SPI). |
| | `SAO` *(SDO)* | **`GND`** | **Must tie to `GND`** for I2C address `0x68` (if left floating, module pull-up shifts address to `0x69`). |
| **On-Board RGB LED** | `WS2812` | **`GPIO48`** | Distance alert: Green (>4m) → Yellow → Flashing Red (faster as target approaches <4m); Dim Blue when shaking. |

---

## 3. Building, Testing & Flashing (PlatformIO)

### Run the 17 Native Unit Tests (No Hardware Required)
Tests cover `ld2450_parser` (datasheet frames, garbage resync, split frames, bad footer, 3-target decoding), `ld2450_cmd` / `ld2450_filter` (range gating, `y <= 0` behind-antenna rejection, optional `<1m` clustering), and `shake_detect` hysteresis:

```bash
pio test -e native
```

### Build & Flash Firmware (`s3_web`)
```bash
pio run -e s3_web -t upload
```
1. On your phone or laptop, connect to Wi-Fi AP **`LD2450-Radar`** (password: **`radar2450`**).
2. Open **`http://192.168.4.1`** in your browser.

### Hardware Bring-Up & Diagnostic Environments (`platformio.ini`)
- **`pio run -e s3_log -t upload`**: Identical to `s3_web` but with `ARDUINO_USB_CDC_ON_BOOT=0` so boot logs (`# radar cfg xong`, `# bmi160 chip 0xD1 ok`) output over the `CH340` UART0 USB port instead of the native USB CDC port.
- **`pio run -e i2cscan -t upload`**: Scans I2C bus on `GPIO8/9` and reads register `0x00` (`chip_id = 0xD1`) at `0x68` and `0x69` (`diag/i2cscan.cpp`).
- **`pio run -e led_test -t upload`**: Alternates `GPIO48` (Red) and `GPIO38` (Blue) to identify the on-board WS2812 pin across DevKitC-1 v1.0 / v1.1 revisions (`diag/led_test.cpp`).

---

## 4. Repository Structure

```text
├── src/
│   ├── main.cpp            # Firmware entry, Wi-Fi AP, HTTP endpoints (/data, /cfg), boot cfg FSM
│   ├── web_page.h          # Embedded HTML5 Canvas radar UI + WebAudio Geiger beep + CSV export
│   ├── imu.h               # Bosch BMI160 I2C wrapper (100Hz gyro -> shake_detect)
│   └── led.h               # WS2812 RGB LED distance & shake indicator
├── lib/
│   ├── ld2450_parser/      # Stream-safe LD2450 frame parser, Hi-Link command builder & target filters
│   ├── shake_detect/       # Gyro angular-velocity hysteresis detector
│   └── bmi160/             # Official Bosch Sensortec BMI160 driver (BSD-3-Clause)
├── test/                   # 17 native Unity unit tests (test_parser, test_cmd_filter, test_shake)
├── diag/                   # Hardware bring-up tools (i2cscan.cpp, led_test.cpp)
├── hardware/               # Python generators + KiCad 10 Schematic (ERC 0), PCB (DRC 0/0) & Gerbers
└── docs/                   # Interactive HTML wiring (wiring.html), 7x9cm perfboard guide (perfboard.html) & datasheet
```

---

## Tóm tắt Tiếng Việt

Dự án **Máy quét Radar cầm tay 24GHz mmWave (`HLK-LD2450` + `ESP32-S3-DevKitC-1 N16R8` + Gyro `Bosch BMI160` + LED RGB `WS2812`)**:
1. **Thư viện `lib/ld2450_parser` tự viết (17/17 test native PASS):** Sửa 3 lỗi của thư viện `RBEGamer/HLK-LD2450` (đọc tràn bộ nhớ OOB, mất gói UART bị cắt ở 256000 baud, gán nhầm độ phân giải 320mm thành khoảng cách `hypot(x,y)`) và đính chính lỗi cộng nhầm `360mm` vs `320mm` trong manual Hi-Link V1.00; tự động cấu hình chế độ multi-target (`0x00FF` → `0x0091` → `0x0090` → `0x00FE`) lúc khởi động.
2. **Chống nhiễu khi tay cầm rung (`BMI160` + `shake_detect`):** Đọc tốc độ góc gyro 100Hz, tự động làm mờ mục tiêu nhiễu trên giao diện Web (`http://192.168.4.1`) và báo LED xanh dương khi lia/lắc máy. Lưu ý phần cứng module CJMCU-160: phải nối **`CS → 3V3`** (chọn I2C) và **`SAO → GND`** (giữ địa chỉ `0x68`, nếu để hở chân `SAO` bị kéo lên `0x69`).
3. **Phần cứng & Mạch in:** Mở [`docs/wiring.html`](docs/wiring.html) và [`docs/perfboard.html`](docs/perfboard.html) để xem sơ đồ đấu nối trực quan và hướng dẫn hàn board đục lỗ 7×9cm (đã tự động đối chiếu netlist với KiCad 10 Schematic & Carrier PCB Gerber trong [`hardware/carrier/`](hardware/carrier)).

---

## License

MIT License (see [`LICENSE`](LICENSE)). `lib/bmi160/` is licensed under the BSD-3-Clause License by Bosch Sensortec.
