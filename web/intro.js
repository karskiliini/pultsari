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
        const titleText = 'GRAND TYR MOSCOW: ';
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
        // darkening over the background picture so the title and the vomit stay readable
        const shade = ctx.createLinearGradient(0, 0, 0, H);
        const stop = (v) => Math.min(1, Math.max(0, v / H));
        shade.addColorStop(0, 'rgba(0,0,0,0.2)');
        shade.addColorStop(stop(baseY - fs * 2.2), 'rgba(0,0,0,0.5)');
        shade.addColorStop(stop(floorY + fs * 1.8), 'rgba(0,0,0,0.5)');
        shade.addColorStop(1, 'rgba(0,0,0,0.15)');

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
        // "Russki Mir" is invisible: only the vomit running over it shows its letters, and they stay stained
        const revealed = new Uint8Array(FW * FH);
        // the vomit first piles up on the screen's bottom border, then oozes over it and drips away
        const floorCell = Math.floor(H / CELL) - 1;
        const smear = new Uint8Array(FW * 3); // stains on the border rows
        const smearK = new Uint8Array(FW * 3); // and what they are made of
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

        // What the drunk had eaten: the liquid is bile, pizza sauce, beer and sausage gravy, and it
        // carries lumps of pepperoni, cheese, sausage, pizza crust and green pepper.
        const PAL = [
            [196, 214, 96],   // 0 bile
            [222, 112, 44],   // 1 pizza sauce
            [232, 200, 104],  // 2 beer and bile
            [180, 112, 74],   // 3 sausage gravy
            [176, 36, 28],    // 4 pepperoni
            [252, 212, 76],   // 5 cheese
            [226, 142, 128],  // 6 sausage
            [210, 162, 92],   // 7 crust
            [92, 156, 40],    // 8 green pepper
        ];
        const CHUNK0 = 4;
        const kind = new Uint8Array(MAX);
        let mealMix = 1;   // the liquid the current retch is mostly made of
        function pickChunk() {
            const c = Math.random();
            return c < 0.3 ? 4 : c < 0.55 ? 5 : c < 0.75 ? 6 : c < 0.9 ? 7 : 8;
        }
        function pickKind() {
            const r = Math.random();
            if (r < 0.14) return pickChunk();
            if (r < 0.8) return mealMix;
            return (Math.random() * CHUNK0) | 0;
        }

        function emit(x, y, ux, uy, th = 1, k = pickKind()) {
            if (n >= MAX) return;
            px[n] = x; py[n] = y; vx[n] = ux; vy[n] = uy; thick[n] = th; rest[n] = 0; fly[n] = 0; kind[n] = k; n++;
        }
        function remove(i) {
            n--;
            px[i] = px[n]; py[i] = py[n]; vx[i] = vx[n]; vy[i] = vy[n]; thick[i] = thick[n]; rest[i] = rest[n]; fly[i] = fly[n]; kind[i] = kind[n];
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
                    if (rest[i] === 0 && Math.random() < 0.25) {
                        const si = (Math.random() < 0.5 ? FW : 0) + sx;
                        smear[si] = 1 + (thick[i] > 0.5);
                        smearK[si] = kind[i];
                    }
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

        const tintR = new Float32Array(FW * FH), tintG = new Float32Array(FW * FH), tintB = new Float32Array(FW * FH);
        const tintW = new Float32Array(FW * FH);
        const lumpAt = new Uint8Array(FW * FH);   // a lump of food in this cell, by kind
        function drawFluid() {
            density.fill(0);
            thickness.fill(0);
            tintR.fill(0); tintG.fill(0); tintB.fill(0); tintW.fill(0);
            lumpAt.fill(0);
            for (let i = 0; i < n; i++) {
                const cx = px[i] | 0, cy = py[i] | 0;
                const k = kind[i], c = PAL[k], tw = k >= CHUNK0 ? 0.3 : 1;
                for (let oy = -1; oy <= 1; oy++) for (let ox = -1; ox <= 1; ox++) {
                    const x = cx + ox, y = cy + oy;
                    if (x < 0 || y < 0 || x >= FW || y >= FH) continue;
                    const w = (ox === 0 && oy === 0) ? 1 : (ox === 0 || oy === 0) ? 0.55 : 0.3;
                    const j = y * FW + x;
                    density[j] += w;
                    thickness[j] += w * thick[i];
                    tintR[j] += w * tw * c[0]; tintG[j] += w * tw * c[1]; tintB[j] += w * tw * c[2]; tintW[j] += w * tw;
                }
                // lumps are a cell or two of solid colour, ragged in shape
                if (k >= CHUNK0 && cx >= 0 && cy >= 0 && cx < FW - 1 && cy < FH - 1) {
                    const j = cy * FW + cx;
                    lumpAt[j] = k;
                    if (i & 1) lumpAt[j + 1] = k;
                    if (i & 2) lumpAt[j + FW] = k;
                }
            }
            for (let i = 0; i < FW * FH; i++) if (density[i] > 0.3 && latin.mask[i]) revealed[i] = 1;
            const d = img.data;
            for (let i = 0; i < FW * FH; i++) {
                const o = i * 4;
                const row = (i / FW) | 0;
                const sr = row - floorCell;
                if (sr >= 0 && sr < 3 && density[i] < 0.85 && smear[sr * FW + (i % FW)]) {
                    // stains left on the bottom border, in the colour of what made them; slime darker
                    const si = sr * FW + (i % FW);
                    const f = smear[si] === 2 ? 0.45 : 0.7, c = PAL[smearK[si]];
                    d[o] = c[0] * f; d[o + 1] = c[1] * f; d[o + 2] = c[2] * f; d[o + 3] = 210;
                    continue;
                }
                if (filled[i]) {
                    const [r, g, b] = wordColor((i / FW) | 0, i % FW);
                    d[o] = r; d[o + 1] = g; d[o + 2] = b; d[o + 3] = 255;
                } else if (density[i] > 0.85) {
                    // slime is dark and deep green, water a pale watery yellow-green; deeper is darker
                    const t = Math.min(1, (density[i] - 0.85) / 2.5);
                    const s = thickness[i] / density[i];
                    let r = 205 - 60 * s, g = 220 - 20 * s, b = 140 - 110 * s;
                    // coloured by what it is made of: sauce, gravy, beer; lumps show in their own colour
                    const lump = lumpAt[i];
                    if (lump) {
                        const c = PAL[lump], v = 0.92 + ((i * 2654435761) >>> 28) / 60;
                        r = c[0] * v; g = c[1] * v; b = c[2] * v;
                    } else if (tintW[i] > 0) {
                        const tw = tintW[i];
                        r = r * 0.3 + 0.7 * tintR[i] / tw; g = g * 0.3 + 0.7 * tintG[i] / tw; b = b * 0.3 + 0.7 * tintB[i] / tw;
                    }
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
                    d[o + 3] = lump ? 255 : Math.round(255 * (0.55 + 0.45 * s));
                } else if (revealed[i]) {
                    // a letter the vomit has run over: stained bile green
                    const shimmer = ((i % FW) * 7 + ((i / FW) | 0) * 13) % 5 * 4;
                    d[o] = 175 + shimmer; d[o + 1] = 205 + shimmer; d[o + 2] = 96; d[o + 3] = 255;
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
        // as long as any vomit is left on the border, a stream at least a pixel thick keeps
        // running off it: it pours for a moment, lets go and falls; then a second or two of quiet
        const trickles = [];
        let nextTrickleAt = 0;
        function updateTrickles() {
            const now = performance.now();
            if (!trickles.length && now >= nextTrickleAt) {
                const from = [];
                for (let i = 0; i < n; i++) if (rest[i] > 0) from.push(px[i] * CELL);
                if (!from.length) for (let x = 0; x < FW; x++) if (smear[x] || smear[FW + x] || smear[2 * FW + x]) from.push(x * CELL + CELL / 2);
                if (from.length) {
                    trickles.push({ x: from[(Math.random() * from.length) | 0], top: lipTop, bottom: lipTop, vb: 0.4, vt: 0,
                                    pour: 20 + Math.random() * 60, w: 1 + Math.random() * 1.3 });
                } else nextTrickleAt = now + 500;  // nothing on the border yet
            }
            for (let k = trickles.length - 1; k >= 0; k--) {
                const tr = trickles[k];
                tr.vb += 0.18; tr.bottom += tr.vb;
                if (tr.pour > 0) {
                    tr.pour--;
                    // the stream drains the puddle it runs from, a little at a time
                    if (tr.pour % 12 === 0) {
                        for (let i = 0; i < n; i++) if (rest[i] > 10 && Math.abs(px[i] * CELL - tr.x) < 3 * CELL) { remove(i); break; }
                    }
                } else { tr.vt += 0.18; tr.top += tr.vt; }
                if (tr.top > HT + 4) { trickles.splice(k, 1); nextTrickleAt = performance.now() + 1000 + Math.random() * 1000; }
            }
        }
        // the colour of the pool on the border, column by column, from what lies there: it changes
        // slowly, so a strand of pizza sauce hangs next to one of sausage gravy
        const colRaw = new Float32Array(FW * 4);
        const colTint = new Float32Array(FW * 3);
        for (let x = 0; x < FW; x++) { colTint[x * 3] = 200; colTint[x * 3 + 1] = 150; colTint[x * 3 + 2] = 70; }
        function updateTint() {
            colRaw.fill(0);
            for (let i = 0; i < n; i++) {
                if (rest[i] === 0) continue;
                const c = PAL[kind[i]], w = kind[i] >= CHUNK0 ? 0.35 : 1, x = px[i] | 0;
                colRaw[x * 4] += c[0] * w; colRaw[x * 4 + 1] += c[1] * w; colRaw[x * 4 + 2] += c[2] * w; colRaw[x * 4 + 3] += w;
            }
            for (let x = 0; x < FW; x++) {
                let r = 0, g = 0, b = 0, w = 0;
                for (let o = -3; o <= 3; o++) {
                    const c = x + o;
                    if (c < 0 || c >= FW) continue;
                    const f = 1 - Math.abs(o) / 4;
                    r += colRaw[c * 4] * f; g += colRaw[c * 4 + 1] * f; b += colRaw[c * 4 + 2] * f; w += colRaw[c * 4 + 3] * f;
                }
                if (w <= 0) continue;
                const k = x * 3;
                colTint[k] += (r / w - colTint[k]) * 0.08;
                colTint[k + 1] += (g / w - colTint[k + 1]) * 0.08;
                colTint[k + 2] += (b / w - colTint[k + 2]) * 0.08;
            }
        }
        function tintAt(x) {
            const k = Math.min(FW - 1, Math.max(0, (x / CELL) | 0)) * 3;
            return [colTint[k], colTint[k + 1], colTint[k + 2]];
        }
        const mix = (a, b) => [(a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2];
        const rgba = (c, f = 1, a = 1) => `rgba(${Math.min(255, c[0] * f) | 0},${Math.min(255, c[1] * f) | 0},${Math.min(255, c[2] * f) | 0},${a})`;

        function drawTrickles() {
            for (const tr of trickles) {
                const y0 = Math.max(tr.top, lipTop), y1 = Math.min(tr.bottom, HT + 2);
                if (y1 <= y0) continue;
                const x0 = Math.round(tr.x - tr.w / 2);
                const c = tintAt(tr.x);
                ctx.fillStyle = rgba(c, 0.6);
                ctx.fillRect(x0, y0, Math.max(1, tr.w), y1 - y0);
                ctx.fillStyle = rgba(c, 1.3, 0.55);
                ctx.fillRect(x0, y0, Math.max(0.5, tr.w * 0.35), y1 - y0);
                ctx.beginPath();
                ctx.arc(tr.x, y1, Math.max(0.9, tr.w * 0.8), 0, Math.PI * 2);
                ctx.fillStyle = rgba(c, 0.75);
                ctx.fill();
            }
        }
        function updateDrips() {
            updateTrickles();
            updateTint();
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
                    drips.push({ x, dead: false, len: 0, w: 3 + Math.random() * Math.random() * 9, vol: 3, starve: 0,
                                 ox: 0, ovx: mv * CELL * 1.6 + (Math.random() - 0.5) * 0.4, bvx: mv * CELL * 0.6,
                                 maxLen: 25 + Math.random() * Math.max(30, (HT - H) * 0.8),
                                 speed: 0.1 + Math.random() * 0.16, tint: tintAt(x),
                                 streak: (Math.random() * CHUNK0) | 0, lump: Math.random() < 0.25 ? { k: pickChunk(), s: 0.05, r: 1.3 + Math.random() * 1.3 } : null });
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
                    if (bi >= 0 && best < 3 * CELL + d.w) {
                        // a lump in the puddle gets carried down the strand
                        if (kind[bi] >= CHUNK0 && !d.lump) d.lump = { k: kind[bi], s: 0.02, r: 1.3 + Math.random() * 1.3 };
                        remove(bi); d.vol += 1; fed = true;
                    }
                }
                d.starve = fed ? 0 : d.starve + 1;
                d.tint = tintAt(d.x);   // already smoothed over time, so it matches the lip it hangs from
                if (d.lump) {
                    // the lump slides down the strand and falls off the end with a drop
                    d.lump.s += 0.0012 + Math.min(0.004, d.vol / 4000);
                    if (d.lump.s > 0.95 && d.len > 10) {
                        drops.push({ x: d.x + d.ox, y: lipTop + d.len, r: d.lump.r, vy: 0.3, vx: d.ovx, c: d.tint.slice(), lump: d.lump.k });
                        d.lump = null;
                        d.len *= 0.75;
                    }
                }

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
                    drops.push({ x: d.x + d.ox, y: lipTop + d.len, r: d.r, vy: d.speed, vx: d.ovx, c: d.tint.slice() });
                    d.vol *= 0.25;
                    d.ovx *= -0.5; // recoil
                    d.len *= 0.35 + Math.random() * 0.3;
                    d.maxLen = 25 + Math.random() * Math.max(30, (HT - H) * 0.8);
                }
                if (d.starve > 90 && d.vol < 4) {
                    // the puddle is gone here: the strand thins to a hair and breaks
                    d.w *= 0.985;
                    if (d.w < 0.7) { drops.push({ x: d.x + d.ox, y: lipTop + d.len, r: Math.max(1, d.r * 0.7), vy: 0.3, vx: d.ovx, c: d.tint.slice() }); d.dead = true; drips.splice(k, 1); }
                }
            }
            for (let a = drips.length - 1; a >= 0; a--) {
                for (let b = a - 1; b >= 0; b--) {
                    const A = drips[a], B = drips[b];
                    if (Math.abs(A.x - B.x) > (A.w + B.w) * 0.5) continue;
                    B.vol += A.vol; B.w = Math.max(A.w, B.w) * 1.1; B.len = Math.max(A.len, B.len);
                    B.ovx = (A.ovx + B.ovx) * 0.5; if (!B.lump) B.lump = A.lump; A.dead = true; drips.splice(a, 1); break;
                }
            }
            for (let k = drops.length - 1; k >= 0; k--) {
                const p = drops[k];
                p.vy += 0.16; p.y += p.vy; p.x += p.vx; p.vx *= 0.996;
                if (p.y > HT + 20) drops.splice(k, 1);
            }
        }

        const GLOSS = 'rgba(255,240,200,0.45)';
        // one vertical shade for everything that hangs from the border, so the lip, the strands
        // and the threads between them read as a single mass of slime with no seams
        let slimeGrad = null;
        function slimeFill() {
            if (slimeGrad) return slimeGrad;
            const span = Math.max(30, HT + 20 - lipTop);
            slimeGrad = ctx.createLinearGradient(0, lipTop, 0, lipTop + span);
            // neutral and brownish: the colour of what lies above is laid over it
            slimeGrad.addColorStop(0, '#b4a452');
            slimeGrad.addColorStop(Math.min(0.9, 6 / span), '#8e7c38');
            slimeGrad.addColorStop(Math.min(0.95, 16 / span), '#6c5a26');
            slimeGrad.addColorStop(1, '#4a3c16');
            return slimeGrad;
        }

        // A strand's centreline and width from inside the lip down to the drop. Where it leaves the
        // lip it flares out like a meniscus, it is thinnest in the middle and swells into the drop.
        const GEO_N = 26;
        const filletOf = (d) => 2.5 + d.w * 1.3;
        function strandGeo(d) {
            const yL = lipTop + d.base;
            const top = lipTop - 0.5;
            const tip = lipTop + d.len;
            const g = d.geo || (d.geo = { pts: [] });
            g.ok = tip > yL + 2;
            if (!g.ok) return g;
            const tx = d.x + d.ox, ty = tip - d.r * 0.6, L = ty - top;
            const w0 = d.w * 1.25, w1 = Math.max(0.9, d.w * 0.5);
            const fil = filletOf(d), D = fil * 1.3;
            g.pts.length = 0;
            for (let k = 0; k <= GEO_N; k++) {
                const s = Math.pow(k / GEO_N, 1.8), u = 1 - s;   // more points near the root
                // straight down from the root, bending toward the swinging tip
                const x = (u * u * u + 3 * u * u * s) * d.x + (3 * u * s * s + s * s * s) * tx;
                const y = u * u * u * top + 3 * u * u * s * (top + L * 0.35) + 3 * u * s * s * (top + L * 0.6) + s * s * s * ty;
                const dy = y - yL;
                const neck = 1 - Math.min(1, Math.max(0, dy) / Math.max(6, L * 0.5));
                let w = w1 + (w0 - w1) * neck * neck;
                // a quarter-ellipse fillet: level with the lip where it starts, vertical where it meets the strand
                if (dy <= 0) w += 2 * fil;
                else if (dy < D) { const q = 1 - dy / D; w += 2 * fil * (1 - Math.sqrt(1 - q * q)); }
                w += d.r * 0.6 * Math.max(0, (s - 0.75) / 0.25);
                g.pts.push({ x, y, w });
            }
            g.yL = yL; g.tip = tip; g.tx = tx; g.ty = ty; g.fil = fil;
            return g;
        }
        // the point at fraction s of the way from where the strand leaves the lip to its drop
        function strandAt(g, s) {
            const y = g.yL + s * (g.ty - g.yL), p = g.pts;
            for (let k = 1; k < p.length; k++) {
                if (p[k].y >= y) {
                    const a = p[k - 1], b = p[k], f = (y - a.y) / Math.max(1e-6, b.y - a.y);
                    return { x: a.x + (b.x - a.x) * f, y, w: a.w + (b.w - a.w) * f };
                }
            }
            const l = p[p.length - 1];
            return { x: l.x, y: l.y, w: l.w };
        }
        function strandPath(g) {
            const p = g.pts;
            ctx.beginPath();
            ctx.moveTo(p[0].x - p[0].w / 2, p[0].y);
            for (let k = 1; k < p.length; k++) ctx.lineTo(p[k].x - p[k].w / 2, p[k].y);
            for (let k = p.length - 1; k >= 0; k--) ctx.lineTo(p[k].x + p[k].w / 2, p[k].y);
            ctx.closePath();
        }

        // ---- bridges: slime stays stuck between strands that hang close together ----
        // Very close ones share a web, a film from the lip down to a sagging, thicker rim;
        // further apart a thread sags between them and its slime runs into a bead at the
        // bottom. Both stretch thinner as the strands swing apart, and snap.
        const bridges = [];
        function sagOf(br) {
            const slack = br.rest / (br.strain * br.strain);
            return Math.min(28, (br.web ? 0.22 : 0.3) * slack + br.bead * 1.2);
        }
        function updateBridges() {
            const live = drips.filter((d) => d.geo && d.geo.ok && d.len > 12);
            live.sort((a, b) => a.x - b.x);
            for (let k = 1; k < live.length && bridges.length < 9; k++) {
                const a = live[k - 1], b = live[k];
                const gap = Math.abs((b.x + b.ox * 0.5) - (a.x + a.ox * 0.5));
                if (gap > 70 || Math.random() > 0.02) continue;
                if (bridges.some((br) => (br.a === a && br.b === b) || (br.a === b && br.b === a))) continue;
                const web = gap < 32;
                const sa = web ? 0.15 + Math.random() * 0.3 : 0.25 + Math.random() * 0.5;
                const sb = web ? Math.max(0.1, Math.min(0.6, sa + (Math.random() - 0.5) * 0.25)) : 0.25 + Math.random() * 0.5;
                const A = strandAt(a.geo, sa), B = strandAt(b.geo, sb);
                bridges.push({ a, b, sa, sb, web, rest: Math.hypot(B.x - A.x, B.y - A.y),
                               thick: web ? 1.4 + Math.random() * 0.8 : 0.7 + Math.random() * 0.9,
                               bead: 0, life: 260 + Math.random() * 900, strain: 1 });
            }
            for (let k = bridges.length - 1; k >= 0; k--) {
                const br = bridges[k];
                if (br.a.dead || br.b.dead || !br.a.geo.ok || !br.b.geo.ok) { bridges.splice(k, 1); continue; }
                const A = strandAt(br.a.geo, br.sa), B = strandAt(br.b.geo, br.sb);
                br.A = A; br.B = B;
                br.strain = Math.hypot(B.x - A.x, B.y - A.y) / Math.max(4, br.rest);
                br.thick *= 0.999;
                if (!br.web) br.bead = Math.min(2.6, br.bead + 0.01);
                // the slime pulls the two strands toward each other
                const dir = Math.sign(B.x - A.x), pull = 0.004 * Math.min(2, br.strain);
                br.a.ovx += dir * pull; br.b.ovx -= dir * pull;
                br.sag = sagOf(br);
                if (--br.life < 0 || br.strain > (br.web ? 1.6 : 1.9) || br.thick / Math.sqrt(br.strain) < 0.35) {
                    // it snaps and what hung at the bottom falls
                    drops.push({ x: (A.x + B.x) / 2, y: (A.y + B.y) / 2 + br.sag,
                                 r: Math.max(1, br.web ? br.thick * 1.2 : br.bead), vy: 0.2, vx: 0, c: mix(br.a.tint, br.b.tint) });
                    bridges.splice(k, 1);
                }
            }
        }
        // quadratic curve that sags by `sag` below the line from A to B
        const sagCtl = (A, B, sag) => [(A.x + B.x) / 2, (A.y + B.y) / 2 + 2 * sag];
        function drawWeb(br) {
            let { a, b, A, B } = br;
            if (A.x > B.x) { [a, b] = [b, a]; [A, B] = [B, A]; }
            const [cx, cy] = sagCtl(A, B, br.sag);
            const film = ctx.createLinearGradient(0, lipTop, 0, Math.max(A.y, B.y) + br.sag);
            const c = mix(a.tint, b.tint);
            film.addColorStop(0, rgba(c, 0.62, 0.9));
            film.addColorStop(1, rgba(c, 0.45, 0.5));
            ctx.fillStyle = film;
            ctx.beginPath();
            ctx.moveTo(A.x, lipTop);
            for (const p of a.geo.pts) if (p.y < A.y) ctx.lineTo(p.x + p.w * 0.3, p.y);
            ctx.lineTo(A.x, A.y);
            ctx.quadraticCurveTo(cx, cy, B.x, B.y);
            const pb = b.geo.pts;
            for (let k = pb.length - 1; k >= 0; k--) if (pb[k].y < B.y) ctx.lineTo(pb[k].x - pb[k].w * 0.3, pb[k].y);
            ctx.lineTo(B.x, lipTop);
            ctx.closePath();
            ctx.fill();
        }
        function drawWebRim(br) {
            const { A, B } = br;
            const [cx, cy] = sagCtl(A, B, br.sag);
            const th = Math.max(0.6, br.thick / Math.sqrt(br.strain));
            ctx.lineCap = 'round';
            ctx.strokeStyle = slimeFill();
            ctx.lineWidth = th * 1.6;
            ctx.beginPath(); ctx.moveTo(A.x, A.y); ctx.quadraticCurveTo(cx, cy, B.x, B.y); ctx.stroke();
            ctx.strokeStyle = rgba(mix(br.a.tint, br.b.tint), 1, 0.75);
            ctx.stroke();
            ctx.strokeStyle = 'rgba(255,240,200,0.4)';
            ctx.lineWidth = 0.6;
            ctx.beginPath(); ctx.moveTo(A.x, A.y - th * 0.4); ctx.quadraticCurveTo(cx, cy - th * 0.4, B.x, B.y - th * 0.4); ctx.stroke();
        }
        function drawThread(br) {
            const { A, B } = br;
            const [cx, cy] = sagCtl(A, B, br.sag);
            const th = Math.max(0.45, br.thick / Math.sqrt(br.strain));
            // thick where it leaves each strand, thinnest in the middle
            const N = 14, up = [], dn = [];
            for (let k = 0; k <= N; k++) {
                const t = k / N, u = 1 - t;
                const x = u * u * A.x + 2 * u * t * cx + t * t * B.x;
                const y = u * u * A.y + 2 * u * t * cy + t * t * B.y;
                const dx = 2 * u * (cx - A.x) + 2 * t * (B.x - cx), dy = 2 * u * (cy - A.y) + 2 * t * (B.y - cy);
                const l = Math.hypot(dx, dy) || 1, nx = -dy / l, ny = dx / l;
                const w = th * (1 + 1.8 * (Math.exp(-t / 0.08) + Math.exp(-u / 0.08))) / 2;
                up.push(x + nx * w, y + ny * w);
                dn.push(x - nx * w, y - ny * w);
            }
            const path = new Path2D();
            path.moveTo(up[0], up[1]);
            for (let k = 2; k < up.length; k += 2) path.lineTo(up[k], up[k + 1]);
            for (let k = dn.length - 2; k >= 0; k -= 2) path.lineTo(dn[k], dn[k + 1]);
            path.closePath();
            ctx.fillStyle = slimeFill();
            ctx.fill(path);
            // coloured like the strands at either end, blending between them
            const tg = ctx.createLinearGradient(A.x, A.y, B.x, B.y);
            tg.addColorStop(0, rgba(br.a.tint, 1, 0.75));
            tg.addColorStop(1, rgba(br.b.tint, 1, 0.75));
            ctx.fillStyle = tg;
            ctx.fill(path);
            ctx.strokeStyle = 'rgba(255,240,200,0.35)';
            ctx.lineWidth = 0.5;
            ctx.beginPath(); ctx.moveTo(A.x, A.y - th * 0.3); ctx.quadraticCurveTo(cx, cy - th * 0.3, B.x, B.y - th * 0.3); ctx.stroke();
        }
        // a wet ball lit from the upper left, in the colour c
        function ballFill(x, y, r, c) {
            const bg = ctx.createRadialGradient(x - r * 0.35, y - r * 0.4, r * 0.1, x, y, r * 1.3);
            bg.addColorStop(0, rgba(c, 1.35));
            bg.addColorStop(0.5, rgba(c, 0.72));
            bg.addColorStop(1, rgba(c, 0.3));
            return bg;
        }
        function drawBead(x, y, r, c) {
            ctx.beginPath();
            ctx.ellipse(x, y, r * 0.9, r * 1.1, 0, 0, Math.PI * 2);
            ctx.fillStyle = ballFill(x, y, r, c);
            ctx.fill();
        }
        // a lump of food: ragged, matt and darker at the edge
        function drawLump(x, y, r, k, seed) {
            const c = PAL[k];
            ctx.beginPath();
            for (let a = 0; a < 7; a++) {
                const ang = a / 7 * Math.PI * 2, rr = r * (0.75 + 0.35 * (((seed * 31 + a * 17) % 7) / 7));
                const X = x + Math.cos(ang) * rr, Y = y + Math.sin(ang) * rr * 0.85;
                if (a) ctx.lineTo(X, Y); else ctx.moveTo(X, Y);
            }
            ctx.closePath();
            const g = ctx.createRadialGradient(x - r * 0.3, y - r * 0.3, r * 0.1, x, y, r * 1.2);
            g.addColorStop(0, rgba(c, 1.15));
            g.addColorStop(1, rgba(c, 0.55));
            ctx.fillStyle = g;
            ctx.fill();
        }

        function drawDrips() {
            drawTrickles();
            // the lip: the puddle bulging over the edge, deeper where it is thicker and around each strand
            for (let x = 0; x < FW; x++) {
                let sum = 0;
                for (let o = -4; o <= 4; o++) { const c = x + o; if (c >= 0 && c < FW) sum += colCount[c] * (1 - Math.abs(o) / 5); }
                lip[x] = sum > 0.5 ? Math.min(22, Math.sqrt(sum) * 3.2) : 0;
            }
            // each strand sags the lip into a short level shelf as wide as its fillet, so the fillet
            // can curve out of it without a step
            for (const d of drips) {
                const c = Math.min(FW - 1, Math.max(0, (d.x / CELL) | 0));
                d.base = Math.max(lip[c], 1 + d.w * 0.6);
            }
            for (const d of drips) {
                const half = (d.w * 1.25 / 2 + filletOf(d)) / CELL;
                const c = d.x / CELL;
                for (let x = Math.max(0, Math.floor(c - half)); x <= Math.min(FW - 1, Math.ceil(c + half)); x++) {
                    lip[x] = Math.max(lip[x], d.base);
                }
            }
            for (const d of drips) strandGeo(d);
            updateBridges();

            // webs hang behind everything, a translucent film between their strands
            for (const br of bridges) if (br.web) drawWeb(br);

            // the strands, in the same slime as the lip; the lip covers their roots so they grow out of it
            for (const d of drips) {
                if (!d.geo.ok) continue;
                strandPath(d.geo);
                ctx.fillStyle = slimeFill();
                ctx.fill();
                ctx.fillStyle = rgba(d.tint, 1, 0.75);
                ctx.fill();
                // a streak of something else marbled through it: sauce in the bile, bile in the gravy
                const p = d.geo.pts;
                ctx.strokeStyle = rgba(PAL[d.streak], 0.95, 0.55);
                ctx.lineCap = 'round';
                ctx.beginPath();
                let on = false;
                for (let k = 0; k < p.length; k++) {
                    if (p[k].y < d.geo.yL + 1) continue;
                    const x = p[k].x + p[k].w * (0.12 + 0.1 * Math.sin(k * 0.9 + d.x));
                    if (on) ctx.lineTo(x, p[k].y); else { ctx.moveTo(x, p[k].y); on = true; }
                }
                ctx.lineWidth = Math.max(0.6, d.w * 0.22);
                ctx.stroke();
            }
            const lipPath = new Path2D();
            let open = false;
            for (let x = 0; x <= FW; x++) {
                const h = x < FW ? lip[x] : 0;
                if (h > 0.4 && !open) { lipPath.moveTo(x * CELL, lipTop); open = true; }
                if (open) lipPath.lineTo(x * CELL + CELL / 2, lipTop + h);
                if (h <= 0.4 && open) { lipPath.lineTo(x * CELL, lipTop); lipPath.closePath(); open = false; }
            }
            ctx.fillStyle = slimeFill();
            ctx.fill(lipPath);
            // the colour of what lies above, column by column: sauce here, gravy or beer there
            ctx.save();
            ctx.clip(lipPath);
            for (let x = 0; x < FW; x++) {
                if (lip[x] <= 0.4) continue;
                ctx.fillStyle = rgba([colTint[x * 3], colTint[x * 3 + 1], colTint[x * 3 + 2]], 1, 0.75);
                ctx.fillRect(x * CELL - 0.5, lipTop, CELL + 1, lip[x] + 2);
            }
            ctx.restore();
            // lumps of food stuck in it
            for (let i = 0; i < n; i++) {
                if (rest[i] < 20 || kind[i] < CHUNK0) continue;
                const x = px[i] * CELL, h = lip[Math.min(FW - 1, px[i] | 0)];
                if (h < 3) continue;
                drawLump(x, lipTop + Math.min(h * 0.5, 2 + (i % 4)), 1.2 + (i % 3) * 0.35, kind[i], i);
            }

            for (const br of bridges) if (br.web) drawWebRim(br); else drawThread(br);

            // round in cross-section: a lit stripe on the left, shadow on the right; it fades in
            // below the lip so the root stays one with it
            ctx.lineCap = 'butt';
            for (const d of drips) {
                const g = d.geo;
                if (!g.ok) continue;
                const p = g.pts;
                for (let k = 1; k < p.length; k++) {
                    const a = p[k - 1], b = p[k];
                    const fade = Math.min(1, Math.max(0, (b.y - g.yL) / (g.fil * 2)));
                    if (fade <= 0) continue;
                    const w = (a.w + b.w) / 2;
                    ctx.globalAlpha = fade;
                    ctx.strokeStyle = 'rgba(255,235,190,0.3)';
                    ctx.lineWidth = w * 0.25;
                    ctx.beginPath(); ctx.moveTo(a.x - a.w * 0.22, a.y); ctx.lineTo(b.x - b.w * 0.22, b.y); ctx.stroke();
                    ctx.strokeStyle = 'rgba(18,34,2,0.5)';
                    ctx.lineWidth = w * 0.3;
                    ctx.beginPath(); ctx.moveTo(a.x + a.w * 0.3, a.y); ctx.lineTo(b.x + b.w * 0.3, b.y); ctx.stroke();
                }
                ctx.globalAlpha = 1;
                ctx.beginPath();
                ctx.ellipse(g.tx, g.tip, d.r, d.r * 1.15, d.ovx * 0.15, 0, Math.PI * 2);
                ctx.fillStyle = ballFill(g.tx, g.tip, d.r, mix(d.tint, [150, 120, 60]));
                ctx.fill();
                if (d.lump) {
                    const q = strandAt(g, Math.min(0.95, d.lump.s));
                    drawLump(q.x, q.y, Math.max(d.lump.r, q.w * 0.45), d.lump.k, d.x | 0);
                }
            }
            for (const br of bridges) {
                if (br.web || br.bead < 0.6) continue;
                const [cx, cy] = sagCtl(br.A, br.B, br.sag);
                drawBead((br.A.x + 2 * cx + br.B.x) / 4, (br.A.y + 2 * cy + br.B.y) / 4 + br.bead * 0.5, br.bead, mix(br.a.tint, br.b.tint));
            }
            for (const p of drops) {
                ctx.beginPath();
                ctx.ellipse(p.x, p.y, p.r * 0.85, p.r * (1.1 + Math.min(0.8, p.vy * 0.08)), 0, 0, Math.PI * 2);
                ctx.fillStyle = ballFill(p.x, p.y, p.r, p.c || [190, 150, 70]);
                ctx.fill();
                if (p.lump) drawLump(p.x, p.y - p.r * 0.2, p.r * 0.75, p.lump, (p.x * 7) | 0);
            }
            // a wet highlight down the left side of each strand and on its drop
            ctx.strokeStyle = GLOSS;
            ctx.lineWidth = 0.8;
            for (const d of drips) {
                const g = d.geo;
                if (!g.ok || d.len < 6) continue;
                ctx.beginPath();
                let started = false;
                for (const p of g.pts) {
                    if (p.y < g.yL + g.fil * 1.2 || p.y > g.tip - d.r * 0.8) continue;
                    const x = p.x - p.w * 0.3;
                    if (started) ctx.lineTo(x, p.y); else { ctx.moveTo(x, p.y); started = true; }
                }
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(g.tx - d.r * 0.35, g.tip - d.r * 0.35, d.r * 0.3, 0, Math.PI * 2);
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
            // each heave is a real recording (web/sounds), the synthesized gurgle until they load
            [3.1, () => { window.SFX && SFX.playSample('vomit', 1, 'vomit'); startJet(3, 42, false, 0.0, 0.45); }],
            [5.0, () => { window.SFX && SFX.playSample('vomit', 1, 'vomit2'); startJet(2, 40, false, 0.45, 0.8); }],
            // the third heave is the longest: a long, wavering jet
            [7.0, () => { window.SFX && SFX.playSample('vomit', 1, 'vomit'); startJet(4, 44, true, 0.1, 1.0); }],
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
                pulses.push({ at, dur, rate: perFrame * (0.7 + Math.random() * 0.6), mix: [1, 3, 0, 2, 1][(Math.random() * 5) | 0] });
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
            mealMix = pulse.mix;
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
            if (!montageAt && window.AdArt) {
                // the game ad burns behind the title; darkest where the letters and the vomit are
                AdArt.draw(ctx, W, H, t);
                ctx.fillStyle = shade;
            } else {
                ctx.fillStyle = '#000';
            }
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
            if (t > 5.0) text('*ÖÖÖRRRKKK*', latinX + latinW * 0.45, baseY - fs * 1.15, '#f0c040', fs * 0.45);

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
