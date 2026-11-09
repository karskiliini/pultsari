"""Shape-matched colour ASCII art: for every cell pick the character whose
glyph shape best matches the picture under it, plus a foreground and a
background colour. Much more detail than a brightness ramp at low resolutions.

usage: ascii_shape.py image.bmp COLS OUT.ans [crop_y0 crop_y1]
  writes OUT.ans (ANSI truecolor fg+bg) and OUT.txt (characters only)

Glyph shapes (glyphs4x8.json) are the ink coverage of each printable ASCII
character on a 4x8 grid, measured from Menlo with the cell squeezed to 1:2.
"""
import json, os, struct, sys

here = os.path.dirname(os.path.abspath(__file__))
GLYPHS = json.load(open(os.path.join(here, 'glyphs4x8.json')))
# only shapes that read as picture structure (lines, edges, blobs); busy
# letters like g, M or & turn into noise at this size
ALLOWED = ' .,:;\'"-_=+~^|/\\()[]<>!*#%oO8LJTVYXAUH'
GLYPHS = {c: v for c, v in GLYPHS.items() if c in ALLOWED}
GW, GH = 4, 8

path, cols, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
data = open(path, 'rb').read()
off = struct.unpack_from('<I', data, 10)[0]
W, H = struct.unpack_from('<ii', data, 18)
topdown = H < 0
H = abs(H)
stride = (W * 3 + 3) & ~3
y0 = int(sys.argv[4]) if len(sys.argv) > 4 else 0
y1 = int(sys.argv[5]) if len(sys.argv) > 5 else H
rows = round(cols * (y1 - y0) / W / 2)

def block(x0, x1, ya, yb):
    """mean RGB of an image rectangle (sampled every 2 px)"""
    R = G = B = n = 0
    for y in range(int(ya), max(int(ya) + 1, int(yb)), 2):
        yy = y if topdown else H - 1 - y
        base = off + yy * stride
        for x in range(int(x0), max(int(x0) + 1, int(x1)), 2):
            i = base + x * 3
            B += data[i]; G += data[i + 1]; R += data[i + 2]; n += 1
    return (R / n, G / n, B / n)

# brighten a little: terminal glyphs on black read darker than pixels
def boost(c, k=1.25, gamma=0.85):
    return tuple(min(255.0, 255 * ((v / 255) ** gamma) * k) for v in c)

glyph_items = [(ch, cov, sum(cov) / len(cov)) for ch, cov in GLYPHS.items()]
cw, chh = W / cols, (y1 - y0) / rows
sw, sh = cw / GW, chh / GH

lines_ans, lines_txt = [], []
for r in range(rows):
    ans, txt = '', ''
    for c in range(cols):
        samples = [boost(block(c * cw + i * sw, c * cw + (i + 1) * sw,
                               y0 + r * chh + j * sh, y0 + r * chh + (j + 1) * sh))
                   for j in range(GH) for i in range(GW)]
        best = None
        for ch, cov, mean_cov in glyph_items:
            # least squares per channel: sample = bg + (fg - bg) * coverage
            var = sum((k - mean_cov) ** 2 for k in cov)
            err, fg, bg = 0.0, [], []
            for ch_i in range(3):
                vals = [s[ch_i] for s in samples]
                mv = sum(vals) / len(vals)
                if var > 1e-6:
                    slope = sum((k - mean_cov) * (v - mv) for k, v in zip(cov, vals)) / var
                else:
                    slope = 0.0
                b = mv - slope * mean_cov
                f = b + slope
                b = min(255, max(0, b)); f = min(255, max(0, f))
                err += sum((v - (b + (f - b) * k)) ** 2 for k, v in zip(cov, vals))
                fg.append(f); bg.append(b)
            # bias toward plain cells so flat areas stay calm
            if ch != ' ':
                err = err * 1.04 + 40
            if best is None or err < best[0]:
                best = (err, ch, fg, bg)
        _, ch, fg, bg = best
        fr, fgc, fb = (int(v) for v in fg)
        br, bgc, bb = (int(v) for v in bg)
        ans += f'\x1b[38;2;{fr};{fgc};{fb};48;2;{br};{bgc};{bb}m{ch}'
        txt += ch
    lines_ans.append(ans + '\x1b[0m')
    lines_txt.append(txt.rstrip())
    print(f'\r{r + 1}/{rows}', end='', file=sys.stderr)
print(file=sys.stderr)

base = out[:-4] if out.endswith('.ans') else out
open(base + '.ans', 'w').write('\n'.join(lines_ans) + '\n')
open(base + '.txt', 'w').write('\n'.join(lines_txt) + '\n')
print(f'{cols}x{rows}')
