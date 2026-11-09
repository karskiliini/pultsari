// PПLTSAЯI 2 browser client: draws the game's JSON frames as ASCII on a canvas
// and layers pixel effects (blood, explosions) on top. No terminal involved.
(function () {
    const COLS = 80, ROWS = 28;
    const CW = 10, CH = 18;          // normal cell size in CSS pixels
    const W = COLS * CW, H = ROWS * CH;
    // the grid can grow when the camera zooms out; cells shrink to fit the canvas
    let cols = COLS, rows = ROWS, cw = CW, ch = CH;
    const FONT = '15px ui-monospace, "Cascadia Code", Menlo, Consolas, monospace';
    const FONT_BOLD = 'bold ' + FONT;

    // ---------- SGR colour parsing ----------
    const BASE = ['#000000', '#c23621', '#25bc24', '#adad27', '#3b5bdb', '#d338d3', '#33bbc8', '#cbcccd',
                  '#818383', '#fc391f', '#31e722', '#eaec23', '#6d8bff', '#f935f8', '#14f0f0', '#f2f2f2'];
    function xterm256(n) {
        if (n < 16) return BASE[n];
        if (n >= 232) { const v = 8 + (n - 232) * 10; return `rgb(${v},${v},${v})`; }
        n -= 16;
        const lv = (c) => (c === 0 ? 0 : 55 + c * 40);
        return `rgb(${lv(Math.floor(n / 36))},${lv(Math.floor(n / 6) % 6)},${lv(n % 6)})`;
    }
    const sgrCache = new Map();
    function parseSgr(sgr) {
        let st = sgrCache.get(sgr);
        if (st) return st;
        st = { fg: '#e6e6e6', bg: null, bold: false, dim: false };
        let fgIndex = -1;
        const p = sgr ? sgr.split(';').map(Number) : [];
        for (let i = 0; i < p.length; i++) {
            const c = p[i];
            if (c === 0) { st = { fg: '#e6e6e6', bg: null, bold: false, dim: false }; fgIndex = -1; }
            else if (c === 1) st.bold = true;
            else if (c === 2) st.dim = true;
            else if (c >= 30 && c <= 37) { fgIndex = c - 30; st.fg = BASE[fgIndex]; }
            else if (c >= 90 && c <= 97) { fgIndex = -1; st.fg = BASE[c - 90 + 8]; }
            else if (c >= 40 && c <= 47) st.bg = BASE[c - 40];
            else if (c >= 100 && c <= 107) st.bg = BASE[c - 100 + 8];
            else if (c === 38 && p[i + 1] === 5) { st.fg = xterm256(p[i + 2]); fgIndex = -1; i += 2; }
            else if (c === 48 && p[i + 1] === 5) { st.bg = xterm256(p[i + 2]); i += 2; }
        }
        // like a terminal, bold makes the basic colours bright
        if (st.bold && fgIndex >= 0) st.fg = BASE[fgIndex + 8];
        sgrCache.set(sgr, st);
        return st;
    }

    // ---------- state ----------
    let canvas, ctx, ui = {};
    let frame = null;
    let invMenu = null, invMenuAt = 0;
    let aim = null, aimAt = 0;
    const particles = [];
    const stains = [];
    const flashes = [];
    const MAX_STAINS = 4000;
    let lastTime = performance.now();
    let mood = { b: 1, s: 1 };

    // colour mood follows the promilles: near a hangover the world goes dark and
    // grey, drunk it gets bright and vivid, very drunk the colours start to swim
    function moodFor(prom) {
        const stops = [[0, 0.55, 0.3], [10, 1.0, 1.0], [30, 1.25, 1.8], [50, 1.35, 2.3]];
        if (prom <= stops[0][0]) return { b: stops[0][1], s: stops[0][2] };
        for (let i = 1; i < stops.length; i++) {
            const [p1, b1, s1] = stops[i], [p0, b0, s0] = stops[i - 1];
            if (prom <= p1) {
                const k = (prom - p0) / (p1 - p0);
                return { b: b0 + (b1 - b0) * k, s: s0 + (s1 - s0) * k };
            }
        }
        return { b: stops[3][1], s: stops[3][2] };
    }

    function setup(els) {
        ui = els;
        canvas = els.canvas;
        const dpr = window.devicePixelRatio || 1;
        canvas.width = COLS * CW * dpr;
        canvas.height = ROWS * CH * dpr;
        canvas.style.width = COLS * CW + 'px';
        canvas.style.height = ROWS * CH + 'px';
        ctx = canvas.getContext('2d');
        ctx.scale(dpr, dpr);
        requestAnimationFrame(render);
    }

    // world cell -> screen pixel (cell centre)
    function toScreen(wx, wy) {
        if (!frame) return [0, 0];
        return [(wx - frame.cam[0]) * cw + cw / 2, (wy - frame.cam[1]) * ch + ch / 2];
    }

    // ---------- effects ----------
    const BLOOD = ['#7a0a0a', '#8f1010', '#a31515', '#5c0606', '#b51c1c'];
    function blood(m) {
        const count = 8 + m.n * 6;
        const bx = m.dx || 0, by = m.dy || 0;
        for (let i = 0; i < count; i++) {
            let a = Math.random() * Math.PI * 2;
            if (bx || by) a = Math.atan2(by, bx) + (Math.random() - 0.5) * 1.6;
            const sp = 40 + Math.random() * (bx || by ? 260 : 160);
            particles.push({
                wx: m.x + 0.5, wy: m.y + 0.5,
                ox: 0, oy: 0, vx: Math.cos(a) * sp, vy: Math.sin(a) * sp,
                life: 0.25 + Math.random() * 0.45, color: BLOOD[(Math.random() * BLOOD.length) | 0],
                size: Math.random() < 0.25 ? 3 : 2, stain: true,
            });
        }
    }
    function boom(m) {
        flashes.push({ wx: m.x + 0.5, wy: m.y + 0.5, r: (m.r || 3) * CW * 1.4, t: 0 });
        addStain(m.x + 0.5, m.y + 0.5, 0, 0, 'rgba(20,16,12,0.55)', (m.r || 3) * CW * 0.9, true);
        for (let i = 0; i < 70; i++) {
            const a = Math.random() * Math.PI * 2, sp = 60 + Math.random() * 320;
            particles.push({
                wx: m.x + 0.5, wy: m.y + 0.5, ox: 0, oy: 0, vx: Math.cos(a) * sp, vy: Math.sin(a) * sp,
                life: 0.2 + Math.random() * 0.6, color: Math.random() < 0.5 ? '#ffb030' : '#5a5a5a',
                size: 2, stain: Math.random() < 0.3,
            });
        }
    }
    function addStain(wx, wy, ox, oy, color, size, round) {
        stains.push({ wx, wy, ox, oy, color, size, round });
        if (stains.length > MAX_STAINS) stains.splice(0, stains.length - MAX_STAINS);
    }

    // ---------- messages from the game ----------
    function handle(m) {
        switch (m.t) {
            case 'frame':
                frame = m;
                cols = m.cols || COLS;
                rows = m.rows.length || ROWS;
                cw = W / cols;
                ch = H / rows;
                updateUi(m);
                if (!m.dead) ui.death.hidden = true;
                break;
            case 'death':
                ui.death.hidden = false;
                ui.deathCause.textContent = m.cause;
                ui.deathScore.textContent = `Hakatut mummot ${m.mummot} · Kaikki hakatut ${m.kaikki}`;
                break;
            case 'restart': if (ui.onRestart) ui.onRestart(); break;
            case 'invmenu': invMenu = m; invMenuAt = performance.now(); break;
            case 'aim': aim = m; aimAt = performance.now(); break;
            case 'snd': if (window.SFX) SFX.play(m.n, m.v); break;
            case 'hit': blood(m); break;
            case 'boom': boom(m); break;
            case 'clear': ui.text.textContent = ''; ui.text.hidden = false; break;
            case 'text': ui.text.hidden = false; ui.text.textContent += m.s; break;
            case 'score':
                ui.text.hidden = false;
                ui.text.textContent += `\n\nПРОТОКОЛ · PÖYTÄKIRJA\n\nHakatut mummot   ${m.mummot}\nKaikki hakatut   ${m.kaikki}\nPISTEET          ${m.total}\n\nВсё идёт по плану. Kiitos pelaamisesta.`;
                break;
        }
    }

    function updateUi(m) {
        ui.msg.textContent = m.msg ? '» ' + m.msg : '';
        ui.level.textContent = m.lvl || '';
        if (m.hud) {
            const [money, hp, prom, belly] = m.hud;
            ui.money.textContent = money + ' ₽';
            ui.hp.textContent = hp;
            ui.hp.className = hp < 5 ? 'bad' : '';
            ui.prom.textContent = (prom / 10).toFixed(1) + ' ‰';
            ui.prom.className = prom < 10 ? 'good' : prom < 30 ? 'warn' : 'bad';
            ui.belly.textContent = belly + '%';
            ui.belly.className = belly <= 20 ? 'bad' : belly <= 50 ? 'warn' : '';
        }
        ui.sounds.innerHTML = '';
        for (const [text, loud] of (m.snd || []).slice(0, 4)) {
            const s = document.createElement('span');
            s.textContent = text;
            if (loud) s.className = 'loud';
            ui.sounds.appendChild(s);
        }
        ui.inv.innerHTML = '';
        if (!m.inv || !m.inv.length) ui.inv.innerHTML = '<li class="empty">(tyhjä)</li>';
        for (const [name, count] of (m.inv || [])) {
            const li = document.createElement('li');
            li.innerHTML = `<span></span><b></b>`;
            li.firstChild.textContent = name;
            li.lastChild.textContent = count;
            ui.inv.appendChild(li);
        }
    }

    // ---------- drawing ----------
    function render(now) {
        const dt = Math.min(0.05, (now - lastTime) / 1000);
        lastTime = now;
        if (frame && frame.hud) {
            const prom = frame.hud[2];
            const target = moodFor(prom);
            mood.b += (target.b - mood.b) * Math.min(1, dt * 1.5);
            mood.s += (target.s - mood.s) * Math.min(1, dt * 1.5);
            const swim = prom > 30 ? Math.sin(now / 900) * Math.min(25, (prom - 30) * 1.2) : 0;
            canvas.style.filter = `brightness(${mood.b.toFixed(3)}) saturate(${mood.s.toFixed(3)}) hue-rotate(${swim.toFixed(1)}deg)`;
        }
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, W, H);
        const scale = cw / CW;         // effects shrink with the zoom
        const tiny = cw < 5;           // too small for letters: draw each glyph as a pixel block
        const font = `${Math.max(4, Math.round(ch * 0.83))}px ui-monospace, "Cascadia Code", Menlo, Consolas, monospace`;
        ctx.textBaseline = 'middle';
        ctx.textAlign = 'center';

        if (frame) {
            // backgrounds
            frame.rows.forEach((runs, y) => {
                let x = 0;
                for (const [text, sgr] of runs) {
                    const st = parseSgr(sgr);
                    const n = Array.from(text).length;
                    if (st.bg) { ctx.fillStyle = st.bg; ctx.fillRect(x * cw, y * ch, n * cw + 0.5, ch + 0.5); }
                    x += n;
                }
            });
            // stains lie on the ground under everyone
            for (const s of stains) {
                const [sx, sy] = toScreen(s.wx - 0.5, s.wy - 0.5);
                const px = sx + s.ox * scale, py = sy + s.oy * scale;
                if (px < -20 || py < -20 || px > W + 20 || py > H + 20) continue;
                ctx.fillStyle = s.color;
                const size = Math.max(1, s.size * scale);
                if (s.round) { ctx.beginPath(); ctx.arc(px, py, size, 0, Math.PI * 2); ctx.fill(); }
                else ctx.fillRect(Math.round(px), Math.round(py), size, size);
            }
            // glyphs
            frame.rows.forEach((runs, y) => {
                let x = 0;
                for (const [text, sgr] of runs) {
                    const st = parseSgr(sgr);
                    ctx.font = cw === CW ? (st.bold ? FONT_BOLD : FONT) : (st.bold ? 'bold ' : '') + font;
                    ctx.fillStyle = st.fg;
                    ctx.globalAlpha = st.dim ? 0.55 : 1;
                    for (const g of text) {
                        if (g !== ' ') {
                            if (tiny) {
                                if (g !== '.') ctx.fillRect(x * cw, y * ch + ch * 0.2, Math.max(1, cw), Math.max(1, ch * 0.6));
                            } else {
                                ctx.fillText(g, x * cw + cw / 2, y * ch + ch / 2 + 1);
                            }
                        }
                        x++;
                    }
                }
            });
            ctx.globalAlpha = 1;
        }

        // flying droplets and debris; blood that lands becomes a stain
        for (let i = particles.length - 1; i >= 0; i--) {
            const p = particles[i];
            p.ox += p.vx * dt; p.oy += p.vy * dt;
            p.vx *= Math.pow(0.04, dt); p.vy *= Math.pow(0.04, dt);
            p.life -= dt;
            const [sx, sy] = toScreen(p.wx - 0.5, p.wy - 0.5);
            ctx.fillStyle = p.color;
            ctx.fillRect(Math.round(sx + p.ox * scale), Math.round(sy + p.oy * scale), Math.max(1, p.size * scale), Math.max(1, p.size * scale));
            if (p.life <= 0) {
                if (p.stain) addStain(p.wx, p.wy, p.ox, p.oy, p.color, p.size, false);
                particles.splice(i, 1);
            }
        }
        for (let i = flashes.length - 1; i >= 0; i--) {
            const f = flashes[i];
            f.t += dt;
            const k = f.t / 0.35;
            if (k >= 1) { flashes.splice(i, 1); continue; }
            const [sx, sy] = toScreen(f.wx - 0.5, f.wy - 0.5);
            const r = f.r * scale;
            const g = ctx.createRadialGradient(sx, sy, 0, sx, sy, r * (0.6 + k));
            g.addColorStop(0, `rgba(255,240,180,${0.9 * (1 - k)})`);
            g.addColorStop(0.4, `rgba(255,140,30,${0.6 * (1 - k)})`);
            g.addColorStop(1, 'rgba(255,60,0,0)');
            ctx.fillStyle = g;
            ctx.fillRect(sx - r * 2, sy - r * 2, r * 4, r * 4);
        }

        // aiming line, and the accidental-discharge meter while a pistol is cocked
        if (aim && now - aimAt < 250) {
            // a laser from the player's hand along the aim, as far as the line of fire reaches
            const steps = aim.cells.length;
            if (aim.p) {
                const x0 = aim.p[0] * CW + CW / 2, y0 = aim.p[1] * CH + CH / 2;
                const len = steps + 0.5;
                const x1 = x0 + Math.cos(aim.a) * len * CW, y1 = y0 + Math.sin(aim.a) * len * CH;
                const alpha = aim.low ? 0.45 : 0.9;
                ctx.lineCap = 'round';
                ctx.strokeStyle = `rgba(255,30,30,${alpha * 0.35})`;
                ctx.lineWidth = 4;
                ctx.beginPath(); ctx.moveTo(x0, y0); ctx.lineTo(x1, y1); ctx.stroke();
                ctx.strokeStyle = `rgba(255,70,70,${alpha})`;
                ctx.lineWidth = 1;
                ctx.beginPath(); ctx.moveTo(x0, y0); ctx.lineTo(x1, y1); ctx.stroke();
                ctx.fillStyle = `rgba(255,90,90,${alpha})`;
                ctx.beginPath(); ctx.arc(x1, y1, 2.5, 0, Math.PI * 2); ctx.fill();
            }
            if (aim.risk >= 0) {
                const w = 220, h = 12, x0 = (COLS * CW - w) / 2, y0 = 10;
                ctx.fillStyle = 'rgba(0,0,0,0.75)';
                ctx.fillRect(x0 - 8, y0 - 6, w + 16, h + 26);
                ctx.strokeStyle = '#888';
                ctx.strokeRect(x0, y0 + 14, w, h);
                const r = Math.min(1, aim.risk);
                ctx.fillStyle = `hsl(${120 - 120 * r}, 85%, 50%)`;
                ctx.fillRect(x0 + 1, y0 + 15, (w - 2) * r, h - 2);
                ctx.fillStyle = '#eee';
                ctx.font = 'bold 12px ui-monospace, Menlo, monospace';
                ctx.textAlign = 'left';
                ctx.fillText('LAUKAISUHERKKYYS', x0, y0 + 5);
                ctx.textAlign = 'right';
                ctx.fillText(Math.round(r * 100) + ' %', x0 + w, y0 + 5);
                ctx.textAlign = 'center';
            }
        }
        drawInvMenu(now);
        requestAnimationFrame(render);
    }

    function drawInvMenu(now) {
        const open = invMenu && now - invMenuAt < 250;
        ui.menu.hidden = !open;
        if (!open) return;
        const key = JSON.stringify(invMenu);
        if (ui.menu.dataset.key === key) return;
        ui.menu.dataset.key = key;
        const rows = invMenu.items.map(([name, count, use, canThrow], i) => {
            const sel = i === invMenu.sel ? ' class="sel"' : count ? '' : ' class="none"';
            return `<tr${sel}><td>${i === invMenu.sel ? '▶' : ''}</td><td>${name}</td><td>×${count}</td><td>${use}</td><td>${canThrow ? '[F]' : ''}</td></tr>`;
        });
        ui.menu.querySelector('tbody').innerHTML = rows.join('');
        ui.menu.querySelector('h3').textContent = invMenu.throw ? 'HEITÄ MITÄ?' : 'ТОВАРЫ · TAVARAT';
    }

    window.Game = { setup, handle };
})();
