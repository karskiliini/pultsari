"""Paste hand+bottle region from old.bmp onto new.bmp with feathered edges.
usage: composite.py old.bmp new.bmp out.bmp x0 y0 x1 y1 dx dy feather
(x0..x1, y0..y1 = source rect in old image; dx,dy = shift when pasting)
"""
import sys, struct
old, new, out = sys.argv[1:4]
x0, y0, x1, y1, dx, dy, fe = map(int, sys.argv[4:11])
A = open(old, 'rb').read()
B = bytearray(open(new, 'rb').read())
off = struct.unpack_from('<I', B, 10)[0]
W, H = struct.unpack_from('<ii', B, 18)
H = abs(H)  # top-down (sips writes negative height)
stride = (W * 3 + 3) & ~3
for y in range(y0, y1):
    for x in range(x0, x1):
        d = min(x - x0, x1 - 1 - x, y - y0, y1 - 1 - y)
        a = min(1.0, (d + 1) / fe)
        si = off + y * stride + x * 3
        tx, ty = x + dx, y + dy
        if not (0 <= tx < W and 0 <= ty < H):
            continue
        ti = off + ty * stride + tx * 3
        for k in range(3):
            B[ti + k] = int(B[ti + k] * (1 - a) + A[si + k] * a)
open(out, 'wb').write(B)
