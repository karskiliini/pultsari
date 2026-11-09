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
        const cyr = textMask(cyrText, cyrX, cyrY);
        const filled = new Uint8Array(FW * FH);
        let filledCount = 0;
        // the vomit first piles up on the screen's bottom border, then oozes over it and drips away
        const floorCell = Math.floor(H / CELL) - 1;
        const smear = new Uint8Array(FW * 3); // stains on the border rows
        const leakSpots = [];

        // ---- fluid: particles with gravity, pressure, cohesion and viscosity ----
        const MAX = 1600;
        const px = new Float32Array(MAX), py = new Float32Array(MAX);
        const vx = new Float32Array(MAX), vy = new Float32Array(MAX);
        // 0 = watery, 1 = thick slime: every splash is half water, the rest stays thick
        const thick = new Float32Array(MAX);
        const rest = new Uint16Array(MAX);   // frames spent lying on the border
        const leak = new Uint8Array(MAX);    // 1 = oozing over the edge
        let n = 0;
        const G = 0.03, R0 = 1.15, H_ = 2.3, K = 0.09, COH = 0.009, VISC = 0.13;
        const grid = new Map();

        function emit(x, y, ux, uy, th = 1) {
            if (n >= MAX) return;
            px[n] = x; py[n] = y; vx[n] = ux; vy[n] = uy; thick[n] = th; rest[n] = 0; leak[n] = 0; n++;
        }
        function remove(i) {
            n--;
            px[i] = px[n]; py[i] = py[n]; vx[i] = vx[n]; vy[i] = vy[n]; thick[i] = thick[n]; rest[i] = rest[n]; leak[i] = leak[n];
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
                for (let ox = -1; ox <= 1; ox++) for (let oy = -1; oy <= 1; oy++) {
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
                }
                // thick slime drags on everything
                vx[i] *= 1 - 0.05 * t; vy[i] *= 1 - 0.035 * t;
                const sp = Math.hypot(vx[i], vy[i]), top = 1.7 - 0.9 * t;
                if (sp > top) { vx[i] *= top / sp; vy[i] *= top / sp; }
            }
            for (let i = n - 1; i >= 0; i--) {
                px[i] += vx[i]; py[i] += vy[i];
                if (px[i] < 1) { px[i] = 1; vx[i] *= -0.3; }
                if (px[i] > FW - 2) { px[i] = FW - 2; vx[i] *= -0.3; }
                if (!leak[i] && py[i] >= floorCell) {
                    // lands on the border and pools there
                    py[i] = floorCell; if (vy[i] > 0) vy[i] *= -0.05; vx[i] *= 0.85;
                    const sx = px[i] | 0;
                    if (rest[i] === 0 && Math.random() < 0.25) smear[(Math.random() < 0.5 ? FW : 0) + sx] = 1 + (thick[i] > 0.5);
                    if (rest[i] < 65000) rest[i]++;
                    if (leakSpots.length < 7 && rest[i] === 1 && Math.random() < 0.015) leakSpots.push(px[i]);
                    // the puddle creeps toward the nearest drip and slowly goes over the edge there
                    let near = -1, best = 1e9;
                    for (const sp of leakSpots) { const d = Math.abs(sp - px[i]); if (d < best) { best = d; near = sp; } }
                    if (near >= 0) {
                        vx[i] += Math.sign(near - px[i]) * 0.004;
                        if (best < 2.5 && rest[i] > 40 + 160 * thick[i] && Math.random() < 0.04) {
                            leak[i] = 1; py[i] = floorCell + 0.6; vy[i] = 0.02;
                            smear[2 * FW + (px[i] | 0)] = 2;
                            // slime pulls its neighbours over the edge with it: a strand, not a drop
                            for (let j = 0; j < n; j++) {
                                if (leak[j] || j === i || rest[j] < 15) continue;
                                if (Math.abs(px[j] - px[i]) < 1.6 && Math.random() < 0.5) {
                                    leak[j] = 1; py[j] = floorCell + 0.2; vy[j] = 0.01; px[j] = px[i] + (Math.random() - 0.5) * 0.8;
                                }
                            }
                        }
                    }
                } else if (leak[i]) {
                    // hanging slime: oozes slowly at first, then lets go and falls
                    const below = py[i] - floorCell;
                    const cap = 0.03 + 0.018 * below * (1.2 - thick[i]);
                    if (vy[i] > cap) vy[i] = cap;
                    vx[i] *= 0.6;
                    if (py[i] >= FH - 1) { remove(i); continue; }
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
                if (leak[i]) {
                    // a hanging string of slime back up to the border
                    const len = Math.min(cy - floorCell, 3 + ((i * 7) % 4));
                    for (let k = 1; k <= len; k++) {
                        const y = cy - k;
                        if (y >= 0 && y < FH) { density[y * FW + cx] += 1; thickness[y * FW + cx] += thick[i]; }
                    }
                }
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
                    const shade = 1 - (0.25 + 0.4 * s) * t;
                    d[o] = Math.round(r * shade); d[o + 1] = Math.round(g * shade); d[o + 2] = Math.round(b * shade);
                    d[o + 3] = Math.round(255 * (0.55 + 0.45 * s));
                } else {
                    d[o + 3] = 0;
                }
            }
            fctx.putImageData(img, 0, 0);
            ctx.imageSmoothingEnabled = false;
            ctx.drawImage(fluidCanvas, 0, 0, FW * CELL, FH * CELL);
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
            [3.1, () => { window.SFX && SFX.play('vomit', 1); splash(520); }],
            [5.0, () => { window.SFX && SFX.play('vomit2', 1); splash(360); }],
        ];

        function splash(count) {
            for (let k = 0; k < count; k++) {
                const x = latin.minX + Math.random() * (latin.maxX - latin.minX);
                // the freshest splash is always half water
                emit(x, latin.minY - 4 - Math.random() * 10, (Math.random() - 0.5) * 0.4, 0.3 + Math.random() * 0.4,
                     k % 2 ? 0 : 1);
            }
        }

        function frame(now) {
            if (finished) return;
            const t = (now - t0) / 1000;
            while (stage < cues.length && t >= cues[stage][0]) cues[stage++][1]();

            const fill = filledCount / Math.max(1, cyr.count);
            if (t > 3.6 && fill < 0.97 && t < 12) feed(4);
            if (!doneAt && t > 3.6 && (fill >= 0.97 || t >= 12)) {
                for (const h of holes) if (!filled[h]) { filled[h] = 1; filledCount++; }
                doneAt = t;
                window.SFX && SFX.play('balalaika', 1);
            }
            step(); step();

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

            drawFluid();

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
