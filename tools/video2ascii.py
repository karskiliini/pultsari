#!/usr/bin/env python3
"""Turn a video clip into colour ASCII art for the browser intro.

    python3 tools/video2ascii.py clip.mp4 web/video/lada.json --caption "PÄÄOSISSA|LADA|Käynnistyy..."

Each frame becomes a grid of characters picked by brightness, each with an
xterm-256 colour. Output: {"fps", "cols", "rows", "caption", "frames": [...]}
where a frame is {"c": characters row by row, "k": base64 colour indices}.
Then list the clip in web/video/index.json to have the intro play it.
Needs only Python 3 and ffmpeg.
"""
import argparse
import base64
import json
import os
import subprocess
import sys

# dark to bright; a space for black keeps the frames light
RAMP = " .,:;-=+*cxoO#%@"


def xterm256(r, g, b):
    """Nearest colour in the xterm 6x6x6 cube or the grey ramp."""
    def cube(v):
        return 0 if v < 48 else 1 if v < 115 else (v - 35) // 40
    ci = 16 + 36 * cube(r) + 6 * cube(g) + cube(b)
    levels = [0, 95, 135, 175, 215, 255]
    cr, cg, cb = levels[cube(r)], levels[cube(g)], levels[cube(b)]
    grey = (r + g + b) // 3
    gi = 232 + min(23, max(0, (grey - 3) // 10))
    gv = 8 + (gi - 232) * 10
    if (r - gv) ** 2 + (g - gv) ** 2 + (b - gv) ** 2 < (r - cr) ** 2 + (g - cg) ** 2 + (b - cb) ** 2:
        return gi
    return ci


def convert(src, dst, cols, fps, seconds, caption):
    # character cells are about twice as tall as wide, so halve the rows
    probe = subprocess.run(
        ["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height",
         "-of", "csv=p=0", src], capture_output=True, text=True, check=True)
    w, h = (int(v) for v in probe.stdout.strip().split(",")[:2])
    rows = max(8, round(cols * h / w / 2))
    cmd = ["ffmpeg", "-v", "error", "-i", src]
    if seconds:
        cmd += ["-t", str(seconds)]
    # raw RGB straight from ffmpeg: no image library needed
    cmd += ["-vf", f"fps={fps},scale={cols}:{rows}:flags=area", "-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
    raw = subprocess.run(cmd, capture_output=True, check=True).stdout

    size = cols * rows * 3
    frames = []
    for start in range(0, len(raw) - size + 1, size):
        px = raw[start:start + size]
        chars = []
        colors = bytearray()
        for i in range(0, size, 3):
            r, g, b = px[i], px[i + 1], px[i + 2]
            lum = 0.2126 * r + 0.7152 * g + 0.0722 * b
            chars.append(RAMP[min(len(RAMP) - 1, int(lum / 256 * len(RAMP)))])
            # lift dark colours a little so the glyphs stay readable on black
            colors.append(xterm256(min(255, r + 25), min(255, g + 25), min(255, b + 25)))
        frames.append({"c": "".join(chars), "k": base64.b64encode(bytes(colors)).decode()})

    out = {"fps": fps, "cols": cols, "rows": rows, "frames": frames}
    if caption:
        who, name, line = (caption.split("|") + ["", "", ""])[:3]
        out["caption"] = {"who": who, "name": name, "line": line}
    os.makedirs(os.path.dirname(dst) or ".", exist_ok=True)
    with open(dst, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, separators=(",", ":"))
    print(f"{dst}: {len(frames)} frames, {cols}x{rows} cells, {os.path.getsize(dst) // 1024} KB")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src", help="input video (mp4, webm, ...)")
    ap.add_argument("dst", help="output JSON, e.g. web/video/lada.json")
    ap.add_argument("--cols", type=int, default=96, help="characters per row (default 96)")
    ap.add_argument("--fps", type=int, default=10, help="frames per second (default 10)")
    ap.add_argument("--seconds", type=float, default=0, help="only convert the first N seconds")
    ap.add_argument("--caption", default="", help='credit text: "WHO|NAME|line"')
    args = ap.parse_args()
    convert(args.src, args.dst, args.cols, args.fps, args.seconds, args.caption)


if __name__ == "__main__":
    sys.exit(main())
