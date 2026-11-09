// PПLTSAЯI 2 browser client: draws the game's JSON frames as ASCII on a canvas
// and layers pixel effects (blood, explosions) on top. No terminal involved.
(function () {
    const COLS = 80, ROWS = 28;
    const CW = 10, CH = 18;          // cell size in CSS pixels
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
        return [(wx - frame.cam[0]) * CW + CW / 2, (wy - frame.cam[1]) * CH + CH / 2];
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
            case 'frame': frame = m; updateUi(m); break;
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
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, COLS * CW, ROWS * CH);
        ctx.textBaseline = 'middle';
        ctx.textAlign = 'center';

        if (frame) {
            // backgrounds
            frame.rows.forEach((runs, y) => {
                let x = 0;
                for (const [text, sgr] of runs) {
                    const st = parseSgr(sgr);
                    const n = Array.from(text).length;
                    if (st.bg) { ctx.fillStyle = st.bg; ctx.fillRect(x * CW, y * CH, n * CW, CH); }
                    x += n;
                }
            });
            // stains lie on the ground under everyone
            for (const s of stains) {
                const [sx, sy] = toScreen(s.wx - 0.5, s.wy - 0.5);
                const px = sx + s.ox, py = sy + s.oy;
                if (px < -20 || py < -20 || px > COLS * CW + 20 || py > ROWS * CH + 20) continue;
                ctx.fillStyle = s.color;
                if (s.round) { ctx.beginPath(); ctx.arc(px, py, s.size, 0, Math.PI * 2); ctx.fill(); }
                else ctx.fillRect(Math.round(px), Math.round(py), s.size, s.size);
            }
            // glyphs
            frame.rows.forEach((runs, y) => {
                let x = 0;
                for (const [text, sgr] of runs) {
                    const st = parseSgr(sgr);
                    ctx.font = st.bold ? FONT_BOLD : FONT;
                    ctx.fillStyle = st.fg;
                    ctx.globalAlpha = st.dim ? 0.55 : 1;
                    for (const ch of text) {
                        if (ch !== ' ') ctx.fillText(ch, x * CW + CW / 2, y * CH + CH / 2 + 1);
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
            ctx.fillRect(Math.round(sx + p.ox), Math.round(sy + p.oy), p.size, p.size);
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
            const g = ctx.createRadialGradient(sx, sy, 0, sx, sy, f.r * (0.6 + k));
            g.addColorStop(0, `rgba(255,240,180,${0.9 * (1 - k)})`);
            g.addColorStop(0.4, `rgba(255,140,30,${0.6 * (1 - k)})`);
            g.addColorStop(1, 'rgba(255,60,0,0)');
            ctx.fillStyle = g;
            ctx.fillRect(sx - f.r * 2, sy - f.r * 2, f.r * 4, f.r * 4);
        }

        // aiming line
        if (aim && now - aimAt < 250) {
            ctx.fillStyle = '#ff4040';
            ctx.font = FONT_BOLD;
            for (const [x, y] of aim.cells) ctx.fillText('*', x * CW + CW / 2, y * CH + CH / 2 + 1);
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
            return `<tr${sel}><td>${i === invMenu.sel ? '▶' : ''}</td><td>${name}</td><td>×${count}</td><td>${use}</td><td>${canThrow ? '[E]' : ''}</td></tr>`;
        });
        ui.menu.querySelector('tbody').innerHTML = rows.join('');
    }

    window.Game = { setup, handle };
})();
