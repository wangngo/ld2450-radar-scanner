"""Sinh schematic KiCad cho carrier (cùng netlist với gen_carrier.py / docs/kicad-prep.md).

Chạy (python nào cũng được, không cần pcbnew):  python3 hardware/gen_schematic.py
Kiểm:  kicad-cli sch erc hardware/carrier/carrier.kicad_sch
Mỗi chân nối bằng net label đặt đúng đầu chân; chân không dùng gắn dấu X (no_connect).
Chữ cạnh mỗi chân = tên chân in trên module thật.
"""
import os
import re
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "carrier", "carrier.kicad_sch")
SYMLIB = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols/"

# ── Tên chân module (theo user guide DevKitC-1 v1.1 / manual LD2450 / silk CJMCU-160) ──
J1_NAMES = ["3V3", "3V3", "RST", "GPIO4", "GPIO5", "GPIO6", "GPIO7", "GPIO15", "GPIO16", "GPIO17", "GPIO18",
            "GPIO8", "GPIO3", "GPIO46", "GPIO9", "GPIO10", "GPIO11", "GPIO12", "GPIO13", "GPIO14", "5V", "G"]
J3_NAMES = ["G", "TX(43)", "RX(44)", "GPIO1", "GPIO2", "GPIO42", "GPIO41", "GPIO40", "GPIO39", "GPIO38",
            "GPIO37", "GPIO36", "GPIO35", "GPIO0", "GPIO45", "GPIO48", "GPIO47", "GPIO21", "GPIO20", "GPIO19", "G", "G"]

G = lambda n: round(n * 2.54, 2)  # mọi toạ độ symbol trên lưới 100mil

# ref, lib, symbol, value, footprint, x, y, {pin: net}, [tên chân]
PARTS = [
    ("J1", "Connector_Generic", "Conn_01x22", "DevKitC-1 J1 (trái)", "Connector_PinSocket_2.54mm:PinSocket_1x22_P2.54mm_Vertical",
     G(24), G(40), {1: "+3V3", 2: "+3V3", 10: "UART_TX", 11: "UART_RX", 12: "I2C_SDA", 15: "I2C_SCL", 21: "+5V", 22: "GND"}, J1_NAMES),
    ("J3", "Connector_Generic", "Conn_01x22", "DevKitC-1 J3 (phải)", "Connector_PinSocket_2.54mm:PinSocket_1x22_P2.54mm_Vertical",
     G(50), G(40), {1: "GND", 21: "GND", 22: "GND"}, J3_NAMES),
    ("J2", "Connector_Generic", "Conn_01x04", "LD2450", "Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical",
     G(78), G(22), {1: "+5V", 2: "GND", 3: "UART_RX", 4: "UART_TX"}, ["5V", "GND", "Tx", "Rx"]),
    ("J4", "Connector_Generic", "Conn_01x07", "BMI160 hàng 7", "Connector_PinSocket_2.54mm:PinSocket_1x07_P2.54mm_Vertical",
     G(78), G(41), {1: "+3V3", 3: "GND", 4: "I2C_SCL", 5: "I2C_SDA", 6: "+3V3", 7: "GND"},
     ["VIN", "3V3", "GND", "SCL", "SDA", "CS", "SAO"]),
    ("J5", "Connector_Generic", "Conn_01x05", "BMI160 hàng 5", "Connector_PinSocket_2.54mm:PinSocket_1x05_P2.54mm_Vertical",
     G(78), G(59), {}, ["OCS", "INT2", "INT1", "SCX", "SDX"]),
    ("C1", "Device", "C_Polarized", "100uF/10V", "", G(98), G(22), {1: "+5V", 2: "GND"}, None),
    ("C2", "Device", "C", "100nF", "", G(108), G(22), {1: "+5V", 2: "GND"}, None),
]

U = lambda: str(uuid.uuid4())
ROOT = U()


def block(txt, name):
    i = txt.index('(symbol "%s"' % name)
    d = 0
    for j in range(i, len(txt)):
        if txt[j] == "(":
            d += 1
        elif txt[j] == ")":
            d -= 1
            if d == 0:
                return txt[i:j + 1]


def pins_of(b):
    """{số chân: (x, y)} theo toạ độ thư viện (y hướng lên)."""
    out = {}
    for m in re.finditer(r'\(pin \w+ \w+\s*\(at ([-\d.]+) ([-\d.]+) [-\d.]+\).*?\(number "(\w+)"', b, re.S):
        out[int(m.group(3))] = (float(m.group(1)), float(m.group(2)))
    return out


libs, pinmap = {}, {}
for _, lib, sym, *_ in PARTS:
    key = f"{lib}:{sym}"
    if key in libs:
        continue
    b = block(open(SYMLIB + lib + ".kicad_sym", encoding="utf-8").read(), sym)
    pinmap[key] = pins_of(b)
    libs[key] = b.replace(f'(symbol "{sym}"', f'(symbol "{key}"', 1)


def prop(name, val, x, y, hide=False, left=False):
    h = " (hide yes)" if hide else ""
    j = " (justify left)" if left else ""
    return f'(property "{name}" "{val}" (at {x:.2f} {y:.2f} 0) (effects (font (size 1.27 1.27)){j}){h})'


out = [f'(kicad_sch (version 20250114) (generator "gen_schematic.py") (generator_version "1")',
       f'(uuid "{ROOT}") (paper "A4")',
       '(title_block (title "ld2450-radar-scanner carrier") (comment 1 "netlist = docs/kicad-prep.md; PCB = gen_carrier.py"))',
       "(lib_symbols " + "\n".join(libs.values()) + ")"]

used = {}
for ref, lib, sym, val, fpn, X, Y, nets, names in PARTS:
    key = f"{lib}:{sym}"
    pins = pinmap[key]
    top = max(y for _, y in pins.values())
    out.append(f'(symbol (lib_id "{key}") (at {X} {Y} 0) (unit 1) (exclude_from_sim no) (in_bom yes) (on_board yes) (dnp no) (uuid "{U()}")')
    if names:  # connector: tên trên đầu
        out.append(prop("Reference", ref, X, Y - top - 3.0))
        out.append(prop("Value", val, X, Y - top - 5.0))
    else:  # tụ: tên bên phải, tránh nhãn net ở 2 đầu
        out.append(prop("Reference", ref, X + 2.5, Y - 1.3, left=True))
        out.append(prop("Value", val, X + 2.5, Y + 1.3, left=True))
    out.append(prop("Footprint", fpn, X, Y, hide=True))
    out.append(prop("Datasheet", "", X, Y, hide=True))
    for n in pins:
        out.append(f'(pin "{n}" (uuid "{U()}"))')
    out.append(f'(instances (project "carrier" (path "/{ROOT}" (reference "{ref}") (unit 1)))))')
    for n, (px, py) in pins.items():
        sx, sy = X + px, Y - py  # lib y lên → sch y xuống
        net = nets.get(n)
        if net:
            used.setdefault(net, []).append(f"{ref}.{n}")
            ang = 180 if px < 0 else (90 if py > 0 else 270) if px == 0 else 0
            just = "right" if ang == 180 else "left"
            out.append(f'(label "{net}" (at {sx:.2f} {sy:.2f} {ang}) (effects (font (size 1.27 1.27)) (justify {just} bottom)) (uuid "{U()}"))')
        else:
            out.append(f'(no_connect (at {sx:.2f} {sy:.2f}) (uuid "{U()}"))')
        if names:
            out.append(f'(text "{names[n - 1]}" (exclude_from_sim yes) (at {X + 2.6:.2f} {sy:.2f} 0) '
                       f'(effects (font (size 1.0 1.0)) (justify left)) (uuid "{U()}"))')

notes = ["CS→3V3 (ép I2C), SAO→GND (addr 0x68) hàn chết trên board.",
         "C1+C2 đặt sát chân 5V của LD2450.",
         "UART chéo: GPIO17(TX)→radar Rx, GPIO18(RX)←radar Tx."]
for i, t in enumerate(notes):
    out.append(f'(text "{t}" (exclude_from_sim yes) (at {G(16)} {G(62) + i * 5.08} 0) (effects (font (size 1.5 1.5)) (justify left)) (uuid "{U()}"))')
out.append(f'(sheet_instances (path "/" (page "1"))))')

os.makedirs(os.path.dirname(OUT), exist_ok=True)
open(OUT, "w", encoding="utf-8").write("\n".join(out))
for net, members in sorted(used.items()):
    print(f"{net:8s} {' '.join(members)}")
print("saved", OUT)
