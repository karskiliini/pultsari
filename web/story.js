// The backstory, between the intro and the game: the drunk sits on the ground leaning on an
// empty bin, an empty beer bottle in his hands, his mate beside him. The mate has heard that
// Vova keeps an endless supply of vodka in his cellar. A light goes on over the drunk's head
// and off he goes to fetch his share.
// The scenes are ASCII art drawn here. A panel's picture can be swapped for one converted with
// tools/video2ascii.py (an image or a short clip, e.g. from Grok Imagine): save it as
// web/video/story-1.json ... story-5.json and it is drawn instead of the hand-made scene.
(function () {
    const COLS = 72;               // the stage is 72 characters wide
    const FONT = 'ui-monospace, "Cascadia Code", Menlo, monospace';

    const DRUNK = '#d8d2b0', SKIN = '#e6e0d0', COAT = '#8a7a5a', MATE = '#9aa0a8', BOTTLE = '#5f8f4a';
    const BIN = '#7d8a86', NIGHT = '#556', KREMLIN = '#7a3a34', STAR = '#d52b1e', LAMP = '#f2d34a';

    const BUBBLE = 'Kuulitko? Vovalla on kuulemma rajaton määrä viinaa kellarissaan.';
    const panels = [
        { dur: 5.5, say: 'Leninski prospekt, myöhään illalla. Pullo on tyhjä.' },
        { dur: 7.0, say: '' },
        { dur: 4.5, say: '...rajaton?' },
        { dur: 5.5, say: 'Pultsari päättää lähteä hakemaan siitä oman osansa.' },
        { dur: 5.5, say: '' },
    ];

    // night sky: the same stars every time
    const stars = [];
    for (let i = 0, s = 7; i < 40; i++) {
        s = (s * 9301 + 49297) % 233280; const x = (s / 233280) * COLS;
        s = (s * 9301 + 49297) % 233280; const y = (s / 233280) * 7;
        stars.push([x, y, i % 5 === 0 ? '*' : '.']);
    }

    const clips = [];
    function xterm256(n) {
        if (n < 16) return '#888';
        if (n >= 232) { const v = 8 + (n - 232) * 10; return `rgb(${v},${v},${v})`; }
        n -= 16;
        const lv = (c) => (c === 0 ? 0 : 55 + c * 40);
        return `rgb(${lv(Math.floor(n / 36))},${lv(Math.floor(n / 6) % 6)},${lv(n % 6)})`;
    }
    function loadClips() {
        return Promise.all(panels.map((_, i) =>
            fetch(`video/story-${i + 1}.json`, { cache: 'no-store' })
                .then((r) => (r.ok ? r.json() : null))
                .then((c) => {
                    if (!c || !c.frames || !c.frames.length) return;
                    c.colors = c.frames.map((f) => Uint8Array.from(atob(f.k), (ch) => ch.charCodeAt(0)));
                    clips[i] = c;
                })
                .catch(() => {})));
    }

    function play(container, onDone) {
        const rect = container.getBoundingClientRect();
        const W = container.clientWidth, H = container.clientHeight;
        const dpr = window.devicePixelRatio || 1;
        const canvas = document.createElement('canvas');
        canvas.width = Math.round(W * dpr);
        canvas.height = Math.round(H * dpr);
        Object.assign(canvas.style, { position: 'fixed', left: rect.left + 'px', top: rect.top + 'px',
                                      width: W + 'px', height: H + 'px', background: '#000', zIndex: 50 });
        document.body.appendChild(canvas);
        const ctx = canvas.getContext('2d');
        ctx.scale(dpr, dpr);

        // a grid of character cells; the art is laid out in columns and rows
        const cw = W / COLS;
        const fs = cw / 0.6;
        const ch = fs * 1.12;
        const ground = Math.min(16, Math.floor(H / ch) - 6);
        const px = (c) => c * cw, py = (r) => r * ch;
        const font = (scale = 1, bold = false) => { ctx.font = `${bold ? 'bold ' : ''}${Math.round(fs * scale)}px ${FONT}`; };

        // a sprite: rows of text, each in its own colour, at a column and the row of its top line
        function sprite(rows, col, row, color, angle = 0) {
            ctx.save();
            ctx.translate(px(col), py(row));
            if (angle) ctx.rotate(angle);
            font();
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            rows.forEach((r, i) => {
                const [s, c] = Array.isArray(r) ? r : [r, color];
                ctx.fillStyle = c;
                ctx.fillText(s, 0, i * ch);
            });
            ctx.restore();
        }

        // ---- the scenery ----
        function sky(t) {
            font();
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            for (const [x, y, c] of stars) {
                ctx.fillStyle = (Math.floor(t * 2 + x) % 7 === 0) ? '#889' : '#445';
                ctx.fillText(c, px(x), py(y));
            }
        }
        function kremlin(col, scale = 1) {
            const rows = [
                ['    *          *          *    ', STAR],
                ['   /^\\        /^\\        /^\\   ', KREMLIN],
                ['   |#|   *    |#|   *    |#|   ', KREMLIN],
                ['   |#|  /^\\   |#|  /^\\   |#|   ', KREMLIN],
                ['___|#|__|#|___|#|__|#|___|#|___', KREMLIN],
                ['|  _   _   _   _   _   _   _  |', KREMLIN],
            ];
            ctx.save();
            ctx.globalAlpha = 0.75;
            if (scale !== 1) {
                ctx.translate(px(col), py(ground - 6 * scale + 1));
                ctx.scale(scale, scale);
                sprite(rows, 0, 0);
            } else sprite(rows, col, ground - 5);
            ctx.restore();
        }
        function lampPost(col, flicker) {
            // the light falls in a cone onto the pavement
            const lx = px(col + 2), ly = py(ground - 9);
            const g = ctx.createRadialGradient(lx, py(ground), 0, lx, py(ground), px(16));
            g.addColorStop(0, `rgba(242,211,74,${0.16 * flicker})`);
            g.addColorStop(1, 'rgba(242,211,74,0)');
            ctx.fillStyle = g;
            ctx.beginPath();
            ctx.moveTo(lx - px(1), ly + ch);
            ctx.lineTo(lx + px(1), ly + ch);
            ctx.lineTo(lx + px(17), py(ground + 1));
            ctx.lineTo(lx - px(13), py(ground + 1));
            ctx.closePath();
            ctx.fill();
            sprite([[' __ ', '#777'], ['(  )', `rgba(242,211,74,${0.5 + 0.5 * flicker})`], [' || ', '#777'], [' || ', '#777'],
                    [' || ', '#777'], [' || ', '#777'], [' || ', '#777'], [' || ', '#777'], [' || ', '#777'], ['_||_', '#777']],
                   col, ground - 9);
        }
        function street() {
            font();
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            ctx.fillStyle = '#555';
            ctx.fillText('_'.repeat(COLS), 0, py(ground + 0.45));
            ctx.fillStyle = '#333';
            ctx.fillText('  .   ,    .      `    .   ,        .     ,    `   .      ,   .    ,   ', 0, py(ground + 1.4));
        }
        function bin(col) {
            sprite([
                ' _________ ',
                '[_________]',
                ' |   |   | ',
                ' |   |   | ',
                ' |   |   | ',
                ' |___|___| ',
            ], col, ground - 5, BIN);
        }

        // ---- the two of them ----
        // the drunk sits on the ground right of the bin, leaning on it, the empty bottle in both hands
        function drunkSitting(col, face, t) {
            const lean = -0.09 + Math.sin(t * 1.3) * 0.03;   // to the left, onto the bin
            sprite([
                ['   ___', COAT],
                [`  ${face}`, SKIN],
                [' /|###|\\', DRUNK],
                ['  \\_[]_/', DRUNK],
                ['  _/   \\_', DRUNK],
                [' (__) (__)', '#9a8a6a'],
            ], col, ground - 5, DRUNK, lean);
            // the empty bottle in both hands, in his lap
            sprite(['', '', '', ['     []', BOTTLE]], col, ground - 5, BOTTLE, lean);
        }
        // his mate sits on the other side of the bin, in a cap, with a beard
        function mateSitting(col, leaning, t) {
            const nod = leaning ? 0.12 + Math.sin(t * 6) * 0.02 : 0.04;
            sprite([
                ['  ,___,  ', '#666'],
                ['  (o.o)  ', SKIN],
                ['  \\www/  ', '#aaa'],
                [' /|===|\\ ', MATE],
                ['  _/ \\_  ', MATE],
                [' (__)(__)', '#777'],
            ], col, ground - 5, MATE, nod);
        }
        function drunkWalking(col, row, t, face) {
            const step = Math.floor(t * 5) % 2;
            sprite([
                ['  ___', COAT],
                [` ${face}`, SKIN],
                [' /|#|\\_[]', DRUNK],
                ['  |_|', DRUNK],
                [step ? '  / \\' : '  |\\ ', DRUNK],
            ], col, row, DRUNK);
        }

        // a speech bubble pointing down at whoever speaks, its text typed out
        function bubble(text, shown, col, row, toCol, toRow) {
            font(0.9);
            const maxW = px(30);
            const words = text.split(' ');
            const lines = [];
            let line = '';
            for (const w of words) {
                const next = line ? line + ' ' + w : w;
                if (ctx.measureText(next).width > maxW && line) { lines.push(line); line = w; } else line = next;
            }
            if (line) lines.push(line);
            const lh = fs * 1.1, padX = cw * 1.2, padY = lh * 0.4;
            const bw = Math.max(...lines.map((l) => ctx.measureText(l).width)) + padX * 2;
            const bh = lines.length * lh + padY * 2;
            const x = px(col), y = py(row);
            ctx.fillStyle = '#f2f0e6';
            ctx.strokeStyle = '#222';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.roundRect(x, y, bw, bh, lh * 0.6);
            ctx.fill();
            ctx.stroke();
            ctx.beginPath();
            const tx = Math.min(x + bw - padX, Math.max(x + padX, px(toCol)));
            ctx.moveTo(tx - cw, y + bh - 1);
            ctx.lineTo(px(toCol), py(toRow));
            ctx.lineTo(tx + cw, y + bh - 1);
            ctx.fill();
            ctx.fillStyle = '#111';
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            let left = shown;
            lines.forEach((l, i) => {
                const part = l.slice(0, Math.max(0, left));
                left -= l.length + 1;
                ctx.fillText(part, x + padX, y + padY + i * lh);
            });
        }

        // the light bulb over his head: it flickers on, then shines
        function bulb(col, row, k) {
            const on = k > 0.35 || (k > 0.1 && Math.floor(k * 40) % 3 !== 0);
            const glow = on ? 1 : 0.15;
            if (on) {
                const g = ctx.createRadialGradient(px(col + 2.5), py(row + 1.2), 0, px(col + 2.5), py(row + 1.2), px(5));
                g.addColorStop(0, 'rgba(255,230,120,0.45)');
                g.addColorStop(1, 'rgba(255,230,120,0)');
                ctx.fillStyle = g;
                ctx.fillRect(px(col - 3), py(row - 2), px(11), py(6));
            }
            const ray = on && k > 0.4 ? LAMP : '#000';
            sprite([
                [' \\ | / ', ray],
                ['- ( ) -', `rgba(255,230,120,${glow})`],
                ['   =   ', '#aaa'],
            ], col, row, LAMP);
        }

        function caption(text, k) {
            if (!text) return;
            font(0.85);
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            const shown = text.slice(0, Math.floor(text.length * Math.min(1, k)));
            ctx.fillStyle = 'rgba(0,0,0,0.75)';
            ctx.fillRect(0, H - ch * 2.4, W, ch * 1.8);
            ctx.fillStyle = '#ddd';
            ctx.fillText(shown, W / 2, H - ch * 1.5);
        }

        function drawClip(clip, t) {
            const frame = Math.min(clip.frames.length - 1, Math.floor(t * clip.fps));
            const chars = clip.frames[frame].c, colors = clip.colors[frame];
            const cwc = W / clip.cols, chc = (H * 0.82) / clip.rows;
            ctx.font = `bold ${Math.max(6, Math.round(chc * 0.95))}px ${FONT}`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            for (let y = 0; y < clip.rows; y++) {
                for (let x = 0; x < clip.cols; x++) {
                    const i = y * clip.cols + x;
                    if (chars[i] === ' ') continue;
                    ctx.fillStyle = xterm256(colors[i]);
                    ctx.fillText(chars[i], x * cwc + cwc / 2, y * chc + chc / 2);
                }
            }
        }

        // ---- the panels ----
        const BIN_COL = 29, MATE_COL = 19.5, DRUNK_COL = 40.4;
        function scene(i, t, flicker) {
            sky(t);
            kremlin(50, 0.6);
            lampPost(8, flicker);
            street();
            bin(BIN_COL);
        }
        // each panel's figures; with a converted picture in its place only the overlays are drawn
        const render = [
            // 1: the two of them against the bin; *hic*
            (t, art) => {
                if (!art) return;
                drunkSitting(DRUNK_COL, '(x_x)', t);
                mateSitting(MATE_COL, false, t);
                if (Math.floor(t * 1.5) % 3 === 1) sprite([['*hik*', LAMP]], DRUNK_COL + 8, ground - 7);
            },
            // 2: the mate leans over and tells
            (t, art) => {
                if (art) {
                    drunkSitting(DRUNK_COL, t > 3.8 ? '(o_o)' : '(x_x)', t);
                    mateSitting(MATE_COL, true, t);
                }
                bubble(BUBBLE, Math.floor((t - 0.4) * 22), MATE_COL - 8, ground - 13, MATE_COL + 4, ground - 5.2);
            },
            // 3: the light goes on
            (t, art) => {
                if (!art) return;
                const k = Math.max(0, (t - 0.6) / 1.2);
                drunkSitting(DRUNK_COL, k > 0.4 ? '(O_O)' : '(o_o)', t);
                mateSitting(MATE_COL, false, t);
                bulb(DRUNK_COL + 0.5, ground - 9.5, k);
                if (k > 0.6) sprite([['!', LAMP]], DRUNK_COL + 8, ground - 7);
            },
            // 4: up and off toward the Kremlin; the mate stays put
            (t, art) => {
                if (!art) return;
                mateSitting(MATE_COL, false, t);
                // he gets up and walks off toward the Kremlin
                const k = Math.max(0, t - 0.5) / (panels[3].dur - 0.5);
                const col = DRUNK_COL + 1 + k * 34, row = ground - 4;
                drunkWalking(col, row, t, '(^_^)');
                if (t < 1.2) bulb(col - 0.5, row - 3.6, 1);
            },
            // 5: the closing line
            (t) => {
                kremlin(20.5, 1);
                font(1.35, true);
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                const line = 'Ties millainen tarina tästä vielä syntyy!?';
                const n = Math.floor(Math.max(0, t - 0.3) * 18);
                ctx.fillStyle = LAMP;
                ctx.fillText(line.slice(0, n), W / 2, H * 0.28);
                if (t > 3 && Math.floor(t * 2) % 2 === 0) {
                    font(0.6);
                    ctx.fillStyle = '#999';
                    ctx.fillText('Paina mitä tahansa näppäintä', W / 2, H * 0.28 + ch * 2.2);
                }
            },
        ];

        let index = 0, startAt = performance.now(), finished = false, pinged = false;
        function next() {
            if (++index >= panels.length) finish();
            else startAt = performance.now();
        }
        function frame(now) {
            if (finished) return;
            const t = (now - startAt) / 1000;
            if (t >= panels[index].dur) { next(); if (finished) return; }
            const lt = (now - startAt) / 1000;
            ctx.clearRect(0, 0, W, H);
            ctx.fillStyle = '#000';
            ctx.fillRect(0, 0, W, H);
            if (index < 4) {
                if (clips[index]) drawClip(clips[index], lt);
                else scene(index, lt, 0.8 + 0.2 * Math.sin(lt * 9) * Math.sin(lt * 2.3));
                render[index](lt, !clips[index]);
                caption(panels[index].say, (lt - 0.3) / 1.6);
            } else render[index](lt);
            // fade in from black at the start of each panel
            const fade = Math.max(0, 1 - lt / 0.35);
            if (fade > 0) { ctx.fillStyle = `rgba(0,0,0,${fade})`; ctx.fillRect(0, 0, W, H); }
            if (index === 2 && lt > 0.75 && !pinged) { pinged = true; if (window.SFX) SFX.play('drip', 1); }
            requestAnimationFrame(frame);
        }
        function finish() {
            if (finished) return;
            finished = true;
            window.removeEventListener('keydown', onKey, true);
            canvas.remove();
            onDone();
        }
        // Space skips the story; any other key or a click goes to the next picture
        function onKey(e) {
            e.preventDefault();
            e.stopPropagation();
            if (e.key === ' ' || e.key === 'Escape') finish(); else next();
        }
        window.addEventListener('keydown', onKey, true);
        canvas.addEventListener('click', next);
        loadClips().then(() => {});
        requestAnimationFrame(frame);
    }

    window.WebStory = { play };
})();
