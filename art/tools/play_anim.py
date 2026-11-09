"""Play the animated ad in a truecolor terminal (needs 200x150 cells: shrink the font).
usage: python3 play_anim.py [anim.json] [fps]
Ctrl-C quits.
"""
import json, os, shutil, sys, time

here = os.path.dirname(os.path.abspath(__file__))
path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, '..', 'mainos-anim.json')
fps = float(sys.argv[2]) if len(sys.argv) > 2 else 12
D = json.load(open(path))
cols, rows = D['cols'], D['rows']

def fg(c):
    return '\x1b[38;2;%d;%d;%dm' % tuple(int(h, 16) * 17 for h in c)

out = sys.stdout
size = shutil.get_terminal_size()
if size.columns < cols or size.lines < rows:
    print(f'Terminaali on {size.columns}x{size.lines}, tarvitaan {cols}x{rows}. '
          'Pienennä fonttia (Cmd/Ctrl -) ja yritä uudelleen.')
    sys.exit(1)

# static picture once
buf = ['\x1b[?25l\x1b[2J\x1b[H']
for y in range(rows):
    line, lc = D['static'][y], D['staticColors'][y]
    prev = None
    for x in range(cols):
        c = lc[x * 3:x * 3 + 3]
        if c != prev:
            buf.append(fg(c)); prev = c
        buf.append(line[x])
    if y < rows - 1:
        buf.append('\r\n')
out.write(''.join(buf)); out.flush()

pos = [divmod(i, cols) for i in D['cells']]
try:
    f = 0
    while True:
        t0 = time.time()
        ch, co = D['chars'][f], D['colors'][f]
        buf, prev_pos, prev_col = [], None, None
        for k, (y, x) in enumerate(pos):
            if prev_pos != (y, x):
                buf.append(f'\x1b[{y + 1};{x + 1}H')
            c = co[k * 3:k * 3 + 3]
            if c != prev_col:
                buf.append(fg(c)); prev_col = c
            buf.append(ch[k])
            prev_pos = (y, x + 1)
        out.write(''.join(buf)); out.flush()
        f = (f + 1) % D['frames']
        time.sleep(max(0, 1 / fps - (time.time() - t0)))
except KeyboardInterrupt:
    pass
finally:
    out.write(f'\x1b[0m\x1b[{rows};1H\n\x1b[?25h'); out.flush()
