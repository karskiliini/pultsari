// Grand Tyr Moscow browser client: draws the game's JSON frames as ASCII on a canvas
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
    const shells = [];   // anti-aircraft tracers: {x0, y0, x1, y1, t}
    const MAX_STAINS = 4000;
    let lastTime = performance.now();
    let mood = { b: 1, s: 1 };
    let cityMap = null;
    let mapOpen = false;

    // overview of the whole city: streets, buildings and a blinking "you are here"
    const BUILDING_COLORS = { 8: '#7a1a1a', 9: '#3d3a2a', 10: '#2b3550', 11: '#4a3a10', 12: '#24402c' };
    function drawCityMap(now) {
        const m = cityMap;
        const pad = 24, sx = (W - pad * 2) / m.w, sy = (H - pad * 2 - 18) / m.h;
        const X = (x) => pad + x * sx, Y = (y) => pad + 18 + y * sy;
        ctx.fillStyle = '#080808';
        ctx.fillRect(0, 0, W, H);
        ctx.strokeStyle = '#d52b1e';
        ctx.strokeRect(pad - 4, pad + 14, W - pad * 2 + 8, H - pad * 2 - 10);
        ctx.font = 'bold 13px ui-monospace, Menlo, monospace';
        ctx.textAlign = 'left';
        ctx.fillStyle = '#d52b1e';
        ctx.fillText('МОСКВА · MOSKOVA', pad, pad + 4);
        ctx.fillStyle = '#f2d34a';
        ctx.textAlign = 'right';
        ctx.fillText(frame && frame.lvl ? frame.lvl : '', W - pad, pad + 4);
        // streets
        ctx.fillStyle = '#3a3a3a';
        for (const [hor, pos] of m.r) {
            if (hor) ctx.fillRect(X(0), Y(pos), m.w * sx, 3 * sy);
            else ctx.fillRect(X(pos), Y(0), 3 * sx, m.h * sy);
        }
        ctx.font = '9px ui-monospace, Menlo, monospace';
        ctx.fillStyle = '#8a8a8a';
        for (const [hor, pos, name] of m.r) {
            if (hor) { ctx.textAlign = 'left'; ctx.fillText(name, X(2), Y(pos) - 3); }
            else { ctx.save(); ctx.translate(X(pos) - 3, Y(4)); ctx.rotate(Math.PI / 2); ctx.textAlign = 'left'; ctx.fillText(name, 0, 0); ctx.restore(); }
        }
        // buildings
        for (const [x0, y0, x1, y1, name, type] of m.b) {
            ctx.fillStyle = BUILDING_COLORS[type] || '#4a4a4a';
            ctx.fillRect(X(x0), Y(y0), (x1 - x0 + 1) * sx, (y1 - y0 + 1) * sy);
            if (name) {
                ctx.fillStyle = '#ddd';
                ctx.textAlign = 'center';
                ctx.fillText(name, X((x0 + x1) / 2), Y((y0 + y1) / 2) + 3);
            }
        }
        // you are here
        if (frame && frame.pos && Math.floor(now / 350) % 2 === 0) {
            const px = X(frame.pos[0]), py = Y(frame.pos[1]);
            ctx.fillStyle = '#ff3030';
            ctx.beginPath(); ctx.arc(px, py, 4, 0, Math.PI * 2); ctx.fill();
            ctx.font = 'bold 11px ui-monospace, Menlo, monospace';
            ctx.textAlign = 'left';
            ctx.fillText('SINÄ', px + 7, py + 4);
        }
        ctx.textAlign = 'center';
        ctx.fillStyle = '#777';
        ctx.font = '11px ui-monospace, Menlo, monospace';
        ctx.fillText('Tab sulkee kartan', W / 2, H - 8);
    }

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

    // the view always fits the browser window: scale the whole grid, keep its shape
    function fit() {
        const dpr = window.devicePixelRatio || 1;
        const frameEl = canvas.closest('#frame') || canvas.parentElement;
        const side = document.getElementById('side');
        const sideW = side && side.offsetParent ? side.offsetWidth + 10 : 0;
        const availW = Math.max(200, document.documentElement.clientWidth - 32 - sideW - 4);
        const top = frameEl.getBoundingClientRect().top + window.scrollY;
        const below = 64; // HUD and sound line under the map
        const availH = Math.max(150, window.innerHeight - top - below - 4);
        const scale = Math.max(0.35, Math.min(3, availW / W, availH / H));
        canvas.width = Math.round(W * scale * dpr);
        canvas.height = Math.round(H * scale * dpr);
        canvas.style.width = Math.round(W * scale) + 'px';
        canvas.style.height = Math.round(H * scale) + 'px';
        ctx.setTransform(dpr * scale, 0, 0, dpr * scale, 0, 0);
        const main = document.getElementById('main');
        if (main) main.style.width = Math.round(W * scale) + 4 + 'px';
    }

    function setup(els) {
        ui = els;
        canvas = els.canvas;
        ctx = canvas.getContext('2d');
        fit();
        window.addEventListener('resize', fit);
        requestAnimationFrame(render);
    }

    // world cell -> screen pixel (cell centre)
    // is a world cell in the player's sight? (no mask sent means everything is visible)
    function seen(wx, wy) {
        if (!frame || !frame.vis) return true;
        const x = Math.floor(wx + 0.5) - frame.cam[0], y = Math.floor(wy + 0.5) - frame.cam[1];
        const row = frame.vis[y];
        if (!row || x < 0) return false;
        const digit = parseInt(row[x >> 2], 16);
        return !isNaN(digit) && (digit >> (x & 3) & 1) === 1;
    }

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
    // vomit: a chunky green-yellow spray, shorter and wetter than blood
    const VOMIT = ['#6f8f1a', '#8aa828', '#9fb83a', '#5e7a14', '#b5c24a', '#7d6a1e'];
    function vomit(m) {
        const bx = m.dx || 0, by = m.dy || 0;
        for (let i = 0; i < 46; i++) {
            let a = Math.random() * Math.PI * 2;
            if (bx || by) a = Math.atan2(by, bx) + (Math.random() - 0.5) * 1.1;
            const sp = 30 + Math.random() * (bx || by ? 190 : 110);
            particles.push({
                wx: m.x + 0.5, wy: m.y + 0.5,
                ox: 0, oy: 0, vx: Math.cos(a) * sp, vy: Math.sin(a) * sp,
                life: 0.2 + Math.random() * 0.4, color: VOMIT[(Math.random() * VOMIT.length) | 0],
                size: Math.random() < 0.35 ? 3 : 2, stain: true,
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
                if (ui.deathComfort) ui.deathComfort.textContent = m.comfort || '';
                ui.deathScore.textContent = `Hakatut babuškat ${m.mummot} · Kaikki hakatut ${m.kaikki}`;
                break;
            case 'restart': if (ui.onRestart) ui.onRestart(); break;
            case 'invmenu': invMenu = m; invMenuAt = performance.now(); break;
            case 'map': cityMap = m; break;
            case 'aim': aim = m; aimAt = performance.now(); break;
            case 'snd': if (window.SFX) SFX.play(m.n, m.v); break;
            case 'hit': blood(m); break;
            case 'vomit': vomit(m); break;
            case 'boom': boom(m); break;
            case 'shell':
                shells.push({ x0: m.x0, y0: m.y0, x1: m.x1, y1: m.y1, t: 0 });
                flashes.push({ wx: m.x0 + 0.5, wy: m.y0 + 0.5, r: CW * 1.6, t: 0 }); // muzzle flash
                break;
            case 'clear': ui.text.textContent = ''; ui.text.hidden = false; break;
            case 'text': ui.text.hidden = false; ui.text.textContent += m.s; break;
            case 'score':
                ui.text.hidden = false;
                ui.text.textContent += `\n\nПРОТОКОЛ · PÖYTÄKIRJA\n\nHakatut babuškat   ${m.mummot}\nKaikki hakatut   ${m.kaikki}\nPISTEET          ${m.total}\n\nВсё идёт по плану. (Kaikki menee suunnitelman mukaan.)\nKiitos pelaamisesta.`;
                break;
        }
    }

    function updateUi(m) {
        ui.msg.textContent = m.msg ? '» ' + m.msg : '';
        ui.level.textContent = m.lvl || '';
        if (m.hud) {
            const [money, hp, prom, belly, kop, sick] = m.hud;
            ui.money.textContent = (kop ? money + ',' + String(kop).padStart(2, '0') : money) + ' ₽';
            ui.hp.textContent = hp;
            ui.hp.className = hp < 5 ? 'bad' : '';
            ui.prom.textContent = (prom / 10).toFixed(1) + ' ‰';
            ui.prom.className = prom < 10 ? 'good' : prom < 30 ? 'warn' : 'bad';
            ui.belly.textContent = belly + '%';
            ui.belly.className = belly <= 20 ? 'bad' : belly <= 50 ? 'warn' : '';
            // HIV stage once symptoms show (1-4), AIDS at 5; nothing while the player does not know
            if (ui.sickBox) {
                ui.sickBox.hidden = !sick;
                if (sick) ui.sick.textContent = sick >= 5 ? 'AIDS' : 'HIV+ ' + '●'.repeat(sick) + '○'.repeat(4 - sick);
            }
        }
        if (ui.drv) {
            ui.drv.hidden = !m.drv;
            if (m.drv) {
                const [gear, rpm, kmh, wheel, clutch, engine, grind, bearing] = m.drv;
                ui.gear.textContent = gear;
                const bars = Math.min(8, Math.round(rpm / 825));
                ui.rpm.innerHTML = '';
                for (let k = 0; k < 8; k++) {
                    const b = document.createElement('span');
                    b.textContent = k < bars ? '▮' : '▯';
                    b.style.color = k >= bars ? '#555' : k >= 7 ? '#ff4d4d' : k >= 5 ? '#f2d34a' : '#5ee05e';
                    ui.rpm.appendChild(b);
                }
                ui.kmh.textContent = kmh + ' km/h ' + String(bearing).padStart(3, '0') + '°';
                const pos = Math.round(wheel / 100 * 3) + 3;
                ui.wheel.textContent = '◄' + [0, 1, 2, 3, 4, 5, 6].map((k) => k === pos ? '╪' : k === 3 ? '·' : '─').join('') + '►';
                ui.drvflags.textContent = [clutch ? 'KYTKIN' : '', engine ? '' : 'SAMMUNUT', grind ? 'KRRRK' : ''].filter(Boolean).join(' ');
            }
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
            // the map is printed paper: no hangover gloom on it
            // late HIV: the eyes blur and the colours wash out
            const sick = frame.hud[5] || 0;
            const haze = sick >= 3 ? ` blur(${((sick - 2) * 0.25).toFixed(2)}px) contrast(${(1 - (sick - 2) * 0.08).toFixed(2)})` : '';
            canvas.style.filter = mapOpen ? 'none'
                : `brightness(${mood.b.toFixed(3)}) saturate(${mood.s.toFixed(3)}) hue-rotate(${swim.toFixed(1)}deg)${haze}`;
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
                if (!seen(s.wx - 0.5 + s.ox / CW, s.wy - 0.5 + s.oy / CH)) continue;
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

            // speech bubbles: 70 % opaque when said, fading out fast so whoever is behind shows through
            if (frame.bub) {
                ctx.font = cw === CW ? FONT_BOLD : 'bold ' + font;
                for (const [bx, by, text, left] of frame.bub) {
                    const k = Math.max(0, Math.min(1, left / 100));
                    const alpha = 0.7 * k * k;
                    if (alpha < 0.02) continue;
                    ctx.globalAlpha = alpha;
                    let x = bx;
                    for (const g of text) {
                        if (g !== ' ') {
                            ctx.fillStyle = '#000';
                            ctx.fillText(g, x * cw + cw / 2 + 1, by * ch + ch / 2 + 2);
                            ctx.fillStyle = '#fff';
                            ctx.fillText(g, x * cw + cw / 2, by * ch + ch / 2 + 1);
                        }
                        x++;
                    }
                }
                ctx.globalAlpha = 1;
            }
        }

        // flying droplets and debris; blood that lands becomes a stain
        for (let i = particles.length - 1; i >= 0; i--) {
            const p = particles[i];
            p.ox += p.vx * dt; p.oy += p.vy * dt;
            p.vx *= Math.pow(0.04, dt); p.vy *= Math.pow(0.04, dt);
            p.life -= dt;
            const [sx, sy] = toScreen(p.wx - 0.5, p.wy - 0.5);
            ctx.fillStyle = seen(p.wx - 0.5 + p.ox / CW, p.wy - 0.5 + p.oy / CH) ? p.color : 'rgba(0,0,0,0)';
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

        // anti-aircraft tracers: a hot streak racing from the barrels to the burst
        for (let i = shells.length - 1; i >= 0; i--) {
            const s = shells[i];
            s.t += dt;
            if (s.t > 0.3) { shells.splice(i, 1); continue; }
            const [ax, ay] = toScreen(s.x0, s.y0), [bx, by] = toScreen(s.x1, s.y1);
            const head = Math.min(1, s.t / 0.1), tail = Math.max(0, head - 0.45);
            const fade = s.t < 0.1 ? 1 : 1 - (s.t - 0.1) / 0.2;
            const lx = (k) => ax + (bx - ax) * k, ly = (k) => ay + (by - ay) * k;
            ctx.lineCap = 'round';
            ctx.strokeStyle = `rgba(255,170,40,${0.35 * fade})`;
            ctx.lineWidth = 5 * scale;
            ctx.beginPath(); ctx.moveTo(lx(tail), ly(tail)); ctx.lineTo(lx(head), ly(head)); ctx.stroke();
            ctx.strokeStyle = `rgba(255,245,190,${fade})`;
            ctx.lineWidth = 1.5 * scale;
            ctx.beginPath(); ctx.moveTo(lx(tail), ly(tail)); ctx.lineTo(lx(head), ly(head)); ctx.stroke();
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
            if (aim.gun !== undefined) {
                // the anti-aircraft gunner's shells left
                const w = 260, h = 12, x0 = (COLS * CW - w) / 2, y0 = 10, MAX = 100;
                ctx.fillStyle = 'rgba(0,0,0,0.75)';
                ctx.fillRect(x0 - 8, y0 - 6, w + 16, h + 26);
                ctx.strokeStyle = '#888';
                ctx.strokeRect(x0, y0 + 14, w, h);
                const r = Math.max(0, Math.min(1, aim.gun / MAX));
                ctx.fillStyle = aim.gun > 30 ? '#8fbf3a' : aim.gun > 0 ? '#f2d34a' : '#ff4d4d';
                for (let k = 0; k < MAX; k++) {
                    if (k / MAX >= r) break;
                    ctx.fillRect(x0 + 1 + k * (w - 2) / MAX, y0 + 15, Math.max(1, (w - 2) / MAX - 1), h - 2);
                }
                ctx.fillStyle = '#eee';
                ctx.font = 'bold 12px ui-monospace, Menlo, monospace';
                ctx.textAlign = 'left';
                ctx.fillText('IT-TYKKI · ЗУ-23-2', x0, y0 + 5);
                ctx.textAlign = 'right';
                ctx.fillText(aim.gun ? `AMMUKSET ${aim.gun}/${MAX}` : 'AMMUKSET LOPPU', x0 + w, y0 + 5);
                ctx.textAlign = 'center';
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
        if (mapOpen && cityMap) drawCityMap(now);
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

    window.Game = { setup, handle, toggleMap: () => { mapOpen = !mapOpen; return mapOpen; }, isDriving: () => !!(frame && frame.drv) };
})();
