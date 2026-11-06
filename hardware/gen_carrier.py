"""Sinh PCB carrier (ESP32-S3-DevKitC-1 + LD2450 + BMI160) bằng pcbnew của KiCad 10.

Chạy bằng Python đi kèm KiCad:
  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 hardware/gen_carrier.py
Netlist = docs/kicad-prep.md. Toạ độ mm, gốc góc trên-trái board, y hướng xuống.
Nguồn kích thước:
  - DevKitC-1 v1.1: DXF Espressif 20220429 -> pitch 2.54, J1 x=1.27 / J3 x=24.13 => row-to-row 22.86mm,
    chân 1 phía antenna, USB phía chân 22. Pinout J1/J3 theo user guide v1.1.
  - BMI160 CJMCU-160: 1x7 + 1x5, cách 10.16mm, hàng 5 lệch 1 bước (nằm giữa) — W xác nhận 2026-11-06.
  - LD2450: 1x4 5V/GND/Tx/Rx theo manual HLK V1.00 (chưa soi module thật).
"""
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "carrier", "carrier.kicad_pcb")
FPLIB = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"
P = 2.54

B = pcbnew.BOARD()
mm = pcbnew.FromMM
V = lambda x, y: pcbnew.VECTOR2I(mm(x), mm(y))

ds = B.GetDesignSettings()
ds.m_TrackMinWidth = mm(0.2)
ds.m_ViasMinSize = mm(0.6)
ds.m_MinClearance = mm(0.2)

NETS = {}
def net(name):
    if name not in NETS:
        n = pcbnew.NETINFO_ITEM(B, name)
        B.Add(n)
        NETS[name] = n
    return NETS[name]

def fp(lib, name, ref, value, x, y, rot=0, pinnets=None):
    f = pcbnew.FootprintLoad(os.path.join(FPLIB, lib + ".pretty"), name)
    f.SetReference(ref)
    f.SetValue(value)
    f.SetPosition(V(x, y))
    if rot:
        f.SetOrientationDegrees(rot)
    B.Add(f)
    for pad in f.Pads():
        n = (pinnets or {}).get(pad.GetNumber())
        if n:
            pad.SetNet(net(n))
    return f

def pad_xy(f, num):
    p = [p for p in f.Pads() if p.GetNumber() == str(num)][0].GetPosition()
    return pcbnew.ToMM(p.x), pcbnew.ToMM(p.y)

def track(netname, pts, w, layer=pcbnew.F_Cu):
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(B)
        t.SetStart(V(x1, y1)); t.SetEnd(V(x2, y2))
        t.SetWidth(mm(w)); t.SetLayer(layer); t.SetNet(net(netname))
        B.Add(t)

def via(netname, x, y):
    v = pcbnew.PCB_VIA(B)
    v.SetPosition(V(x, y)); v.SetDrill(mm(0.3)); v.SetWidth(mm(0.6))
    v.SetNet(net(netname))
    B.Add(v)

W, H = 66.0, 66.0
J1X, J3X, JY = 20.0, 20.0 + 22.86, 10.0          # DevKitC: chân 1 tại y=10
LDX, LDY = 5.0, 45.0                              # LD2450 chân 1 (5V)
BX, BY = 52.0, 20.0                               # BMI160 1x7 chân 1 (VIN)
jy = lambda n: JY + (n - 1) * P                   # y của chân n trên J1/J3

# --- footprint + gán net ---
J1 = fp("Connector_PinSocket_2.54mm", "PinSocket_1x22_P2.54mm_Vertical", "J1", "DevKitC_J1", J1X, JY,
        pinnets={"1": "+3V3", "2": "+3V3", "10": "UART_TX", "11": "UART_RX", "12": "I2C_SDA",
                 "15": "I2C_SCL", "21": "+5V", "22": "GND"})
J3 = fp("Connector_PinSocket_2.54mm", "PinSocket_1x22_P2.54mm_Vertical", "J3", "DevKitC_J3", J3X, JY,
        pinnets={"1": "GND", "21": "GND", "22": "GND"})
J2 = fp("Connector_PinSocket_2.54mm", "PinSocket_1x04_P2.54mm_Vertical", "J2", "LD2450", LDX, LDY,
        pinnets={"1": "+5V", "2": "GND", "3": "UART_RX", "4": "UART_TX"})   # 5V GND Tx Rx
J4 = fp("Connector_PinSocket_2.54mm", "PinSocket_1x07_P2.54mm_Vertical", "J4", "BMI160_R", BX, BY,
        pinnets={"1": "+3V3", "3": "GND", "4": "I2C_SCL", "5": "I2C_SDA", "6": "+3V3", "7": "GND"})
        # VIN 3V3 GND SCL SDA CS SAO ; chân 2 (3V3 ra của board BMI) để trống
J5 = fp("Connector_PinSocket_2.54mm", "PinSocket_1x05_P2.54mm_Vertical", "J5", "BMI160_L",
        BX + 10.16, BY + P)                        # OCS INT2 INT1 SCX SDX — NC

C1 = fp("Capacitor_SMD", "CP_Elec_6.3x5.4", "C1", "100uF/10V", 0, 0, pinnets={"1": "+5V", "2": "GND"})
C2 = fp("Capacitor_SMD", "C_0805_2012Metric", "C2", "100nF", 0, 0, rot=90, pinnets={"1": "+5V", "2": "GND"})
# đặt tụ sao cho pad1 (+) nằm đúng trên đường 5V
def place_pad1(f, x, y):
    px, py = pad_xy(f, 1)
    pos = f.GetPosition()
    f.SetPosition(V(pcbnew.ToMM(pos.x) + x - px, pcbnew.ToMM(pos.y) + y - py))
place_pad1(C2, 8.2, LDY)      # sát chân 5V LD2450
place_pad1(C1, 10.0, 54.0)
C2.Reference().SetPosition(V(11.6, 41.5))   # tránh silk J2

for i, (x, y) in enumerate([(3.5, 3.5), (W - 3.5, 3.5), (3.5, H - 3.5), (W - 3.5, H - 3.5)]):
    fp("MountingHole", "MountingHole_3.2mm_M3", f"H{i+1}", "M3", x, y)

# --- đi dây ---
F, Bk = pcbnew.F_Cu, pcbnew.B_Cu
ld = lambda n: LDY + (n - 1) * P
# 5V: J1.21 -> LD2450.1 (qua C1, C2)
track("+5V", [(J1X, jy(21)), (10, jy(21)), (10, LDY), (LDX, LDY)], 0.6, F)
c1g = pad_xy(C1, 2); via("GND", c1g[0], c1g[1] + 2.0); track("GND", [c1g, (c1g[0], c1g[1] + 2.0)], 0.6, F)
c2g = pad_xy(C2, 2); via("GND", c2g[0], c2g[1] - 1.4); track("GND", [c2g, (c2g[0], c2g[1] - 1.4)], 0.5, F)
# UART_RX (radar Tx -> GPIO18): toàn B.Cu
track("UART_RX", [(J1X, jy(11)), (15, jy(11)), (15, ld(3)), (LDX, ld(3))], 0.25, Bk)
# UART_TX (GPIO17 -> radar Rx): F.Cu rồi xuống B.Cu để vượt UART_RX
track("UART_TX", [(J1X, jy(10)), (12.5, jy(10)), (12.5, 51.4)], 0.25, F)
via("UART_TX", 12.5, 51.4)
track("UART_TX", [(12.5, 51.4), (12.5, ld(4)), (LDX, ld(4))], 0.25, Bk)
# 3V3: J1.2 -> dưới shield module, luồn giữa J3.2/J3.3 -> cột x=58 -> BMI VIN + CS
# (KHÔNG đi mép trên: antenna WROOM thò 6.3mm khỏi mép DevKitC, treo trên notch)
mid = lambda n: jy(n) + P / 2
X33 = 58.0
track("+3V3", [(J1X, jy(1)), (J1X, jy(2))], 0.4, F)
track("+3V3", [(J1X, jy(2)), (40.0, jy(2)), (41.5, mid(2)), (44.5, mid(2))], 0.3, F)
track("+3V3", [(44.5, mid(2)), (X33, mid(2)), (X33, BY + 5 * P), (BX, BY + 5 * P)], 0.4, F)
track("+3V3", [(X33, BY), (BX, BY)], 0.4, F)
# I2C pull-up 4k7 — DNP (lắp nếu CJMCU-160 không có pull-up sẵn)
for ref, pin, netname in (("R1", 4, "I2C_SCL"), ("R2", 5, "I2C_SDA")):
    y = BY + (pin - 1) * P
    r = fp("Resistor_SMD", "R_0805_2012Metric", ref, "4k7 DNP", 0, 0, pinnets={"1": netname, "2": "+3V3"})
    place_pad1(r, 54.75, y)
    try:
        r.SetDNP(True)
    except AttributeError:
        pass
    r.Reference().SetPosition(V(55.7, y - 1.4 if pin == 4 else y + 1.4))
    track(netname, [(BX, y), (54.75, y)], 0.25, F)
    track("+3V3", [pad_xy(r, 2), (X33, y)], 0.3, F)
track("I2C_SDA", [(J1X, jy(12)), (40.0, jy(12)), (41.5, mid(11)), (46.0, mid(11)), (46.0, BY + 4 * P), (BX, BY + 4 * P)], 0.25, Bk)
track("I2C_SCL", [(J1X, jy(15)), (40.0, jy(15)), (41.5, mid(14)), (44.5, mid(14))], 0.25, Bk)
via("I2C_SCL", 44.5, mid(14))
track("I2C_SCL", [(44.5, mid(14)), (44.5, BY + 3 * P), (BX, BY + 3 * P)], 0.25, F)
# GND của BMI (chân 3, SAO chân 7) nối qua pour cả 2 lớp; thêm via khâu
for x, y in [(30, 62), (8, 30), (30, 25), (55, 45), (62, 40), (25, 50)]:
    via("GND", x, y)

# --- viền board ---
# notch mép trên cho antenna WROOM (DXF: module x3.75..21.75 vượt mép board tới y69.15
# -> carrier x~22.5..40.5, y~2.2..8.8): antenna treo trên khoảng trống, không đồng bên dưới
NX0, NX1, NY = J1X + 1.6, J3X - 1.56, 9.5
pts = [(0, 0), (NX0, 0), (NX0, NY), (NX1, NY), (NX1, 0), (W, 0), (W, H), (0, H), (0, 0)]
for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
    s = pcbnew.PCB_SHAPE(B); s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(V(x1, y1)); s.SetEnd(V(x2, y2)); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(mm(0.1))
    B.Add(s)

def poly(x0, y0, x1, y1):
    c = pcbnew.SHAPE_LINE_CHAIN()
    for x, y in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]:
        c.Append(mm(x), mm(y))
    c.SetClosed(True)
    return c

# --- GND pour 2 lớp ---
for layer in (F, Bk):
    z = pcbnew.ZONE(B); z.SetLayer(layer); z.SetNet(net("GND"))
    z.Outline().AddOutline(poly(0.3, 0.3, W - 0.3, H - 0.3))
    z.SetLocalClearance(mm(0.3)); z.SetMinThickness(mm(0.25))
    B.Add(z)
# --- silk: tên chân + hướng cắm (socket đối xứng, cắm ngược 180° là cháy) ---
def silk(txt, x, y, size=0.8, rot=0, layer=pcbnew.F_SilkS):
    t = pcbnew.PCB_TEXT(B); t.SetText(txt); t.SetPosition(V(x, y)); t.SetLayer(layer)
    if layer == pcbnew.B_SilkS:
        t.SetMirrored(True)
    t.SetTextSize(V(size, size)); t.SetTextThickness(mm(size * 0.15))
    if rot:
        t.SetTextAngleDegrees(rot)
    B.Add(t)
for i, n in enumerate(["5V", "GND", "TX", "RX"]):
    silk(n, 2.0, ld(i + 1))
for i, n in enumerate(["VIN", "3V3", "GND", "SCL", "SDA", "CS", "SAO"]):
    silk(n, 49.4, BY + i * P)
silk("ANT ^ (antenna treo notch)", (J1X + J3X) / 2, NY + 1.5, 0.9)
silk("ESP32-S3-DevKitC-1", (J1X + J3X) / 2, 30.0, 1.0)
silk("USB v", (J1X + J3X) / 2, 62.0, 1.0)
silk("ld2450-radar-scanner carrier v1", (J1X + J3X) / 2, 40.0, 0.8, layer=pcbnew.B_SilkS)

pcbnew.ZONE_FILLER(B).Fill(B.Zones())
os.makedirs(os.path.dirname(OUT), exist_ok=True)
B.Save(OUT)
print("saved", OUT)
