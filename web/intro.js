// Browser intro: "Russki Mir" gets vomited on and the vomit, simulated as a
// simple particle fluid, oozes off the letters and drips into "Русский Мир"
// on the line below. Space skips straight to the game.
(function () {
    const CELL = 2; // fluid resolution in CSS pixels: a chunky, pixel-exact look

    function play(container, onDone) {
        const W = container.clientWidth;
        const H = container.clientHeight;
        const dpr = window.devicePixelRatio || 1;
        // the canvas reaches past the screen's bottom border down to the window edge,
        // so the vomit can pile up on the border and ooze over it, out of sight
        const rect = container.getBoundingClientRect();
        const HT = Math.max(H + 40, Math.round(window.innerHeight - rect.top));

        const canvas = document.createElement('canvas');
        canvas.width = Math.round(W * dpr);
        canvas.height = Math.round(HT * dpr);
        Object.assign(canvas.style, { position: 'fixed', left: rect.left + 'px', top: rect.top + 'px',
                                      width: W + 'px', height: HT + 'px', background: 'transparent', zIndex: 50 });
        document.body.appendChild(canvas);
        const ctx = canvas.getContext('2d');
        ctx.scale(dpr, dpr);

        // ---- layout ----
        const fs = Math.max(22, Math.min(46, W / 19));
        const font = `bold ${fs}px ui-monospace, "Cascadia Code", Menlo, monospace`;
        ctx.font = font;
        const titleText = 'PПLTSAЯI 2: ';
        const latinText = 'Russki Mir';
        const cyrText = 'Русский Мир';
        const titleW = ctx.measureText(titleText).width;
        const latinW = ctx.measureText(latinText).width;
        const startX = Math.round((W - titleW - latinW) / 2);
        const latinX = startX + titleW;
        const baseY = Math.round(H * 0.3);
        const cyrW = ctx.measureText(cyrText).width;
        const cyrX = Math.round(latinX + (latinW - cyrW) / 2);
        const cyrY = Math.round(baseY + fs * 1.15); // the very next line
        const floorY = Math.round(cyrY + fs * 1.6);

        // ---- masks at fluid resolution ----
        const FW = Math.ceil(W / CELL);
        const FH = Math.ceil(HT / CELL);
        function textMask(text, x, y) {
            const c = document.createElement('canvas');
            c.width = FW; c.height = FH;
            const m = c.getContext('2d');
            m.scale(1 / CELL, 1 / CELL);
            m.font = font;
            m.fillStyle = '#fff';
            m.fillText(text, x, y);
            const data = m.getImageData(0, 0, FW, FH).data;
            const mask = new Uint8Array(FW * FH);
            let minY = FH, maxY = 0, minX = FW, maxX = 0, count = 0;
            for (let i = 0; i < FW * FH; i++) {
                if (data[i * 4 + 3] > 110) {
                    mask[i] = 1; count++;
                    const cx = i % FW, cy = (i / FW) | 0;
                    if (cy < minY) minY = cy; if (cy > maxY) maxY = cy;
                    if (cx < minX) minX = cx; if (cx > maxX) maxX = cx;
                }
            }
            return { mask, minY, maxY, minX, maxX, count };
        }
        const latin = textMask(latinText, latinX, baseY);
        const title = textMask(titleText, startX, baseY);
        const cyr = textMask(cyrText, cyrX, cyrY);
        const filled = new Uint8Array(FW * FH);
        let filledCount = 0;
        // the vomit first piles up on the screen's bottom border, then oozes over it and drips away
        const floorCell = Math.floor(H / CELL) - 1;
        const smear = new Uint8Array(FW * 3); // stains on the border rows
        const drips = [];   // slime strands hanging from the border
        const drops = [];   // drops that let go and fall

        // ---- fluid: particles with gravity, pressure, cohesion and viscosity ----
        const MAX = 5000;
        let impacts = 0;   // splats this frame, for the sound
        const px = new Float32Array(MAX), py = new Float32Array(MAX);
        const vx = new Float32Array(MAX), vy = new Float32Array(MAX);
        // 0 = watery, 1 = thick slime: every splash is half water, the rest stays thick
        const thick = new Float32Array(MAX);
        const fly = new Uint8Array(MAX);     // >0: airborne jet, frames left
        const rest = new Uint16Array(MAX);   // frames spent lying on the border
        let n = 0;
        const G = 0.03, R0 = 1.15, H_ = 2.3, K = 0.09, COH = 0.009, VISC = 0.13;
        const grid = new Map();

        function emit(x, y, ux, uy, th = 1) {
            if (n >= MAX) return;
            px[n] = x; py[n] = y; vx[n] = ux; vy[n] = uy; thick[n] = th; rest[n] = 0; fly[n] = 0; n++;
        }
        function remove(i) {
            n--;
            px[i] = px[n]; py[i] = py[n]; vx[i] = vx[n]; vy[i] = vy[n]; thick[i] = thick[n]; rest[i] = rest[n]; fly[i] = fly[n];
        }
        const at = (mask, x, y) => {
            const cx = x | 0, cy = y | 0;
            return cx >= 0 && cy >= 0 && cx < FW && cy < FH && mask[cy * FW + cx] === 1;
        };

        function step() {
            grid.clear();
            for (let i = 0; i < n; i++) {
                const key = ((px[i] / H_) | 0) * 4096 + ((py[i] / H_) | 0);
                let bucket = grid.get(key);
                if (!bucket) grid.set(key, bucket = []);
                bucket.push(i);
            }
            for (let i = 0; i < n; i++) {
                const gx = (px[i] / H_) | 0, gy = (py[i] / H_) | 0;
                let ax = 0, ay = G;
                // in the air the jet is pure ballistics: it stays a tight tube until it hits something
                if (!fly[i]) for (let ox = -1; ox <= 1; ox++) for (let oy = -1; oy <= 1; oy++) {
                    const bucket = grid.get((gx + ox) * 4096 + gy + oy);
                    if (!bucket) continue;
                    for (const j of bucket) {
                        if (j === i) continue;
                        const dx = px[j] - px[i], dy = py[j] - py[i];
                        const d2 = dx * dx + dy * dy;
                        if (d2 > H_ * H_ || d2 < 1e-6) continue;
                        const d = Math.sqrt(d2);
                        // slime clings together and drags; water barely does
                        const th = (thick[i] + thick[j]) * 0.5;
                        const f = d < R0 ? -K * (R0 - d) : COH * (0.3 + 1.7 * th) * (1 - d / H_);
                        ax += f * dx / d; ay += f * dy / d;
                        const w = VISC * (0.25 + 1.75 * th) * (1 - d / H_);
                        ax += (vx[j] - vx[i]) * w; ay += (vy[j] - vy[i]) * w;
                    }
                }
                vx[i] += ax; vy[i] += ay;
                const t = thick[i];
                // the vomit sticks to the letters and oozes off their bottoms: slime slowly, water quickly
                if (at(latin.mask, px[i], py[i]) || at(latin.mask, px[i], py[i] + 1)) {
                    vx[i] *= 0.93 - 0.23 * t; vy[i] *= 0.95 - 0.2 * t;
                    if (fly[i]) impacts++;
                    fly[i] = 0;
                } else if (at(title.mask, px[i], py[i]) || at(title.mask, px[i], py[i] + 1)) {
                    // flying slime catches on the title here and there along its length; caught slime
                    // clings and oozes down slowly, water mostly flies on
                    if (fly[i]) {
                        // impact: slime mostly catches, water splashes back off the letter
                        if (Math.random() < (t > 0.5 ? 0.35 : 0.1)) { fly[i] = 0; vx[i] *= 0.3; vy[i] *= 0.2; impacts++; }
                        else if (Math.random() < 0.3) { vy[i] *= -0.35; vx[i] *= 0.7; }
                    } else { vx[i] *= 0.55 - 0.3 * t; vy[i] *= 0.6 - 0.35 * t; }
                }
                // the jet flies freely until it hits something; after that slime drags on everything
                if (fly[i]) {
                    if (--fly[i] === 0 || rest[i]) fly[i] = 0;
                    continue;
                }
                vx[i] *= 1 - 0.012 * t; vy[i] *= 1 - 0.035 * t;
                const sp = Math.hypot(vx[i], vy[i]), top = 2.8 - 1.3 * t;
                if (sp > top) { vx[i] *= top / sp; vy[i] *= top / sp; }
            }
            for (let i = n - 1; i >= 0; i--) {
                px[i] += vx[i]; py[i] += vy[i];
                if (px[i] < 1) { px[i] = 1; vx[i] *= -0.3; }
                if (px[i] > FW - 2) { px[i] = FW - 2; vx[i] *= -0.3; }
                if (py[i] >= floorCell) {
                    if (rest[i] === 0) impacts++;
                    // lands on the border and pools there
                    py[i] = floorCell; if (vy[i] > 0) vy[i] *= -0.05; vx[i] *= 0.975;
                    const sx = px[i] | 0;
                    if (rest[i] === 0 && Math.random() < 0.25) smear[(Math.random() < 0.5 ? FW : 0) + sx] = 1 + (thick[i] > 0.5);
                    if (rest[i] < 65000) rest[i]++;
                    // the puddle creeps toward the nearest hanging drip, which drains it
                    let best = 1e9, near = 0;
                    for (const d of drips) { const dd = Math.abs(d.x / CELL - px[i]); if (dd < best) { best = dd; near = d.x / CELL; } }
                    if (best < 30) vx[i] += Math.sign(near - px[i]) * 0.006;
                }
                // drips that land in the Cyrillic letters set there and form the word
                const cx = px[i] | 0, cy = py[i] | 0;
                if (at(cyr.mask, cx, cy) && !filled[cy * FW + cx] && Math.random() < 0.45) {
                    filled[cy * FW + cx] = 1; filledCount++;
                    remove(i);
                    dripSound();
                }
            }
        }

        // the poured word fills like liquid: from each cell a drip filled, downward fast, sideways slower
        function spreadFill(t) {
            const late = t > 13 ? 3 : 1; // toward the end it hurries a little
            const fresh = [];
            for (const h of holes) {
                if (filled[h]) continue;
                const up = h - FW, down = h + FW;
                let p = 0;
                if (up >= 0 && filled[up]) p += 0.05;
                if (filled[h - 1] || filled[h + 1]) p += 0.012;
                if (down < FW * FH && filled[down]) p += 0.003;
                if (p && Math.random() < p * late) fresh.push(h);
            }
            for (const h of fresh) { filled[h] = 1; filledCount++; }
        }

        // feed drips from the bottom of the Latin letters above unfilled Cyrillic cells
        const holes = [];
        for (let i = 0; i < FW * FH; i++) if (cyr.mask[i]) holes.push(i);
        function feed(count) {
            for (let k = 0; k < count; k++) {
                const h = holes[(Math.random() * holes.length) | 0];
                if (filled[h]) continue;
                const hx = h % FW;
                const sx = latin.minX + (hx - cyr.minX) * (latin.maxX - latin.minX) / Math.max(1, cyr.maxX - cyr.minX);
                emit(sx + Math.random() - 0.5, latin.maxY + Math.random(), 0, 0.05, 0.85);
            }
        }

        let lastDrip = 0;
        function dripSound() {
            const now = performance.now();
            if (now - lastDrip > 220 && window.SFX) { lastDrip = now; SFX.play('drip', 0.25 + Math.random() * 0.25); }
        }

        // ---- rendering ----
        const fluidCanvas = document.createElement('canvas');
        fluidCanvas.width = FW; fluidCanvas.height = FH;
        const fctx = fluidCanvas.getContext('2d');
        const img = fctx.createImageData(FW, FH);
        const density = new Float32Array(FW * FH);
        const thickness = new Float32Array(FW * FH);

        // the poured word: a green gradient from bright bile at the top to dark at the bottom
        function wordColor(cy, cx) {
            const t = (cy - cyr.minY) / Math.max(1, cyr.maxY - cyr.minY);
            const shimmer = ((cx * 7 + cy * 13) % 5) * 4;
            return [Math.round(196 - 140 * t + shimmer), Math.round(238 - 110 * t + shimmer), Math.round(92 - 72 * t)];
        }

        function drawFluid() {
            density.fill(0);
            thickness.fill(0);
            for (let i = 0; i < n; i++) {
                const cx = px[i] | 0, cy = py[i] | 0;
                for (let oy = -1; oy <= 1; oy++) for (let ox = -1; ox <= 1; ox++) {
                    const x = cx + ox, y = cy + oy;
                    if (x < 0 || y < 0 || x >= FW || y >= FH) continue;
                    const w = (ox === 0 && oy === 0) ? 1 : (ox === 0 || oy === 0) ? 0.55 : 0.3;
                    density[y * FW + x] += w;
                    thickness[y * FW + x] += w * thick[i];
                }
            }
            const d = img.data;
            for (let i = 0; i < FW * FH; i++) {
                const o = i * 4;
                const row = (i / FW) | 0;
                const sr = row - floorCell;
                if (sr >= 0 && sr < 3 && density[i] < 0.85 && smear[sr * FW + (i % FW)]) {
                    // stains left on the bottom border: slime darker than the watery splashes
                    const slime = smear[sr * FW + (i % FW)] === 2;
                    d[o] = slime ? 70 : 140; d[o + 1] = slime ? 105 : 160; d[o + 2] = slime ? 20 : 70; d[o + 3] = 210;
                    continue;
                }
                if (filled[i]) {
                    const [r, g, b] = wordColor((i / FW) | 0, i % FW);
                    d[o] = r; d[o + 1] = g; d[o + 2] = b; d[o + 3] = 255;
                } else if (density[i] > 0.85) {
                    // slime is dark and deep green, water a pale watery yellow-green; deeper is darker
                    const t = Math.min(1, (density[i] - 0.85) / 2.5);
                    const s = thickness[i] / density[i];
                    const r = 205 - 60 * s, g = 220 - 20 * s, b = 140 - 110 * s;
                    // relief: light from the upper left on the slopes of the blob, shadow on the far side,
                    // and a wet gloss on the thickest spots, so the thick parts seem to bulge out
                    const col = i % FW;
                    const dl = col > 0 ? density[i - 1] : 0, dr = col < FW - 1 ? density[i + 1] : 0;
                    const du = i >= FW ? density[i - FW] : 0, dd = i + FW < FW * FH ? density[i + FW] : 0;
                    const slope = ((dr - dl) * 0.6 + (dd - du) * 0.8) / (1 + density[i]);
                    const light = Math.max(-0.45, Math.min(0.5, slope * 0.9));
                    const shade = (1 - (0.25 + 0.4 * s) * t) * (1 + light);
                    const gloss = t > 0.55 && light > 0.12 ? 70 * (t - 0.55) * light * 4 : 0;
                    d[o] = Math.min(255, Math.round(r * shade + gloss));
                    d[o + 1] = Math.min(255, Math.round(g * shade + gloss));
                    d[o + 2] = Math.min(255, Math.round(b * shade + gloss * 0.6));
                    d[o + 3] = Math.round(255 * (0.55 + 0.45 * s));
                } else {
                    d[o + 3] = 0;
                }
            }
            fctx.putImageData(img, 0, 0);
            ctx.imageSmoothingEnabled = false;
            ctx.drawImage(fluidCanvas, 0, 0, FW * CELL, FH * CELL);
        }


        // ---- drips: the pool on the border sags into lips and hangs down in slime strands ----
        const lipTop = H + 1;                  // just under the border line
        const colCount = new Float32Array(FW);
        const lip = new Float32Array(FW);
        function updateDrips() {
            colCount.fill(0);
            for (let i = 0; i < n; i++) if (rest[i] > 0) colCount[px[i] | 0] += 1;

            // new strands start where the puddle is deep
            if (drips.length < 11 && Math.random() < 0.05) {
                for (let tries = 0; tries < 12; tries++) {
                    const i = (Math.random() * n) | 0;
                    if (n === 0 || rest[i] < 30 || colCount[px[i] | 0] < 2) continue;
                    const x = px[i] * CELL;
                    if (drips.some((d) => Math.abs(d.x - x) < 16)) continue;
                    // the strand inherits the puddle's flow under it
                    let mv = 0, mc = 0;
                    for (let j = 0; j < n; j++) if (rest[j] > 0 && Math.abs(px[j] - px[i]) < 4) { mv += vx[j]; mc++; }
                    mv = mc ? mv / mc : 0;
                    drips.push({ x, len: 0, w: 3 + Math.random() * Math.random() * 9, vol: 3, starve: 0,
                                 ox: 0, ovx: mv * CELL * 1.6 + (Math.random() - 0.5) * 0.4, bvx: mv * CELL * 0.6,
                                 maxLen: 25 + Math.random() * Math.max(30, (HT - H) * 0.8),
                                 speed: 0.1 + Math.random() * 0.16 });
                    break;
                }
            }
            for (let k = drips.length - 1; k >= 0; k--) {
                const d = drips[k];
                // drain the puddle slowly into the strand
                let fed = false;
                if (Math.random() < 0.09) {
                    let best = 1e9, bi = -1;
                    for (let i = 0; i < n; i++) {
                        if (rest[i] < 10) continue;
                        const dd = Math.abs(px[i] * CELL - d.x);
                        if (dd < best) { best = dd; bi = i; }
                    }
                    if (bi >= 0 && best < 3 * CELL + d.w) { remove(bi); d.vol += 1; fed = true; }
                }
                d.starve = fed ? 0 : d.starve + 1;

                // the root slides with the puddle; the tip swings like a slow pendulum
                d.x += d.bvx; d.bvx *= 0.992;
                if (d.x < 2) { d.x = 2; d.bvx = Math.abs(d.bvx); }
                if (d.x > W - 2) { d.x = W - 2; d.bvx = -Math.abs(d.bvx); }
                const spring = 0.06 / (d.len + 8);
                d.ovx += -d.ox * spring + (Math.random() - 0.5) * 0.03;
                // surface tension: neighbouring strands and the pool around pull the tip sideways
                for (const o of drips) {
                    if (o === d) continue;
                    const dx = (o.x + o.ox) - (d.x + d.ox);
                    if (Math.abs(dx) < 28) d.ovx += Math.sign(dx) * 0.006 * (1 - Math.abs(dx) / 28);
                }
                d.ovx *= 0.988;
                d.ox += d.ovx;
                const lim = d.len * 0.55;
                if (d.ox > lim) { d.ox = lim; d.ovx *= -0.3; } else if (d.ox < -lim) { d.ox = -lim; d.ovx *= -0.3; }
                d.len += d.speed * (0.35 + Math.min(1.6, d.vol / 14));
                d.r = Math.min(d.w * 0.95, 1.4 + Math.sqrt(d.vol) * 0.7);
                if ((d.vol > 18 + d.w * 4 && Math.random() < 0.015) || d.len > d.maxLen) {
                    // the heavy drop lets go; the strand snaps back up and keeps oozing
                    drops.push({ x: d.x + d.ox, y: lipTop + d.len, r: d.r, vy: d.speed, vx: d.ovx });
                    d.vol *= 0.25;
                    d.ovx *= -0.5; // recoil
                    d.len *= 0.35 + Math.random() * 0.3;
                    d.maxLen = 25 + Math.random() * Math.max(30, (HT - H) * 0.8);
                }
                if (d.starve > 90 && d.vol < 4) {
                    // the puddle is gone here: the strand thins to a hair and breaks
                    d.w *= 0.985;
                    if (d.w < 0.7) { drops.push({ x: d.x + d.ox, y: lipTop + d.len, r: Math.max(1, d.r * 0.7), vy: 0.3, vx: d.ovx }); drips.splice(k, 1); }
                }
            }
            for (let a = drips.length - 1; a >= 0; a--) {
                for (let b = a - 1; b >= 0; b--) {
                    const A = drips[a], B = drips[b];
                    if (Math.abs(A.x - B.x) > (A.w + B.w) * 0.5) continue;
                    B.vol += A.vol; B.w = Math.max(A.w, B.w) * 1.1; B.len = Math.max(A.len, B.len);
                    B.ovx = (A.ovx + B.ovx) * 0.5; drips.splice(a, 1); break;
                }
            }
            for (let k = drops.length - 1; k >= 0; k--) {
                const p = drops[k];
                p.vy += 0.16; p.y += p.vy; p.x += p.vx; p.vx *= 0.996;
                if (p.y > HT + 20) drops.splice(k, 1);
            }
        }

        const SLIME = '#4b7414', GLOSS = 'rgba(205,240,120,0.5)';
        function drawDrips() {
            // the lip: the puddle bulging over the edge, deeper where it is thicker and around each strand
            for (let x = 0; x < FW; x++) {
                let sum = 0;
                for (let o = -4; o <= 4; o++) { const c = x + o; if (c >= 0 && c < FW) sum += colCount[c] * (1 - Math.abs(o) / 5); }
                lip[x] = sum > 0.5 ? Math.min(22, Math.sqrt(sum) * 3.2) : 0;
            }
            for (const d of drips) {
                const c = d.x / CELL;
                for (let x = Math.max(0, (c - 10) | 0); x < Math.min(FW, c + 10); x++) {
                    const f = Math.exp(-Math.pow((x - c) * CELL / (d.w * 1.7), 2));
                    lip[x] = Math.max(lip[x], lip[x] * 0.4 + d.w * 2.4 * f);
                }
            }
            const lipGrad = ctx.createLinearGradient(0, lipTop, 0, lipTop + 22);
            lipGrad.addColorStop(0, '#7aa82c');
            lipGrad.addColorStop(0.35, '#557f16');
            lipGrad.addColorStop(1, '#2c4606');
            ctx.fillStyle = lipGrad;
            ctx.beginPath();
            let open = false;
            for (let x = 0; x <= FW; x++) {
                const h = x < FW ? lip[x] : 0;
                if (h > 0.4 && !open) { ctx.moveTo(x * CELL, lipTop); open = true; }
                if (open) ctx.lineTo(x * CELL + CELL / 2, lipTop + h);
                if (h <= 0.4 && open) { ctx.lineTo(x * CELL, lipTop); ctx.closePath(); open = false; }
            }
            ctx.fill();

            // the strands: wide where they leave the lip, thin in the middle, a round drop at the tip
            for (const d of drips) {
                const top = lipTop + (lip[Math.min(FW - 1, (d.x / CELL) | 0)] || 0) * 0.6;
                const tip = lipTop + d.len;
                if (tip <= top) continue;
                const w0 = d.w * 1.4, w1 = Math.max(0.9, d.w * 0.55);
                const tx = d.x + d.ox, L = tip - top, ty = tip - d.r * 0.6;
                // round in cross-section: lit on the left, in shadow on the right
                const g = ctx.createLinearGradient(Math.min(d.x, tx) - w0, 0, Math.max(d.x, tx) + w0, 0);
                g.addColorStop(0, '#3d600c');
                g.addColorStop(0.3, '#7cab30');
                g.addColorStop(0.55, '#4f7a14');
                g.addColorStop(1, '#223a04');
                ctx.fillStyle = g;
                // the strand hangs straight from the root and bends toward the swinging tip
                ctx.beginPath();
                ctx.moveTo(d.x - w0 / 2, top);
                ctx.bezierCurveTo(d.x - w1 / 2, top + L * 0.35, tx - w1 / 2, top + L * 0.6, tx - w1 / 2, ty);
                ctx.lineTo(tx + w1 / 2, ty);
                ctx.bezierCurveTo(tx + w1 / 2, top + L * 0.6, d.x + w1 / 2, top + L * 0.35, d.x + w0 / 2, top);
                ctx.closePath();
                ctx.fill();
                ctx.beginPath();
                ctx.ellipse(tx, tip, d.r, d.r * 1.15, d.ovx * 0.15, 0, Math.PI * 2);
                const bulb = ctx.createRadialGradient(tx - d.r * 0.4, tip - d.r * 0.45, d.r * 0.1, tx, tip, d.r * 1.2);
                bulb.addColorStop(0, '#a6d65a');
                bulb.addColorStop(0.45, '#5a8618');
                bulb.addColorStop(1, '#243c05');
                ctx.fillStyle = bulb;
                ctx.fill();
            }
            for (const p of drops) {
                ctx.beginPath();
                ctx.ellipse(p.x, p.y, p.r * 0.85, p.r * (1.1 + Math.min(0.8, p.vy * 0.08)), 0, 0, Math.PI * 2);
                const dg = ctx.createRadialGradient(p.x - p.r * 0.35, p.y - p.r * 0.4, p.r * 0.1, p.x, p.y, p.r * 1.3);
                dg.addColorStop(0, '#a6d65a');
                dg.addColorStop(0.5, '#507c14');
                dg.addColorStop(1, '#243c05');
                ctx.fillStyle = dg;
                ctx.fill();
            }
            // a wet highlight down the left side of each strand
            ctx.strokeStyle = GLOSS;
            ctx.lineWidth = 0.8;
            for (const d of drips) {
                const tip = lipTop + d.len;
                if (d.len < 6) continue;
                const tx = d.x + d.ox;
                ctx.beginPath();
                ctx.moveTo(d.x - d.w * 0.25, lipTop + 3);
                ctx.bezierCurveTo(d.x - d.w * 0.2, lipTop + d.len * 0.35, tx - d.w * 0.12, lipTop + d.len * 0.6, tx - d.w * 0.12, tip - d.r * 0.8);
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(tx - d.r * 0.35, tip - d.r * 0.35, d.r * 0.3, 0, Math.PI * 2);
                ctx.stroke();
            }
        }

        function text(str, x, y, color, size) {
            ctx.font = size ? `bold ${size}px ui-monospace, Menlo, monospace` : font;
            ctx.textAlign = 'left';
            ctx.textBaseline = 'alphabetic';
            ctx.fillStyle = color;
            ctx.fillText(str, x, y);
        }

        // ---- timeline ----
        let t0 = performance.now();
        let stage = 0;
        let doneAt = 0;
        let finished = false;
        let montageAt = 0;
        let montageDone = false;
        let stopTheme = () => {};
        const cues = [
            [2.6, () => window.SFX && SFX.play('cough', 1)],
            [3.1, () => { window.SFX && SFX.play('vomit', 1); startJet(3, 42, false, 0.0, 0.45); }],
            [5.0, () => { window.SFX && SFX.play('vomit2', 1); startJet(2, 40, false, 0.45, 0.8); }],
            // the third heave is the longest: a long, wavering jet
            [7.0, () => { window.SFX && SFX.play('vomit', 1); startJet(4, 44, true, 0.1, 1.0); }],
            [7.9, () => window.SFX && SFX.play('vomit2', 0.8)],
        ];

        // Vomit from one mouth just above the top edge. Not a hose: it comes out at low pressure in a
        // thick, lumpy gush that mostly falls, in several retches per heave. The drunk turns his head
        // as he goes, so where it lands moves across the title.
        const mouthX = (startX + titleW * 0.2) / CELL, mouthY = -8;
        let pulses = [], jetT = 0, heaveLong = false, heaveSweep = [0, 1];
        function startJet(retches, perFrame, long, sweepFrom, sweepTo) {
            pulses = []; jetT = 0; heaveLong = long; heaveSweep = [sweepFrom, sweepTo];
            let at = 0;
            for (let r = 0; r < retches; r++) {
                const dur = (long && r === retches - 1 ? 34 : 14) + Math.random() * 12;  // the last one drags on
                pulses.push({ at, dur, rate: perFrame * (0.7 + Math.random() * 0.6) });
                at += dur + 6 + Math.random() * 9;                                       // a gasp between
            }
        }
        function jetFrame() {
            if (!pulses.length) return;
            jetT++;
            const end = pulses[pulses.length - 1];
            if (jetT > end.at + end.dur) { pulses = []; return; }
            const pulse = pulses.find((p) => jetT >= p.at && jetT < p.at + p.dur);
            if (!pulse) return; // between retches
            const k = (jetT - pulse.at) / pulse.dur;
            const progress = jetT / (end.at + end.dur);
            // each retch surges and dies away, with lumps in it
            const surge = Math.sin(Math.PI * Math.min(1, k * 1.3)) * (0.55 + Math.random() * 0.9);
            const count = Math.round(pulse.rate * surge);
            const sweep = heaveSweep[0] + (heaveSweep[1] - heaveSweep[0]) * progress;
            const hitX = (startX + (titleW + latinW) * sweep) / CELL;
            const hitY = (baseY - fs * 0.4) / CELL;
            // low pressure: a long, falling flight; weaker at the start and end of each retch
            const air = 58 + 14 * (1 - surge);
            const ux = (hitX - mouthX) / air * (0.85 + 0.3 * surge);
            const uy = (hitY - mouthY) / air - 0.5 * G * air;
            // a thick stream, about as wide as a mouth, and never even
            const width = 9 + 7 * Math.random() + 6 * surge;
            for (let c = 0; c < count; c++) {
                const lead = Math.random() * 2;
                const vxk = ux * (1 + (Math.random() - 0.5) * 0.16);
                const vyk = uy + (Math.random() - 0.5) * 0.12;
                emit(mouthX + (Math.random() - 0.5) * width + vxk * lead, mouthY + (Math.random() - 0.5) * width * 0.6 + vyk * lead,
                     vxk, vyk + G * lead, Math.random() < 0.5 ? 0 : 1);
                fly[n - 1] = 200;
            }
        }

        function splash(count) {
            for (let k = 0; k < count; k++) {
                const x = latin.minX + Math.random() * (latin.maxX - latin.minX);
                // the freshest splash is always half water
                emit(x - 20, -6 - Math.random() * 12, 0.45 + Math.random() * 0.3, 0.3 + Math.random() * 0.4,
                     k % 2 ? 0 : 1);
                fly[n - 1] = 140;
            }
        }

        function frame(now) {
            if (finished) return;
            const t = (now - t0) / 1000;
            while (stage < cues.length && t >= cues[stage][0]) cues[stage++][1]();

            const fill = filledCount / Math.max(1, cyr.count);
            if (t > 3.6 && fill < 0.97) spreadFill(t);
            if (!doneAt && t > 3.6 && ((fill >= 0.97 && t > 9.5) || t >= 17)) {
                for (const h of holes) if (!filled[h]) { filled[h] = 1; filledCount++; }
                doneAt = t;
                window.SFX && SFX.play('balalaika', 1);
            }
            jetFrame();
            // wet splats and slaps when it hits the letters or the floor
            if (impacts > 0 && window.SFX) {
                if (impacts > 6) SFX.play('splat', Math.min(1, 0.3 + impacts / 40));
                else SFX.play('splatter', 0.25 + impacts * 0.08);
            }
            impacts = 0;
            step(); step();
            updateDrips();

            ctx.clearRect(0, 0, W, HT);
            ctx.fillStyle = '#000';
            ctx.fillRect(0, 0, W, H);

            if (montageAt) {
                // the credits own the screen; the last of the vomit still drips off the border below it
                if (!montageDone && !Montage.draw(ctx, W, H, t - montageAt)) montageDone = true;
                if (montageDone) { ctx.fillStyle = '#000'; ctx.fillRect(0, 0, W, H); Montage.titleCard(ctx, W, H, 1); }
                if (montageDone && Math.floor(t * 2) % 2 === 0) {
                    text('Paina mitä tahansa näppäintä  ·  Välilyönti aloittaa heti',
                         W / 2 - fs * 7.6, H - fs * 1.2, '#ddd', fs * 0.4);
                }
                ctx.save();
                ctx.beginPath(); ctx.rect(0, H - 2 * CELL, W, HT - H + 2 * CELL); ctx.clip();
                drawFluid();
                drawDrips();
                ctx.restore();
                requestAnimationFrame(frame);
                return;
            }

            const typed = Math.min(titleText.length, Math.floor(t / 0.11));
            text(titleText.slice(0, typed), startX, baseY, '#fff');
            if (t > 1.6) text(latinText, latinX, baseY, t > 3.1 ? '#b9d36a' : '#fff');
            if (t > 2.6) text('*RÖKH*', startX - fs * 0.2, baseY + fs * 0.9, '#e0b030', fs * 0.42);
            if (t > 3.1) text('*BLAAARGH*', startX - fs * 0.6, baseY + fs * 1.6, '#f0c040', fs * 0.5);
            if (t > 5.0) text('*ÖÖÖRRRKKK*', latinX + latinW * 0.45, baseY - fs * 1.15, '#f0c040', fs * 0.45);
            if (t > 7.0) text('*BLÖÖÖÖÖRRGHHH-HHH*', startX - fs * 0.4, baseY - fs * 1.4, '#ffd24a', fs * 0.55);

            drawFluid();
            drawDrips();

            if (doneAt) {
                text('♪ Балалайка ♪  tiling-tiling-tiling...',
                     W / 2 - fs * 6.2, floorY + fs * 1.4, '#888', fs * 0.4);
            }

            // then the opening credits roll, to the theme
            if (doneAt && t - doneAt > 2.6 && window.Montage) {
                montageAt = t;
                stopTheme = window.SFX ? SFX.startTheme(Montage.duration() + 1) : () => {};
            }
            requestAnimationFrame(frame);
        }

        function finish() {
            if (finished) return;
            finished = true;
            stopTheme();
            window.removeEventListener('keydown', onKey, true);
            canvas.remove();
            onDone();
        }
        // keys never reach the page during the intro; Space starts the game at once
        function onKey(e) {
            e.preventDefault();
            e.stopPropagation();
            // Space starts the game; any other key skips ahead, and once the credits are done, starts too
            if (e.key === ' ' || montageDone) { finish(); return; }
            if (montageAt) { montageDone = true; stopTheme(); }
            else if (doneAt) { montageAt = -1; montageDone = true; stopTheme(); }
        }
        window.addEventListener('keydown', onKey, true);
        canvas.addEventListener('click', () => { if (montageDone) finish(); });
        requestAnimationFrame(frame);
    }

    window.WebIntro = { play };
})();
