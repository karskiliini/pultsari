"""Generate looping fire + smoke animation on top of a static ANSI colour art.

usage: python3 gen_anim.py STATIC.ans OUT.json [frames]

The static art must be the 200-column (or any width, coordinates scale)
truecolor art produced by ascii.py. Output JSON:
  { cols, rows, frames, static: [[char, "rgb"]...], cells: [idx...],
    anim: [ "chars-for-frame", ... ], colors: [ "rgbrgb..." (3 hex per cell) ] }
"""
import sys, re, math, json, random

src, out = sys.argv[1], sys.argv[2]
N = int(sys.argv[3]) if len(sys.argv) > 3 else 48

rx = re.compile(r'\x1b\[38;2;(\d+);(\d+);(\d+)m(.)')
grid = [[(int(r), int(g), int(b), c) for r, g, b, c in rx.findall(l)]
        for l in open(src).read().rstrip('\n').split('\n')]
rows, cols = len(grid), len(grid[0])
S = cols / 200.0  # coordinates below are given for a 200x150 grid

RAMP = " .:-=+*#%@"

# ---------- periodic value noise (wraps in y so the rising motion loops) ----
random.seed(7)
PX, PY = 64, 16  # lattice period
LAT = [[random.random() for _ in range(PX)] for _ in range(PY)]

def vnoise(x, y):
    xi, yi = math.floor(x), math.floor(y)
    fx, fy = x - xi, y - yi
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    def L(i, j):
        return LAT[j % PY][i % PX]
    a = L(xi, yi) + (L(xi + 1, yi) - L(xi, yi)) * fx
    b = L(xi, yi + 1) + (L(xi + 1, yi + 1) - L(xi, yi + 1)) * fx
    return a + (b - a) * fy

def fbm(x, y):
    # octaves use integer lattice multiples so the y-period still divides PY
    return (0.55 * vnoise(x, y) + 0.3 * vnoise(2 * x + 17, 2 * y) +
            0.15 * vnoise(4 * x + 31, 4 * y)) / 1.0

def lerp(a, b, t):
    return a + (b - a) * t

def mix(c1, c2, t):
    return tuple(int(lerp(a, b, t)) for a, b in zip(c1, c2))

def clamp(v, lo=0.0, hi=1.0):
    return lo if v < lo else hi if v > hi else v

# ---------- sky background per row (from smoke-free sky cells) ----------
def sky(row):
    xs = range(int(48 * S), int(88 * S))
    cs = [grid[row][x] for x in xs]
    r = sum(c[0] for c in cs) / len(cs)
    g = sum(c[1] for c in cs) / len(cs)
    b = sum(c[2] for c in cs) / len(cs)
    v = sum(RAMP.index(c[3]) if c[3] in RAMP else 0 for c in cs) / len(cs) / 9
    return (r, g, b), v

SKY = [sky(min(r, rows - 1)) for r in range(rows)]

# ---------- regions (200x150 grid units) ----------
CH_X, CH_Y = 29.0, 23.5            # chimney mouth (flame base)
SMOKE_TOP = 0

def plume_center(row):
    # smoke rises from the chimney and leans to the top-left
    t = clamp((CH_Y - row) / CH_Y)
    return lerp(CH_X, 6.0, 1 - (1 - t) ** 1.6)

def plume_half(row):
    t = clamp((CH_Y - row) / CH_Y)
    return 2.5 + 13.0 * t ** 0.7

def is_fire_colour(r, g, b):
    return r > 150 and r > 1.35 * b and r > 1.1 * g

EXCLUDE = [  # boxes (x0,y0,x1,y1) whose orange cells are not fire: face, rifle stock
    (95, 29, 170, 150), (52, 42, 82, 150), (0, 56, 200, 150)]

def excluded(x, y):
    return any(x0 <= x < x1 and y0 <= y < y1 for x0, y0, x1, y1 in EXCLUDE)

smoke_cells, flame_cells, glow_cells = [], [], []
for y in range(rows):
    gy = y / S
    for x in range(cols):
        gx = x / S
        in_flame = (CH_X - 9 <= gx <= CH_X + 9) and (CH_Y - 8 <= gy <= CH_Y + 1.5)
        in_smoke = gy <= CH_Y - 1 and abs(gx - plume_center(gy)) <= plume_half(gy) + 4
        if in_flame:
            flame_cells.append((x, y))
        elif in_smoke:
            smoke_cells.append((x, y))
        else:
            r, g, b, c = grid[y][x]
            if gy < 56 and is_fire_colour(r, g, b) and not excluded(gx, gy):
                glow_cells.append((x, y))

FIRE_PAL = [(0.0, (40, 8, 0)), (0.25, (140, 25, 0)), (0.5, (230, 90, 10)),
            (0.75, (255, 170, 40)), (1.0, (255, 245, 190))]

def fire_col(v):
    for (a, ca), (b, cb) in zip(FIRE_PAL, FIRE_PAL[1:]):
        if v <= b:
            return mix(ca, cb, (v - a) / (b - a))
    return FIRE_PAL[-1][1]

SMOKE_COL = (22, 20, 19)

def smoke_at(gx, gy, ph):
    """smoke density 0..1 at grid point, phase ph in [0,1)"""
    c, h = plume_center(gy), plume_half(gy)
    d = abs(gx - c) / (h + 1e-6)
    n = fbm(gx * 0.16, gy * 0.25 + ph * PY)  # rises: y offset grows with time
    edge = clamp(1.5 - d - 0.6 * (n - 0.5))  # soft, billowing plume edge
    return clamp(edge * (0.55 + 1.6 * (n - 0.45)))

def cell(ch, col):
    return ch, '%x%x%x' % (col[0] >> 4, col[1] >> 4, col[2] >> 4)

frames_chars, frames_cols = [], []
cells = smoke_cells + flame_cells + glow_cells
for f in range(N):
    ph = f / N
    tw = 2 * math.pi * ph
    chars, colstr = [], []

    for x, y in smoke_cells:
        gx, gy = x / S, y / S
        sc, sv = SKY[y]
        s = smoke_at(gx, gy, ph)
        # warm underlight close to the flame
        warm = clamp(1 - (CH_Y - gy) / 7) * 0.6 * (0.8 + 0.2 * math.sin(tw * 3 + gx))
        v = clamp(sv * (1 - 0.9 * s))
        col = mix(sc, SMOKE_COL, s)
        col = mix(col, (200, 90, 20), warm * s)
        v = clamp(v + warm * 0.25 * s)
        ch, cc = cell(RAMP[min(9, int(v * 10))], col)
        chars.append(ch); colstr.append(cc)

    for x, y in flame_cells:
        gx, gy = x / S, y / S
        hgt = (CH_Y + 1.5 - gy) / 9.5  # 0 at base, 1 at top of flame box
        dx = (gx - CH_X) / 7.5
        n = fbm(gx * 0.9 + 40, gy * 0.7 + ph * PY * 3)  # fast rise, narrow tongues
        flick = 0.12 * math.sin(tw * 4 + gx * 0.7) + 0.08 * math.sin(tw * 7 + gy)
        width = 1.0 - 0.75 * hgt + 0.5 * (n - 0.5)
        shape = clamp((width - abs(dx)) * 3)
        v = clamp(shape * (1.05 - 0.9 * hgt + 1.3 * (n - 0.5) + flick))
        if v < 0.12:
            # outside the flame: sky / smoke background
            sc, sv = SKY[y]
            s = smoke_at(gx, gy, ph) if gy < CH_Y - 1 else 0
            base = grid[y][x] if gy > CH_Y else None
            if base and not is_fire_colour(*base[:3]):
                ch, cc = cell(base[3], base[:3])
            else:
                col = mix(sc, SMOKE_COL, s)
                ch, cc = cell(RAMP[min(9, int(clamp(sv * (1 - 0.9 * s)) * 10))], col)
        else:
            ch, cc = cell(RAMP[min(9, 2 + int(v * 8))], fire_col(v))
        chars.append(ch); colstr.append(cc)

    for x, y in glow_cells:
        r, g, b, c = grid[y][x]
        # slow flicker, coherent across neighbouring cells
        k = (0.85 + 0.12 * math.sin(tw * 2 + x * 0.35 + y * 0.5) +
             0.08 * math.sin(tw * 5 + x * 0.9 - y * 0.3) +
             0.06 * math.sin(tw * 9 + x * 2.1 + y * 1.7))
        col = (min(255, int(r * k)), min(255, int(g * k ** 1.3)), min(255, int(b * k ** 1.6)))
        i = RAMP.index(c) if c in RAMP else 5
        i = max(1, min(9, i + (1 if k > 1.0 else -1 if k < 0.8 else 0)))
        ch, cc = cell(RAMP[i], col)
        chars.append(ch); colstr.append(cc)

    frames_chars.append(''.join(chars))
    frames_cols.append(''.join(colstr))

static = [[cell(c, (r, g, b)) for r, g, b, c in row] for row in grid]
json.dump({
    'cols': cols, 'rows': rows, 'frames': N,
    'static': [''.join(c for c, _ in row) for row in static],
    'staticColors': [''.join(cc for _, cc in row) for row in static],
    'cells': [y * cols + x for x, y in cells],
    'chars': frames_chars, 'colors': frames_cols,
}, open(out, 'w'), separators=(',', ':'))
print(f'{len(smoke_cells)} smoke, {len(flame_cells)} flame, {len(glow_cells)} glow cells, {N} frames')
