"""Sơ đồ hàn board đục lỗ 7x9cm (24 cột x 30 hàng, bước 2.54mm) → docs/perfboard.html.

Chạy:  python3 hardware/gen_perfboard.py
Tự kiểm: trích netlist từ schematic (kicad-cli) rồi so với dây + cầu hàn ở đây; lệch là báo lỗi, không ghi file.
Toạ độ (cột c, hàng r) nhìn từ MẶT TRÊN (mặt cắm linh kiện), c=0 bên trái, r=0 mép trên (phía antenna).
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SCH = os.path.join(HERE, "carrier", "carrier.kicad_sch")
OUT = os.path.join(HERE, "..", "docs", "perfboard.html")
CLI = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
COLS, ROWS = 24, 30

# ── Linh kiện: ref -> (nhãn, [(chân, c, r, tên chân)]) ──
def column(c, r0, names):
    return [(i + 1, c, r0 + i, n) for i, n in enumerate(names)]

J1N = ["3V3", "3V3", "RST", "4", "5", "6", "7", "15", "16", "17", "18", "8", "3", "46", "9", "10", "11", "12", "13", "14", "5V", "G"]
J3N = ["G", "TX", "RX", "1", "2", "42", "41", "40", "39", "38", "37", "36", "35", "0", "45", "48", "47", "21", "20", "19", "G", "G"]
PARTS = {
    "J1": ("DevKitC J1", column(6, 0, J1N)),
    "J3": ("DevKitC J3", column(15, 0, J3N)),
    "J2": ("LD2450", column(1, 17, ["5V", "GND", "Tx", "Rx"])),
    "J4": ("BMI160 hàng 7", column(19, 3, ["VIN", "3V3", "GND", "SCL", "SDA", "CS", "SAO"])),
    "J5": ("BMI160 hàng 5", column(23, 4, ["OCS", "INT2", "INT1", "SCX", "SDX"])),
    "C2": ("100nF", [(1, 3, 17, "+"), (2, 3, 18, "−")]),
    "C1": ("100µF", [(1, 4, 20, "+"), (2, 4, 21, "−")]),
}
# Chân nối sẵn bên trong DevKitC → không cần dây trên board đục lỗ
INTERNAL = {("J1", 2), ("J3", 21), ("J3", 22)}

# ── Nối ở mặt dưới: cầu hàn (thiếc/chân linh kiện nối lỗ kề nhau) + dây có vỏ bọc ──
BRIDGES = [  # (net, [(c, r), ...]) các lỗ liền nhau trên một hàng
    ("+5V", [(1, 17), (2, 17), (3, 17)]),
    ("GND", [(1, 18), (2, 18), (3, 18)]),
    ("+5V", [(4, 20), (5, 20), (6, 20)]),
    ("GND", [(4, 21), (5, 21), (6, 21)]),
]
WIRES = [  # (số, net, từ (c, r), tới (c, r), màu)
    ("W1", "+5V", (3, 17), (4, 20), "#d23b2f"),
    ("W2", "GND", (3, 18), (4, 21), "#333333"),
    ("W3", "UART_TX", (6, 9), (1, 20), "#1f6fd1"),
    ("W4", "UART_RX", (6, 10), (1, 19), "#c98a00"),
    ("W5", "+3V3", (6, 0), (19, 3), "#e8742a"),
    ("W6", "+3V3", (19, 3), (19, 8), "#e8742a"),
    ("W7", "GND", (15, 0), (19, 5), "#333333"),
    ("W8", "GND", (19, 5), (19, 9), "#333333"),
    ("W9", "I2C_SDA", (6, 11), (19, 7), "#8a5cd6"),
    ("W10", "I2C_SCL", (6, 14), (19, 6), "#1a9e8f"),
]

HOLE = {}  # (c, r) -> (ref, pin, tên)
for ref, (_, pins) in PARTS.items():
    for pin, c, r, name in pins:
        assert (c, r) not in HOLE, f"trùng lỗ {(c, r)}"
        assert 0 <= c < COLS and 0 <= r < ROWS, f"ngoài board {ref}.{pin}"
        HOLE[(c, r)] = (ref, pin, name)


def sch_nets():
    """{net: {(ref, pin)}} từ schematic."""
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "n.net")
        subprocess.run([CLI, "sch", "export", "netlist", "-o", out, SCH], check=True, capture_output=True)
        txt = open(out, encoding="utf-8").read()
    nets = {}
    body = txt[txt.index("(nets"):]
    for chunk in re.split(r'\(net\s*\(code', body)[1:]:  # mỗi khối (net (code ..) (name ..) (node ..)...)
        name = re.search(r'\(name "([^"]+)"\)', chunk).group(1).lstrip("/")
        nodes = {(a, int(b)) for a, b in re.findall(r'\(ref "(\w+)"\)\s*\(pin "(\d+)"\)', chunk)}
        if not name.startswith("unconnected") and len(nodes) > 1:
            nets[name] = nodes
    assert nets, "không đọc được net nào từ netlist"
    return nets


def board_nets():
    """Union-find qua cầu hàn + dây → {net: {(ref, pin)}}; báo lỗi nếu 1 cụm mang 2 tên net."""
    parent = {}
    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    def join(a, b):
        parent[find(a)] = find(b)
    label = {}
    for net, holes in BRIDGES:
        for h in holes:
            label.setdefault(h, set()).add(net)
        for a, b in zip(holes, holes[1:]):
            assert a[1] == b[1] and abs(a[0] - b[0]) == 1, f"cầu hàn không liền kề {a}-{b}"
            join(a, b)
    for _, net, a, b, _ in WIRES:
        for h in (a, b):
            assert h in HOLE, f"dây {net} vào lỗ trống {h}"
            label.setdefault(h, set()).add(net)
        join(a, b)
    groups = {}
    for h in list(parent):
        groups.setdefault(find(h), set()).add(h)
    nets = {}
    for g in groups.values():
        names = set().union(*(label.get(h, set()) for h in g))
        assert len(names) == 1, f"chập net {names}"
        pins = {HOLE[h][:2] for h in g if h in HOLE}
        nets.setdefault(names.pop(), set()).update(pins)
    return nets


def check():
    want, got = sch_nets(), board_nets()
    ok = True
    for net, pins in want.items():
        need = {p for p in pins if p not in INTERNAL}
        have = got.get(net, set())
        if need != have:
            ok = False
            print(f"LỆCH {net}: thiếu {sorted(need - have)} thừa {sorted(have - need)}")
    for net in set(got) - set(want):
        ok = False
        print(f"LỆCH net lạ {net}")
    print("netlist khớp schematic" if ok else "netlist KHÔNG khớp")
    return ok


# ── Vẽ SVG (đơn vị mm, in 100% = đúng kích thước thật) ──
P, M = 2.54, 4.0
W, H = COLS * P + 2 * M, ROWS * P + 2 * M
xy = lambda c, r, mir: (M + ((COLS - 1 - c) if mir else c) * P + P / 2, M + r * P + P / 2)


def svg(mirror):
    s = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W:.2f} {H:.2f}" width="{W:.2f}mm" height="{H:.2f}mm" '
         f'class="board" role="img" aria-label="{"Mặt dưới (mặt hàn)" if mirror else "Mặt trên (mặt cắm)"}">',
         f'<rect x="0.5" y="0.5" width="{W - 1:.2f}" height="{H - 1:.2f}" rx="1.5" class="pcb"/>']
    # antenna vượt mép trên
    ax0, _ = xy(6, 0, mirror)
    ax1, _ = xy(15, 0, mirror)
    a, b = sorted((ax0, ax1))
    s.append(f'<rect x="{a + 1.5:.2f}" y="-0.0" width="{b - a - 3:.2f}" height="2.2" class="ant"/>')
    tags = [f'<text x="{(a + b) / 2:.2f}" y="1.7" class="tag" text-anchor="middle">▲ antenna thò ra ngoài mép</text>']
    # module outlines (chỉ mặt trên)
    if not mirror:
        def outline(c0, r0, c1, r1, cls):
            x0, y0 = xy(c0, r0, False)
            x1, y1 = xy(c1, r1, False)
            s.append(f'<rect x="{x0 - 1.4:.2f}" y="{y0 - 1.4:.2f}" width="{x1 - x0 + 2.8:.2f}" height="{y1 - y0 + 2.8:.2f}" rx="0.6" class="{cls}"/>')
        outline(6, 0, 15, 21, "mod")
        outline(19, 3, 23, 9, "mod")
        x, y = xy(10.5, 23.2, False)
        tags.append(f'<text x="{x:.2f}" y="{y:.2f}" class="tag" text-anchor="middle">▼ USB (DevKitC)</text>')
    # lỗ trống
    for c in range(COLS):
        for r in range(ROWS):
            if (c, r) not in HOLE:
                x, y = xy(c, r, mirror)
                s.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="0.5" class="h"/>')
    # cầu hàn
    for net, holes in BRIDGES:
        (x0, y0), (x1, y1) = xy(*holes[0], mirror), xy(*holes[-1], mirror)
        s.append(f'<line x1="{x0:.2f}" y1="{y0:.2f}" x2="{x1:.2f}" y2="{y1:.2f}" class="br"/>')
    # dây (vẽ đè cả 2 mặt; mặt trên vẽ mờ để biết đường đi)
    bow = 0
    for name, net, a, b, col in WIRES:
        (x0, y0), (x1, y1) = xy(*a, mirror), xy(*b, mirror)
        mx, my = (x0 + x1) / 2, (y0 + y1) / 2
        cls = f'w{" ghost" if not mirror else ""}'
        if a[0] == b[0]:  # cùng cột: cong ra ngoài để không đè chân giữa / dây khác
            bow += 1
            off = 2.6 if bow % 2 else -2.6
            mx += off
            s.append(f'<path d="M{x0:.2f},{y0:.2f} Q{mx + off:.2f},{my:.2f} {x1:.2f},{y1:.2f}" fill="none" '
                     f'stroke="{col}" class="{cls}"/>')
        else:
            s.append(f'<line x1="{x0:.2f}" y1="{y0:.2f}" x2="{x1:.2f}" y2="{y1:.2f}" stroke="{col}" class="{cls}"/>')
        if mirror:
            s.append(f'<text x="{mx:.2f}" y="{my - 0.4:.2f}" class="wl" text-anchor="middle">{name}</text>')
    # chân linh kiện
    for (c, r), (ref, pin, name) in HOLE.items():
        x, y = xy(c, r, mirror)
        first = pin == 1
        shape = (f'<rect x="{x - 0.85:.2f}" y="{y - 0.85:.2f}" width="1.7" height="1.7" class="pad"/>' if first
                 else f'<circle cx="{x:.2f}" cy="{y:.2f}" r="0.85" class="pad"/>')
        s.append(shape)
        used = (ref, pin) in USED
        s.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="0.4" class="{"drill used" if used else "drill"}"/>')
        if ref in ("J2", "J4", "J5", "C1", "C2") or used:
            left = (c <= 6) != mirror  # nhãn ra phía ngoài
            if ref in ("J1",):
                left = not mirror
            if ref in ("J3", "J4"):
                left = mirror
            dx = -1.2 if left else 1.2
            s.append(f'<text x="{x + dx:.2f}" y="{y + 0.45:.2f}" class="pn" text-anchor="{"end" if left else "start"}">{name}</text>')
    # tên linh kiện
    for ref, (lab, pins) in PARTS.items():
        _, c, r, _ = pins[0]
        x, y = xy(c, r, mirror)
        if ref in ("C1", "C2"):
            s.append(f'<text x="{x:.2f}" y="{y - 1.3:.2f}" class="ref" text-anchor="middle">{ref}</text>')
        else:
            s.append(f'<text x="{x:.2f}" y="{y - 1.4:.2f}" class="ref" text-anchor="middle">{ref}</text>')
    s.extend(tags)
    s.append("</svg>")
    return "\n".join(s)


USED = {(ref, pin) for ref, (_, pins) in PARTS.items() for pin, c, r, _ in pins
        if any((c, r) in (a, b) for _, _, a, b, _ in WIRES) or any((c, r) in h for _, h in BRIDGES)}


def html():
    rows = "\n".join(
        f'<tr><td><span class="sw" style="background:{col}"></span>{n}</td><td class="mono">{net}</td>'
        f'<td>{HOLE[a][0]}.{HOLE[a][1]} <span class="dim">({HOLE[a][2]})</span></td>'
        f'<td>{HOLE[b][0]}.{HOLE[b][1]} <span class="dim">({HOLE[b][2]})</span></td></tr>'
        for n, net, a, b, col in WIRES)
    br = "\n".join(
        f'<tr><td class="mono">{net}</td><td>' + " – ".join(
            f'{HOLE[h][0]}.{HOLE[h][1]}' if h in HOLE else "lỗ trống" for h in holes) + "</td></tr>"
        for net, holes in BRIDGES)
    return f"""<!doctype html>
<html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Board đục lỗ carrier</title>
<style>
:root{{--bg:#f6f4ef;--fg:#1d1d1b;--dim:#6b6b66;--line:#d8d4ca;--pcb:#e9e2cf;--pad:#b98a3a;--mod:#2f6db5;--ant:#d23b2f;--br:#8c8c8c}}
@media (prefers-color-scheme:dark){{:root:not([data-theme="light"]){{--bg:#151614;--fg:#ecebe6;--dim:#9a9a92;--line:#33352f;--pcb:#2a2a24;--pad:#d3a454;--mod:#6aa2ea;--ant:#ff6a5c;--br:#a8a8a8}}}}
:root[data-theme="dark"]{{--bg:#151614;--fg:#ecebe6;--dim:#9a9a92;--line:#33352f;--pcb:#2a2a24;--pad:#d3a454;--mod:#6aa2ea;--ant:#ff6a5c;--br:#a8a8a8}}
*{{box-sizing:border-box}}html,body{{overflow-x:hidden}}
body{{margin:0;background:var(--bg);color:var(--fg);font:15px/1.5 system-ui,-apple-system,sans-serif;padding:20px 16px 40px;max-width:1100px;margin-inline:auto}}
p,li,td{{overflow-wrap:anywhere}}figure,.views>*{{min-width:0}}
h1{{font-size:22px;margin:0 0 4px}}h2{{font-size:17px;margin:28px 0 8px}}p{{margin:6px 0}}.dim{{color:var(--dim)}}.mono{{font-family:ui-monospace,Menlo,monospace}}
.views{{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin-top:14px}}
figure{{margin:0}}figcaption{{font-weight:600;margin-bottom:6px}}
svg.board{{width:100%;height:auto;max-width:520px;display:block}}
.pcb{{fill:var(--pcb);stroke:var(--line);stroke-width:.3}}.h{{fill:var(--line)}}
.pad{{fill:var(--pad)}}.drill{{fill:var(--bg)}}.drill.used{{fill:var(--fg)}}
.mod{{fill:none;stroke:var(--mod);stroke-width:.35;stroke-dasharray:1 .6}}
.ant{{fill:var(--ant);opacity:.18}}
.tag{{font-size:1.5px;fill:var(--ant);font-family:system-ui,sans-serif}}
.br{{stroke:var(--br);stroke-width:1.1;stroke-linecap:round;opacity:.85}}
.w{{stroke-width:.55;stroke-linecap:round}}.w.ghost{{opacity:.28;stroke-dasharray:.8 .6}}
.wl{{font-size:1.3px;fill:var(--fg);font-weight:700;font-family:system-ui,sans-serif;paint-order:stroke;stroke:var(--pcb);stroke-width:.35}}
.pn{{font-size:1.15px;fill:var(--fg);font-family:ui-monospace,Menlo,monospace}}
.ref{{font-size:1.5px;fill:var(--mod);font-weight:700;font-family:system-ui,sans-serif}}
table{{border-collapse:collapse;width:100%;font-size:14px}}td,th{{border-bottom:1px solid var(--line);padding:6px 6px;text-align:left;vertical-align:top}}th{{color:var(--dim);font-weight:500}}
.sw{{display:inline-block;width:12px;height:12px;border-radius:3px;margin-right:6px;vertical-align:-1px}}
.tbl{{overflow-x:auto}}
ol li{{margin:4px 0}}.warn{{border-left:3px solid var(--ant);padding:6px 10px;margin:12px 0}}
@media print{{body{{max-width:none;padding:0}}svg.board{{width:auto;max-width:none}}.noprint{{display:none}}}}
</style></head><body>
<h1>Board đục lỗ carrier — sơ đồ hàn</h1>
<p class="dim">Board 7×9cm (24×30 lỗ, bước 2.54mm). Sinh từ <span class="mono">hardware/gen_perfboard.py</span>, đã đối chiếu tự động với schematic <span class="mono">hardware/carrier/carrier.kicad_sch</span> (ERC 0 lỗi).</p>
<p class="warn"><b>In tỉ lệ 100% (Actual size)</b>, đừng chọn "Fit to page", thì hình đúng kích thước thật. Đặt board lên giấy, <b>căn theo lỗ, không theo viền</b> (viền vẽ chỉ minh hoạ).</p>

<div class="views">
<figure><figcaption>① Mặt trên — mặt cắm socket</figcaption>{svg(False)}
<p class="dim">Ô vuông = chân 1. Khung xanh nét đứt = chỗ DevKitC / BMI160 nằm. Dây vẽ mờ để biết hướng, dây thật nằm mặt dưới.</p></figure>
<figure><figcaption>② Mặt dưới — mặt hàn (đã lật gương)</figcaption>{svg(True)}
<p class="dim">Lật board <b>theo trục dọc</b> (trái↔phải, antenna vẫn ở trên): J1 giờ ở bên PHẢI. Thanh xám = cầu thiếc giữa các lỗ kề nhau. Đường màu = dây có vỏ bọc.</p></figure>
</div>

<h2>Dây nối (mặt dưới, dây có vỏ bọc ~26–30AWG)</h2>
<div class="tbl"><table><thead><tr><th>Dây</th><th>Net</th><th>Từ</th><th>Tới</th></tr></thead><tbody>
{rows}
</tbody></table></div>

<h2>Cầu thiếc (nối các lỗ liền kề cùng hàng)</h2>
<div class="tbl"><table><thead><tr><th>Net</th><th>Các lỗ</th></tr></thead><tbody>
{br}
</tbody></table></div>

<h2>Thứ tự làm</h2>
<ol>
<li>Linh kiện: socket cái 1×22 ×2, 1×4, 1×7, 1×5; tụ hoá 100µF/10V chân cắm (chân cách 2.5mm); tụ gốm 100nF chân cắm (chân cách 2.54mm).</li>
<li>Cắm DevKitC vào 2 socket 1×22 <b>rồi mới hàn</b>, để 2 hàng cách đúng 9 lỗ (22.86mm). Hàn chân 1 và chân 22 mỗi hàng trước, kiểm thẳng rồi hàn hết. Xong thì <b>rút DevKitC ra</b> trước khi làm các bước sau.</li>
<li>Socket BMI160: hàng 7 ở cột trái, hàng 5 cách 4 lỗ về bên phải, lùi xuống 1 lỗ (OCS ngang 3V3). Cắm module mặt có chip LÊN, VIN ở trên.</li>
<li>Tụ: chân <b>+</b> của C1 (vạch trắng là chân −) vào hàng 5V. C2 không có cực.</li>
<li>Cầu thiếc: <b>đặt một đoạn chân linh kiện thừa dọc theo hàng</b> rồi hàn, đừng kéo thiếc qua 3 lỗ. Hàng 17/18 và 20/21 là 5V và GND nằm sát nhau. Cầu 5V ở hàng 20 kết thúc tại J1.21, <b>sát ngay J1.20 (GPIO14) ở trên</b>: không để thiếc lan sang. Sau đó hàn dây W1→W10. Chỗ dây băng qua chân J3 để dây có vỏ bọc nằm sát board.</li>
<li>Trước khi cắm module: đo thông mạch từng dây theo bảng, rồi đo các cặp sau, tất cả phải KHÔNG thông: <b>5V–GND</b>, <b>3V3–GND</b>, <b>5V–3V3</b>, <b>5V–J1.20</b> (GPIO14, chân sát cầu 5V).</li>
<li>Cắm DevKitC + BMI160, cấp USB, <b>chưa cắm radar</b>. Log phải ra <span class="mono"># bmi160 chip 0xD1 ok</span>. Đo socket LD2450: 5V–GND ra ~5V. <b>Rút USB</b>, cắm radar, rồi mới cấp USB lại (không cắm radar khi đang có điện).</li>
</ol>
<p class="dim">Không cần dây cho J1.2 (3V3), J3.21/J3.22 (G): đã nối sẵn bên trong DevKitC. Hàng lỗ r=0 phải sát mép trên board để antenna thò ra ngoài; board còn dư mép thì cưa bớt. Nếu đầu cắm USB vướng board phía dưới, cưa bớt các hàng dưới hàng 22.</p>
</body></html>
"""


if __name__ == "__main__":
    if not check():
        sys.exit(1)
    open(OUT, "w", encoding="utf-8").write(html())
    print("saved", os.path.normpath(OUT))
