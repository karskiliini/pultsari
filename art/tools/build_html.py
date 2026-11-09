"""Build a self-contained animated HTML page from the animation JSON.
usage: python3 build_html.py anim.json out.html
"""
import sys

data = open(sys.argv[1]).read()
page = """<!doctype html>
<html lang="fi"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Pultsari</title>
<style>
html,body{margin:0;height:100%;background:#000}
body{display:flex;align-items:center;justify-content:center;overflow:hidden}
pre{margin:0;font-family:Menlo,Consolas,"DejaVu Sans Mono",monospace;line-height:1.2;cursor:pointer;
  filter:brightness(1.6) saturate(1.2)}  /* thin glyphs on black read darker than the source pixels */
b,i{font-weight:normal;font-style:normal}
</style></head><body><pre id="art" title="Klikkaa: tauko / jatka"></pre>
<script>
const D = __DATA__;
const pre = document.getElementById('art');
const cols = D.cols, rows = D.rows;
const dyn = new Map(D.cells.map((c, k) => [c, k]));
const hex = s => '#' + s;
// build static runs + one element per animated cell
const frag = document.createDocumentFragment();
const els = new Array(D.cells.length);
for (let y = 0; y < rows; y++) {
  const line = D.static[y], lc = D.staticColors[y];
  let run = '', runCol = null;
  const flush = () => { if (run) { const b = document.createElement('b'); b.style.color = hex(runCol); b.textContent = run; frag.appendChild(b); run = ''; } };
  for (let x = 0; x < cols; x++) {
    const idx = y * cols + x, col = lc.substr(x * 3, 3);
    if (dyn.has(idx)) {
      flush();
      const e = document.createElement('i'); e.textContent = line[x]; e.style.color = hex(col);
      els[dyn.get(idx)] = e; frag.appendChild(e);
    } else {
      if (col !== runCol) { flush(); runCol = col; }
      run += line[x];
    }
  }
  flush();
  frag.appendChild(document.createTextNode('\\n'));
}
pre.appendChild(frag);
function fit() {
  // Monospace glyphs are ~0.6em wide; line-height 1.2em keeps the 1:2 cell shape.
  const fs = Math.min(innerWidth / (cols * 0.6), innerHeight / (rows * 1.2)) * 0.98;
  pre.style.fontSize = fs + 'px';
}
addEventListener('resize', fit); fit();
let f = 0, paused = matchMedia('(prefers-reduced-motion: reduce)').matches, last = 0;
function draw(fr) {
  const ch = D.chars[fr], co = D.colors[fr];
  for (let k = 0; k < els.length; k++) {
    const e = els[k];
    e.textContent = ch[k];
    e.style.color = '#' + co.substr(k * 3, 3);
  }
}
function tick(t) {
  if (!paused && t - last > 1000 / 12) { last = t; draw(f); f = (f + 1) % D.frames; }
  requestAnimationFrame(tick);
}
pre.addEventListener('click', () => { paused = !paused; });
requestAnimationFrame(tick);
</script></body></html>
"""
open(sys.argv[2], 'w').write(page.replace('__DATA__', data))
