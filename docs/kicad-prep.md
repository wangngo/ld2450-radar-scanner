# KiCad board carrier — prep (phiên 2026-11-06)

Mục tiêu: bo **carrier** nhỏ cắm 3 module (ESP32-S3-DevKitC N16R8 + LD2450 + BMI160) + 2 tụ lọc, đặt gia công JLCPCB. Và dựng track **KiCad MCP** dùng lâu dài. Theo P1: setup MCP + research footprint phiên này; dựng schematic+PCB phiên sau (sau restart).

## Quyết định MCP (sau advisor Opus, 2026-11-06)
- **KHÔNG fork `lamaalrajih/kicad-mcp`** — nó chỉ *phân tích*, không tạo mới.
- **Dùng `kicad-mcp-pro`** (github.com/oaslananka/kicad-mcp-pro, **MIT**, baseline KiCad 10): tạo schematic bằng **thao tác file `.kicad_sch`** (né bẫy IPC), PCB qua IPC/SWIG, ERC/DRC/export qua `kicad-cli`. Chạy `uvx kicad-mcp-pro` (không cần fork).
- **Cảnh báo advisor:** 1 maintainer, end-to-end "chưa đủ bằng chứng" → **không tin tới khi tự verify 1 board pass DRC/ERC**. Git-commit từng bước (`.kicad_sch` là text, diff được) làm lưới an toàn.
- **Kiến trúc KiCad 10:** schematic = file (IPC schematic phải đợi KiCad 11); pcbnew SWIG còn ở v10 (gỡ ở v11). → pin KiCad 10.0.x cho board sản xuất.

### Config Claude Code (project-scoped, chưa ghi — chờ W duyệt)
File `.mcp.json` ở repo root (git-track được):
```json
{
  "mcpServers": {
    "kicad": {
      "command": "uvx",
      "args": ["kicad-mcp-pro"],
      "env": {
        "KICAD_MCP_PROJECT_DIR": "/Users/wangngo/ld2450-radar-scanner/hardware",
        "KICAD_MCP_PROFILE": "build",
        "KICAD_MCP_OPERATING_MODE": "write",
        "KICAD_MCP_KICAD_CLI": "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
      }
    }
  }
}
```
- Profile mặc định `default` = 24 tool READ-ONLY. Để **tạo** schematic/PCB cần `build` + mode `write`. Lần đầu có thể để `default`/`readonly` test kết nối rồi đổi.
- Verify trước khi tin: `uvx kicad-mcp-pro doctor` (check path KiCad CLI + IPC). File-based chạy không cần KiCad mở; thao tác GUI qua IPC mới cần bật KiCad → Preferences → Scripting → Enable IPC API Server.
- ⚠️ MCP chỉ nạp tool **sau khi restart Claude Code session**. Và chạy `uvx kicad-mcp-pro` bị auto-mode classifier chặn ("Code from External") → cần W tự chạy (`! uvx …`) hoặc thêm permission rule.

## Footprint — anchor + ĐO LIVE (nút thắt thật theo advisor)

> Luật định danh (CLAUDE.md): số row-to-row của DevKitC PHẢI đo trên board thật — bản N16R8 có thể là clone (YD-ESP32-S3…) khác official. Dưới đây là official làm tham chiếu, W xác nhận LIVE.

| Module | Chân | Pitch | Row-to-row | Nguồn / cần đo |
|---|---|---|---|---|
| ESP32-S3-DevKitC-1 | 2×22 = 44 (J1/J3) | 2.54mm (chuẩn) | **CHƯA chốt** | Official: DXF `DXF_ESP32-S3-DevKitC-1_V1.1_20220429.dxf` (dl.espressif.com). **W đo bằng thước kẹp** + đếm lại 22/hàng + đo dài board. |
| LD2450 | 1×4 | 2.54mm | — (1 hàng) | Manual HLK V1.00 (`docs/datasheets/`). Thứ tự `5V/GND/Tx/Rx` — soi silk board thật. |
| BMI160 CJMCU-160 | 2 hàng (ảnh: 7 + 5?) | 2.54mm | **đo LIVE** | Ảnh W: phải `VIN/3V3/GND/SCL/SDA/CS/SAO`, trái `OCS/INT2/INT1/SCX/SDX`. Đếm + đo lại khi hàn. |

- KiCad lib: thử tìm footprint sẵn (`ESP32-S3-DevKit*`), không có thì ghép 2× `PinSocket_1x22_P2.54mm` đặt theo row spacing đo được. LD2450 = `PinSocket_1x04_P2.54mm`. BMI160 = `PinSocket_1x07` + `1x05` (theo đếm LIVE).
- Caps: C1 100µF (CP SMD hoặc radial THT) + C2 100nF (0805) đặt sát chân 5V LD2450.

## Netlist (từ pin firmware s3_web + gotcha BMI160)
- **5V**: ESP32 5V · LD2450 5V · C1(+) · C2
- **3V3**: ESP32 3V3 · BMI160 VIN · BMI160 CS (kéo cao ép I2C)
- **GND**: ESP32 GND · LD2450 GND · BMI160 GND · BMI160 SAO (addr 0x68) · C1(−) · C2
- **UART_TX** (ESP32→radar): ESP32 GPIO17 · LD2450 Rx
- **UART_RX** (radar→ESP32): ESP32 GPIO18 · LD2450 Tx
- **I2C_SDA**: ESP32 GPIO8 · BMI160 SDA
- **I2C_SCL**: ESP32 GPIO9 · BMI160 SCL
- **NC**: BMI160 OCS/SCX/SDX/INT1/INT2
- CS→3V3 và SAO→GND **hàn chết trên carrier** (không ra dây ngoài).

## Bước phiên sau
1. W duyệt + ghi `.mcp.json`; `uvx kicad-mcp-pro doctor` pass; restart session.
2. W cấp số row-to-row DevKitC (đo) + xác nhận đếm chân BMI160.
3. Lái MCP (hoặc kicad-cli) dựng schematic theo netlist → PCB carrier → ERC/DRC pass → export Gerber. Git-commit từng bước.
