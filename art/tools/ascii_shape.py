"""Shape-matched colour ASCII art: for every cell pick the character whose
glyph shape best matches the picture under it, plus a foreground and a
background colour. Much more detail than a brightness ramp at low resolutions.

usage: ascii_shape.py image.bmp COLS OUT.ans [crop_y0 crop_y1]
  writes OUT.ans (ANSI truecolor fg+bg) and OUT.txt (characters only)

Glyph shapes (glyphs4x8.json) are the ink coverage of each printable ASCII
character on a 4x8 grid, measured from Menlo with the cell squeezed to 1:2.
Also used as a module by gen_shape_anim.py.
"""
import json, os, struct, sys

here = os.path.dirname(os.path.abspath(__file__))
GLYPHS = json.load(open(os.path.join(here, 'glyphs4x8.json')))
# only shapes that read as picture structure (lines, edges, blobs); busy
# letters like g, M or & turn into noise at this size
ALLOWED = ' .,:;\'"-_=+~^|/\\()[]<>!*#%oO8LJTVYXAUH'
GLYPHS = {c: v for c, v in GLYPHS.items() if c in ALLOWED}
GW, GH = 4, 8
GLYPH_ITEMS = [(ch, cov, sum(cov) / len(cov)) for ch, cov in GLYPHS.items()]


class Image:
    """A 24-bit BMP (as written by macOS sips)."""

    def __init__(self, path):
        self.data = open(path, 'rb').read()
        self.off = struct.unpack_from('<I', self.data, 10)[0]
        W, H = struct.unpack_from('<ii', self.data, 18)
        self.topdown = H < 0
        self.W, self.H = W, abs(H)
        self.stride = (W * 3 + 3) & ~3

    def block(self, x0, x1, ya, yb):
        """mean RGB of an image rectangle (sampled every 2 px)"""
        d, R, G, B, n = self.data, 0, 0, 0, 0
        for y in range(int(ya), max(int(ya) + 1, int(yb)), 2):
            yy = y if self.topdown else self.H - 1 - y
            base = self.off + yy * self.stride
            for x in range(int(x0), max(int(x0) + 1, int(x1)), 2):
                i = base + x * 3
                B += d[i]; G += d[i + 1]; R += d[i + 2]; n += 1
        return (R / n, G / n, B / n)


def boost(c, k=1.25, gamma=0.85):
    """brighten a little: terminal glyphs on black read darker than pixels"""
    return tuple(min(255.0, 255 * ((v / 255) ** gamma) * k) for v in c)


class Grid:
    """Cell layout of an image: COLS columns of 1:2 cells, 4x8 samples each."""

    def __init__(self, img, cols, y0=0, y1=None):
        self.img, self.cols = img, cols
        self.y0 = y0
        self.y1 = img.H if y1 is None else y1
        self.rows = round(cols * (self.y1 - self.y0) / img.W / 2)
        self.cw, self.ch = img.W / cols, (self.y1 - self.y0) / self.rows
        self.sw, self.sh = self.cw / GW, self.ch / GH

    def sample_points(self, r, c):
        """image-pixel centres of the 4x8 samples of cell (r, c)"""
        return [(c * self.cw + (i + 0.5) * self.sw, self.y0 + r * self.ch + (j + 0.5) * self.sh)
                for j in range(GH) for i in range(GW)]

    def samples(self, r, c):
        cw, ch, sw, sh, y0 = self.cw, self.ch, self.sw, self.sh, self.y0
        return [boost(self.img.block(c * cw + i * sw, c * cw + (i + 1) * sw,
                                     y0 + r * ch + j * sh, y0 + r * ch + (j + 1) * sh))
                for j in range(GH) for i in range(GW)]


def match(samples):
    """best (char, fg, bg) for 32 RGB samples"""
    best = None
    for ch, cov, mean_cov in GLYPH_ITEMS:
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
    return ch, tuple(int(v) for v in fg), tuple(int(v) for v in bg)


def ansi(ch, fg, bg):
    return f'\x1b[38;2;{fg[0]};{fg[1]};{fg[2]};48;2;{bg[0]};{bg[1]};{bg[2]}m{ch}'


def main():
    path, cols, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    img = Image(path)
    grid = Grid(img, cols,
                int(sys.argv[4]) if len(sys.argv) > 4 else 0,
                int(sys.argv[5]) if len(sys.argv) > 5 else None)
    lines_ans, lines_txt = [], []
    for r in range(grid.rows):
        ans, txt = '', ''
        for c in range(cols):
            ch, fg, bg = match(grid.samples(r, c))
            ans += ansi(ch, fg, bg)
            txt += ch
        lines_ans.append(ans + '\x1b[0m')
        lines_txt.append(txt.rstrip())
        print(f'\r{r + 1}/{grid.rows}', end='', file=sys.stderr)
    print(file=sys.stderr)
    base = out[:-4] if out.endswith('.ans') else out
    open(base + '.ans', 'w').write('\n'.join(lines_ans) + '\n')
    open(base + '.txt', 'w').write('\n'.join(lines_txt) + '\n')
    print(f'{cols}x{grid.rows}')


if __name__ == '__main__':
    main()
