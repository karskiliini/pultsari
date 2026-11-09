"""Convert a 24-bit BMP to ASCII art.

usage: ascii.py image.bmp COLS OUTBASE [gamma] [sharpen]
writes OUTBASE.txt (plain), OUTBASE.ans (ANSI truecolor), OUTBASE.html (preview)
"""
import sys, struct, html

path, cols, outbase = sys.argv[1], int(sys.argv[2]), sys.argv[3]
gamma = float(sys.argv[4]) if len(sys.argv) > 4 else 0.9
sharpen = float(sys.argv[5]) if len(sys.argv) > 5 else 0.5

data = open(path, 'rb').read()
off = struct.unpack_from('<I', data, 10)[0]
W, H = struct.unpack_from('<ii', data, 18)
topdown = H < 0
H = abs(H)
stride = (W * 3 + 3) & ~3

def px(x, y):
    if not topdown:
        y = H - 1 - y
    i = off + y * stride + x * 3
    return data[i + 2], data[i + 1], data[i]

rows = round(cols * H / W / 2.0)
cw, ch = W / cols, H / rows
lum, rgb = [], []
for ry in range(rows):
    ll, cl = [], []
    for cx in range(cols):
        x0, x1 = int(cx * cw), int((cx + 1) * cw)
        y0, y1 = int(ry * ch), int((ry + 1) * ch)
        R = G = B = n = 0
        for y in range(y0, y1):
            for x in range(x0, x1):
                r, g, b = px(x, y)
                R += r; G += g; B += b; n += 1
        R, G, B = R / n, G / n, B / n
        ll.append(0.299 * R + 0.587 * G + 0.114 * B)
        cl.append((R, G, B))
    lum.append(ll); rgb.append(cl)

flat = sorted(v for l in lum for v in l)
lo, hi = flat[int(len(flat) * 0.03)], flat[int(len(flat) * 0.995)]
norm = [[min(1.0, max(0.0, (v - lo) / (hi - lo))) ** gamma for v in l] for l in lum]
# local contrast: mix global value with value normalised to its neighbourhood
local_w = float(sys.argv[6]) if len(sys.argv) > 6 else 0.0
R = 4
if local_w > 0:
    import math
    g = norm
    loc = []
    for y in range(rows):
        l = []
        for x in range(cols):
            vs = [g[yy][xx] for yy in range(max(0, y - R), min(rows, y + R + 1))
                  for xx in range(max(0, x - 2 * R), min(cols, x + 2 * R + 1))]
            m = sum(vs) / len(vs)
            sd = (sum((v - m) ** 2 for v in vs) / len(vs)) ** 0.5 + 0.04
            l.append(1 / (1 + math.exp(-2.0 * (g[y][x] - m) / sd)))
        loc.append(l)
    norm = [[(1 - local_w) * norm[y][x] + local_w * loc[y][x] * (0.35 + 0.65 * norm[y][x] ** 0.5)
             for x in range(cols)] for y in range(rows)]

def at(y, x):
    return norm[min(rows - 1, max(0, y))][min(cols - 1, max(0, x))]

ramp = " .:-=+*#%@"
plain, ansi, htm = [], [], []
for y in range(rows):
    p = a = h = ''
    for x in range(cols):
        blur = sum(at(y + dy, x + dx) for dy in (-1, 0, 1) for dx in (-1, 0, 1)) / 9
        v = min(1.0, max(0.0, norm[y][x] + sharpen * (norm[y][x] - blur)))
        c = ramp[min(len(ramp) - 1, int(v * len(ramp)))]
        p += c
        # colour: boost saturation/brightness a bit so dark pixels stay visible
        r, g, b = rgb[y][x]
        m = max(r, g, b, 1)
        k = min(2.2, 255 / m) ** 0.6
        r, g, b = (int(min(255, ch_ * k)) for ch_ in (r, g, b))
        a += f'\x1b[38;2;{r};{g};{b}m{c}'
        h += f'<span style="color:#{r:02x}{g:02x}{b:02x}">{html.escape(c)}</span>'
    plain.append(p.rstrip())
    ansi.append(a + '\x1b[0m')
    htm.append(h)

open(outbase + '.txt', 'w').write('\n'.join(plain) + '\n')
open(outbase + '.ans', 'w').write('\n'.join(ansi) + '\n')
open(outbase + '.html', 'w').write(
    '<!doctype html><meta charset=utf-8><body style="background:#000;margin:8px">'
    '<pre style="font:9px/9px Menlo,monospace;color:#ccc;float:left;margin-right:12px">'
    + html.escape('\n'.join(plain)) + '</pre>'
    '<pre style="font:9px/9px Menlo,monospace;float:left">' + '\n'.join(htm) + '</pre>')
print(rows, 'rows')
