// Browser intro: "Russki Mir" gets vomited on and the vomit, simulated as a
// simple particle fluid, oozes off the letters and drips into "Русский Мир"
// on the line below. Space skips straight to the game.
(function () {
    const CELL = 2; // fluid resolution in CSS pixels: a chunky, pixel-exact look

    function play(container, onDone) {
        const W = container.clientWidth;
        const H = container.clientHeight;
        const dpr = window.devicePixelRatio || 1;

        const canvas = document.createElement('canvas');
        canvas.width = Math.round(W * dpr);
        canvas.height = Math.round(H * dpr);
        Object.assign(canvas.style, { position: 'absolute', inset: '0', width: W + 'px', height: H + 'px', background: '#000', zIndex: 5 });
        container.appendChild(canvas);
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
        const FH = Math.ceil(H / CELL);
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
        const floorCell = Math.floor(floorY / CELL);

        // ---- fluid: particles with gravity, pressure, cohesion and viscosity ----
        const MAX = 1600;
        const px = new Float32Array(MAX), py = new Float32Array(MAX);
        const vx = new Float32Array(MAX), vy = new Float32Array(MAX);
        let n = 0;
        const G = 0.03, R0 = 1.15, H_ = 2.3, K = 0.09, COH = 0.006, VISC = 0.09;
        const grid = new Map();

        function emit(x, y, ux, uy) {
            if (n >= MAX) return;
            px[n] = x; py[n] = y; vx[n] = ux; vy[n] = uy; n++;
        }
        function remove(i) {
            n--;
            px[i] = px[n]; py[i] = py[n]; vx[i] = vx[n]; vy[i] = vy[n];
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
                        const f = d < R0 ? -K * (R0 - d) : COH * (1 - d / H_);
                        ax += f * dx / d; ay += f * dy / d;
                        const w = VISC * (1 - d / H_);
                        ax += (vx[j] - vx[i]) * w; ay += (vy[j] - vy[i]) * w;
                    }
                }
                vx[i] += ax; vy[i] += ay;
                // the vomit sticks to the letters and oozes slowly off their bottoms
                if (at(latin.mask, px[i], py[i]) || at(latin.mask, px[i], py[i] + 1)) { vx[i] *= 0.82; vy[i] *= 0.86; }
                const sp = Math.hypot(vx[i], vy[i]);
                if (sp > 1.4) { vx[i] *= 1.4 / sp; vy[i] *= 1.4 / sp; }
            }
            for (let i = n - 1; i >= 0; i--) {
                px[i] += vx[i]; py[i] += vy[i];
                if (px[i] < 1) { px[i] = 1; vx[i] *= -0.3; }
                if (px[i] > FW - 2) { px[i] = FW - 2; vx[i] *= -0.3; }
                if (py[i] >= floorCell) { py[i] = floorCell; vy[i] *= -0.1; vx[i] *= 0.92; }
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
                emit(sx + Math.random() - 0.5, latin.maxY + Math.random(), 0, 0.05);
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

        // the poured word: a green gradient from bright bile at the top to dark at the bottom
        function wordColor(cy, cx) {
            const t = (cy - cyr.minY) / Math.max(1, cyr.maxY - cyr.minY);
            const shimmer = ((cx * 7 + cy * 13) % 5) * 4;
            return [Math.round(196 - 140 * t + shimmer), Math.round(238 - 110 * t + shimmer), Math.round(92 - 72 * t)];
        }

        function drawFluid() {
            density.fill(0);
            for (let i = 0; i < n; i++) {
                const cx = px[i] | 0, cy = py[i] | 0;
                for (let oy = -1; oy <= 1; oy++) for (let ox = -1; ox <= 1; ox++) {
                    const x = cx + ox, y = cy + oy;
                    if (x < 0 || y < 0 || x >= FW || y >= FH) continue;
                    density[y * FW + x] += (ox === 0 && oy === 0) ? 1 : (ox === 0 || oy === 0) ? 0.55 : 0.3;
                }
            }
            const d = img.data;
            for (let i = 0; i < FW * FH; i++) {
                const o = i * 4;
                if (filled[i]) {
                    const [r, g, b] = wordColor((i / FW) | 0, i % FW);
                    d[o] = r; d[o + 1] = g; d[o + 2] = b; d[o + 3] = 255;
                } else if (density[i] > 0.85) {
                    // thicker fluid is darker green, thin edges catch the light
                    const t = Math.min(1, (density[i] - 0.85) / 2.5);
                    d[o] = Math.round(175 - 120 * t); d[o + 1] = Math.round(225 - 100 * t); d[o + 2] = Math.round(70 - 55 * t); d[o + 3] = 255;
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
            ctx.fillStyle = color;
            ctx.fillText(str, x, y);
        }

        // ---- timeline ----
        let t0 = performance.now();
        let stage = 0;
        let doneAt = 0;
        let finished = false;
        const cues = [
            [2.6, () => window.SFX && SFX.play('cough', 1)],
            [3.1, () => { window.SFX && SFX.play('vomit', 1); splash(520); }],
            [5.0, () => { window.SFX && SFX.play('vomit2', 1); splash(360); }],
        ];

        function splash(count) {
            for (let k = 0; k < count; k++) {
                const x = latin.minX + Math.random() * (latin.maxX - latin.minX);
                emit(x, latin.minY - 4 - Math.random() * 10, (Math.random() - 0.5) * 0.4, 0.3 + Math.random() * 0.4);
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

            ctx.fillStyle = '#000';
            ctx.fillRect(0, 0, W, H);

            const typed = Math.min(titleText.length, Math.floor(t / 0.11));
            text(titleText.slice(0, typed), startX, baseY, '#fff');
            if (t > 1.6) text(latinText, latinX, baseY, t > 3.1 ? '#b9d36a' : '#fff');
            if (t > 2.6) text('*RÖKH*', startX - fs * 0.2, baseY + fs * 0.9, '#e0b030', fs * 0.42);
            if (t > 3.1) text('*BLAAARGH*', startX - fs * 0.6, baseY + fs * 1.6, '#f0c040', fs * 0.5);
            if (t > 5.0) text('*ÖÖÖRRRKKK*', latinX + latinW + fs * 0.3, baseY + fs * 0.6, '#f0c040', fs * 0.45);

            drawFluid();

            if (doneAt) {
                text('♪ Балалайка ♪  tiling-tiling-tiling...',
                     W / 2 - fs * 6.2, floorY + fs * 1.4, '#888', fs * 0.4);
                if (t - doneAt > 1.2 && Math.floor(t * 2) % 2 === 0) {
                    text('Paina mitä tahansa näppäintä  ·  Välilyönti aloittaa heti',
                         W / 2 - fs * 7.6, H - fs * 1.2, '#ddd', fs * 0.4);
                }
            }
            requestAnimationFrame(frame);
        }

        function finish() {
            if (finished) return;
            finished = true;
            window.removeEventListener('keydown', onKey, true);
            canvas.remove();
            onDone();
        }
        // keys never reach the page during the intro; Space starts the game at once
        function onKey(e) {
            e.preventDefault();
            e.stopPropagation();
            const t = (performance.now() - t0) / 1000;
            if (e.key === ' ' || (doneAt && t - doneAt > 1.2)) finish();
        }
        window.addEventListener('keydown', onKey, true);
        canvas.addEventListener('click', () => { if (doneAt) finish(); });
        requestAnimationFrame(frame);
    }

    window.WebIntro = { play };
})();
