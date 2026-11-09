"""Animate the chimney fire, its smoke and the fires on the horizon for the
shape-matched terminal art. The fire and smoke are painted onto the picture's
pixel samples frame by frame, and only the cells they touch are matched again,
so the animated cells look exactly like the rest of the picture.

usage: gen_shape_anim.py image.bmp COLS OUT.json [frames]
Output JSON: { cols, rows, frames, cells: [idx...],
               frames_data: [ [[ch, [r,g,b], [r,g,b]] per cell] per frame ] }
"""
import json, math, os, random, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ascii_shape import Image, Grid, match, boost  # noqa: E402

img_path, cols, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
N = int(sys.argv[4]) if len(sys.argv) > 4 else 48
img = Image(img_path)
grid = Grid(img, cols)
UX, UY = img.W / 200.0, img.H / 150.0  # positions below are in a 200x150 grid

# ---------- periodic value noise (wraps in y so rising motion loops) ----------
random.seed(7)
PX, PY = 64, 16
LAT = [[random.random() for _ in range(PX)] for _ in range(PY)]

def vnoise(x, y):
    xi, yi = math.floor(x), math.floor(y)
    fx, fy = x - xi, y - yi
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    L = lambda i, j: LAT[j % PY][i % PX]
    a = L(xi, yi) + (L(xi + 1, yi) - L(xi, yi)) * fx
    b = L(xi, yi + 1) + (L(xi + 1, yi + 1) - L(xi, yi + 1)) * fx
    return a + (b - a) * fy

def fbm(x, y):
    return 0.55 * vnoise(x, y) + 0.3 * vnoise(2 * x + 17, 2 * y) + 0.15 * vnoise(4 * x + 31, 4 * y)

def clamp(v, lo=0.0, hi=1.0):
    return lo if v < lo else hi if v > hi else v

def mix(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))

# ---------- the scene (same model as gen_anim.py for the browser) ----------
CH_X, CH_Y = 29.0, 23.5            # chimney mouth

def plume_center(gy):
    t = clamp((CH_Y - gy) / CH_Y)
    return CH_X + (6.0 - CH_X) * (1 - (1 - t) ** 1.6)

def plume_half(gy):
    return 2.5 + 13.0 * clamp((CH_Y - gy) / CH_Y) ** 0.7

def in_smoke(gx, gy):
    return gy <= CH_Y - 1 and abs(gx - plume_center(gy)) <= plume_half(gy) + 4

def in_flame(gx, gy):
    return CH_X - 9 <= gx <= CH_X + 9 and CH_Y - 8 <= gy <= CH_Y + 1.5

def smoke_at(gx, gy, ph):
    n = fbm(gx * 0.16, gy * 0.25 + ph * PY)
    d = abs(gx - plume_center(gy)) / (plume_half(gy) + 1e-6)
    edge = clamp(1.5 - d - 0.6 * (n - 0.5))
    return clamp(edge * (0.55 + 1.6 * (n - 0.45)))

def flame_at(gx, gy, ph, tw):
    hgt = (CH_Y + 1.5 - gy) / 9.5
    dx = (gx - CH_X) / 7.5
    n = fbm(gx * 0.9 + 40, gy * 0.7 + ph * PY * 3)
    flick = 0.12 * math.sin(tw * 4 + gx * 0.7) + 0.08 * math.sin(tw * 7 + gy)
    width = 1.0 - 0.75 * hgt + 0.5 * (n - 0.5)
    shape = clamp((width - abs(dx)) * 3)
    return clamp(shape * (1.05 - 0.9 * hgt + 1.3 * (n - 0.5) + flick))

FIRE_PAL = [(0.0, (40, 8, 0)), (0.25, (140, 25, 0)), (0.5, (230, 90, 10)),
            (0.75, (255, 170, 40)), (1.0, (255, 245, 190))]

def fire_col(v):
    for (a, ca), (b, cb) in zip(FIRE_PAL, FIRE_PAL[1:]):
        if v <= b:
            return mix(ca, cb, (v - a) / (b - a))
    return FIRE_PAL[-1][1]

SMOKE_COL = (22, 20, 19)
EXCLUDE = [(95, 29, 170, 150), (52, 42, 82, 150), (0, 56, 200, 150)]  # face, rifle stock, below

def is_fire_colour(c):
    r, g, b = c
    return r > 150 and r > 1.35 * b and r > 1.1 * g

def glow_place(gx, gy):
    return gy < 56 and not any(x0 <= gx < x1 and y0 <= gy < y1 for x0, y0, x1, y1 in EXCLUDE)

_sky = {}
def sky(gy):
    """the smoke-free sky colour at this height (sampled right of the chimneys)"""
    k = int(gy)
    if k not in _sky:
        _sky[k] = boost(img.block(48 * UX, 88 * UX, k * UY, (k + 1) * UY))
    return _sky[k]

# ---------- which cells move ----------
base_samples, points, cells = {}, {}, []
for r in range(grid.rows):
    for c in range(grid.cols):
        pts = [(x / UX, y / UY) for x, y in grid.sample_points(r, c)]
        smp = grid.samples(r, c)
        moving = any(in_smoke(gx, gy) or in_flame(gx, gy) for gx, gy in pts) or \
            any(is_fire_colour(s) and glow_place(gx, gy) for s, (gx, gy) in zip(smp, pts))
        if moving:
            cells.append(r * grid.cols + c)
            base_samples[r * grid.cols + c] = smp
            points[r * grid.cols + c] = pts

def paint(base, gx, gy, ph, tw):
    col = base
    smoky = in_smoke(gx, gy)
    if smoky or (in_flame(gx, gy) and gy <= CH_Y):
        # the static smoke and flame are replaced by sky, then the moving smoke
        s = smoke_at(gx, gy, ph) if smoky else 0.0
        col = mix(sky(gy), SMOKE_COL, s)
        warm = clamp(1 - (CH_Y - gy) / 7) * 0.6 * (0.8 + 0.2 * math.sin(tw * 3 + gx))
        col = mix(col, (200, 90, 20), warm * s)
    if in_flame(gx, gy):
        v = flame_at(gx, gy, ph, tw)
        if v >= 0.12:
            col = mix(col, fire_col(v), clamp(v * 1.8))
    elif is_fire_colour(base) and glow_place(gx, gy):
        k = (0.85 + 0.12 * math.sin(tw * 2 + gx * 0.35 + gy * 0.5) +
             0.08 * math.sin(tw * 5 + gx * 0.9 - gy * 0.3) +
             0.06 * math.sin(tw * 9 + gx * 2.1 + gy * 1.7))
        col = (min(255, base[0] * k), min(255, base[1] * k ** 1.3), min(255, base[2] * k ** 1.6))
    return col

frames = []
for f in range(N):
    ph = f / N
    tw = 2 * math.pi * ph
    fr = []
    for idx in cells:
        smp = [paint(s, gx, gy, ph, tw) for s, (gx, gy) in zip(base_samples[idx], points[idx])]
        ch, fg, bg = match(smp)
        fr.append([ch, list(fg), list(bg)])
    frames.append(fr)
    print(f'\rframe {f + 1}/{N}', end='', file=sys.stderr)
print(file=sys.stderr)

json.dump({'cols': grid.cols, 'rows': grid.rows, 'frames': N, 'cells': cells, 'frames_data': frames},
          open(out, 'w'), separators=(',', ':'))
print(f'{grid.cols}x{grid.rows}: {len(cells)} animated cells, {N} frames')
