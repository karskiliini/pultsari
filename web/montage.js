// An action-show opening-credits parody: quick ASCII-art cuts of everyday
// Russia, each with a "starring" caption, cut to a synth theme.
(function () {
    const SCENE_S = 2.4;

    // each scene: ASCII art, its colour, an animated layer and the caption
    const scenes = [
        {
            who: 'PÄÄOSISSA', name: 'LADA', line: 'Käynnistyy ensimmäisellä yrityksellä. Kolmannellatoista.',
            color: '#d8d8d8',
            art: [
                '       _______',
                '   ___/ |  |  \\___',
                '  |  _   ----  _  |=',
                "  '-(_)-------(_)-'",
            ],
            anim(ctx, t, x, y, cw, ch) {
                // exhaust puffs drifting off behind the car
                ctx.fillStyle = '#777';
                for (let i = 0; i < 4; i++) {
                    const k = (t * 1.3 + i * 0.25) % 1;
                    ctx.fillText(k < 0.5 ? 'o' : 'O', x - cw * (2 + k * 8), y + ch * (2.2 - k * 1.5));
                }
            },
        },
        {
            who: 'PÄÄOSISSA', name: 'VASJA, 19', line: 'Värvätty eilen. Kivääri vuodelta 1953.',
            color: '#9fb36a',
            art: [
                '      ___',
                '     (o_o)   "Allekirjoita tähän."',
                '     /|=|\\--=====',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'BABUŠKA, 84', line: 'Pelasti perunat droonilta. Taas.',
            color: '#d07a9a',
            art: [
                '      __',
                '    ,/  \\.',
                '   (  o o )',
                '    \\_~~_/     [ПЕРУНА]',
                '   /|####|\\',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#bbb';
                ctx.fillText('--=[■]=--', x + cw * (30 - t * 6), y - ch * 0.4);
            },
        },
        {
            who: 'SEKÄ', name: 'BOMŽ', line: 'Korjaa Ladan teipillä ja vodkalla.',
            color: '#e2b46a',
            art: [
                '   [VODKA]   ~~~~~~ teippiä ~~~~~~',
                '     ________',
                '   _/  ____  \\___',
                '  |___[____]_____|',
                '     O        O',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#f2d34a';
                if (Math.floor(t * 6) % 2 === 0) ctx.fillText('*kilk*', x + cw * 20, y + ch * 5);
            },
        },
        {
            who: 'VIERAILEVA TÄHTI', name: 'POLITSIJA', line: 'Ottaa vastaan vain käteistä.',
            color: '#6d8bff',
            art: [
                '    [ПОЛИЦИЯ]',
                '     (-_-)   _____',
                '     /|█|\\  |₽ ₽ ₽|',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'НПЗ', line: 'Palaa taas. Savu on "länsimaista propagandaa".',
            color: '#c9a96a',
            art: [
                '     |█|      |█|',
                '   __|█|______|█|___',
                '  |   Н  П  З       |',
                '  |_________________|',
            ],
            anim(ctx, t, x, y, cw, ch) {
                for (let i = 0; i < 8; i++) {
                    const k = (t * 0.9 + i / 8) % 1;
                    ctx.fillStyle = k < 0.3 ? '#ff8030' : '#5a5a5a';
                    ctx.fillText(k < 0.3 ? '^' : '(  )', x + cw * (5 + (i % 2) * 8 + Math.sin(i + t) * 2), y - ch * (k * 4));
                }
            },
        },
        {
            who: 'SEKÄ', name: 'JOHTAJA, 61', line: 'Kaatui ikkunasta. Ihan itse.',
            color: '#cfcfcf',
            dur: 4.2,
            art: [
                '  _______________',
                ' |  ▒▒  ▒▒  ▒▒  |',
                ' |  ▒▒  ▒▒  ▒▒  |',
                ' |  ▒▒  ▒▒  ▒▒  |',
                ' |  ▒▒  ▒▒  ▒▒  |',
                ' |  ▒▒  ▒▒  ▒▒  |',
                ' |  ▒▒  ▒▒  ▒▒  |',
                ' |  ▒▒  ▒▒  ▒▒  |',
                '_|______________|______________________',
            ],
            anim(ctx, t, x, y, cw, ch) {
                // fixed pseudo-randomness, so the scene plays the same every time
                const rnd = (i) => { const v = Math.sin(i * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
                const winL = x + cw * 12, winC = x + cw * 13, winY = y + ch * 2;
                const groundY = y + ch * 7.75;
                const BREAK = 0.9;                       // the building stands intact until then
                const FALL = 0.8;                        // fast: he comes out hard
                const vx = cw * 13, vy0 = -ch * 1.2;     // out through the glass, a little upward first
                const g = 2 * (groundY - winY - vy0 * FALL) / (FALL * FALL);
                const landX = winC + vx * FALL;
                const tb = t - BREAK;

                const stickman = (px, py, angle, squash) => {
                    ctx.save();
                    ctx.translate(px, py);
                    ctx.rotate(angle);
                    ctx.scale(1, squash);
                    ctx.fillStyle = '#fff';
                    ctx.fillText('o', -cw * 0.5, -ch * 0.9);
                    ctx.fillText('/|\\', -cw * 1.5, 0);
                    ctx.fillText('/ \\', -cw * 1.5, ch * 0.9);
                    ctx.restore();
                };

                if (tb < 0) return; // the building, intact

                // the window is smashed: a dark hole with jagged glass left in the frame
                ctx.fillStyle = '#000';
                ctx.fillRect(winL - cw * 0.1, winY - ch * 0.5, cw * 2.2, ch);
                ctx.fillStyle = '#9fb7c9';
                ctx.fillText('\u25E5', winL - cw * 0.1, winY - ch * 0.3);
                ctx.fillText('\u25E3', winL + cw * 1.2, winY + ch * 0.3);

                // glass shards fly out with him and rain down, then lie glittering on the ground
                for (let i = 0; i < 26; i++) {
                    const svx = cw * (2 + rnd(i) * 9), svy = -ch * (rnd(i + 30) * 2.5);
                    const sg = g * (0.8 + rnd(i + 60) * 0.4);
                    let sx = winC + svx * tb, sy = winY + svy * tb + 0.5 * sg * tb * tb;
                    const landed = sy >= groundY;
                    if (landed) {
                        // where it came down: solve for the moment it reached the ground
                        const tl = (-svy + Math.sqrt(svy * svy + 2 * sg * (groundY - winY))) / sg;
                        sx = winC + svx * tl; sy = groundY;
                    }
                    ctx.save();
                    ctx.translate(sx, sy);
                    ctx.rotate(landed ? rnd(i + 5) * 3 : tb * (4 + rnd(i + 7) * 10));
                    ctx.fillStyle = (Math.floor(t * 12 + i) % 3 === 0) ? '#e8f4ff' : '#8fb0c8';
                    ctx.fillRect(-2, -1, 3 + rnd(i + 11) * 3, 2);
                    ctx.restore();
                }

                if (tb < FALL) {
                    // the fall: fast, tumbling
                    stickman(winC + vx * tb, winY + vy0 * tb + 0.5 * g * tb * tb, tb * 11, 1);
                } else {
                    // the landing: no bounce, no pieces - the body just folds into a broken heap
                    const tl = tb - FALL;
                    const k = Math.min(1, tl / 0.12);
                    // blood spreads out along the ground from under him
                    const r = Math.min(1, tl / 1.2);
                    ctx.fillStyle = 'rgba(120,8,8,0.92)';
                    ctx.beginPath();
                    ctx.ellipse(landX, groundY + ch * 0.05, cw * (1.2 + 5 * Math.sqrt(r)), ch * (0.12 + 0.22 * r), 0, 0, Math.PI * 2);
                    ctx.fill();
                    for (let i = 0; i < 14; i++) {
                        const dir = rnd(i + 200) < 0.5 ? -1 : 1;
                        const reach = cw * (1 + rnd(i + 210) * 6) * Math.min(1, tl / 0.35);
                        ctx.fillStyle = i % 2 ? '#8a0a0a' : '#5e0505';
                        ctx.fillRect(landX + dir * reach, groundY - 1 + rnd(i + 220) * 3, 2 + rnd(i + 230) * 4, 2);
                    }
                    // the heap: squashed flat, limbs at wrong angles
                    ctx.save();
                    ctx.translate(landX, groundY - ch * 0.15);
                    ctx.fillStyle = '#e8e8e8';
                    ctx.scale(1, 1 - 0.55 * k);
                    ctx.rotate(1.35);
                    ctx.fillText('o', -cw * 0.4, -ch * 0.6);
                    ctx.restore();
                    ctx.save();
                    ctx.translate(landX, groundY - ch * 0.1);
                    ctx.fillStyle = '#d8d8d8';
                    ctx.scale(1.25, 0.45 + 0.55 * (1 - k));
                    ctx.fillText('\\_/\u203E\\', -cw * 2, 0);
                    ctx.restore();
                    ctx.fillStyle = 'rgba(150,10,10,0.9)';
                    ctx.fillText('~', landX - cw * 0.5, groundY - ch * 0.3);
                    if (tl < 0.5) {
                        ctx.fillStyle = '#f2d34a';
                        ctx.fillText('*MÄTS*', landX - cw * 3, groundY - ch * 1.6);
                    }
                }

                // a man from the FSB looks down from the window for a moment, then is gone
                const fsbAt = FALL + 0.45, fsbFor = 1.1;
                if (tb > fsbAt && tb < fsbAt + fsbFor) {
                    const peek = Math.min(1, (tb - fsbAt) / 0.2) * Math.min(1, (fsbAt + fsbFor - tb) / 0.2);
                    ctx.globalAlpha = peek;
                    ctx.fillStyle = '#7e8c6a';
                    ctx.fillText('\u2580', winL + cw * 0.5, winY - ch * 0.45);  // hat brim
                    ctx.fillStyle = '#ddd';
                    ctx.fillText('o', winL + cw * 0.7, winY);
                    ctx.fillStyle = '#c33';
                    ctx.font = ctx.font.replace(/\d+px/, (m) => Math.round(parseInt(m, 10) * 0.55) + 'px');
                    ctx.fillText('ФСБ', winL - cw * 0.2, winY + ch * 0.55);
                    ctx.globalAlpha = 1;
                }
            },
        },
        {
            who: 'ERITYISESSÄ ROOLISSA', name: 'VOVA', line: 'Pitää kenraalinsa turvav\u00e4lin p\u00e4\u00e4ss\u00e4.',
            color: '#f2d34a',
            dur: 7.0,
            // camera work: close on Vova, a quick pull back, then a long pan left along the table
            // to the two generals at the far end
            render(ctx, W, H, t, fs) {
                const ease = (k) => (k <= 0 ? 0 : k >= 1 ? 1 : k * k * (3 - 2 * k));
                const L = 150;                                 // the table, in cells
                const cw0 = ctx.measureText('M').width, ch0 = fs * 1.2;
                const zoom = 3.4 - 2.4 * ease((t - 1.1) / 0.6);   // big on Vova, then pulled back fast
                const camX = (L + 5) - 13 * ease((t - 1.1) / 0.6) - (L + 4) * ease((t - 2.0) / 4.0);
                const camY = 1.6 + 0.9 * ease((t - 1.1) / 0.6);
                const cw = cw0 * zoom, ch = ch0 * zoom;
                const sx = (wx) => W / 2 + (wx - camX) * cw;
                const sy = (wy) => H * 0.42 + (wy - camY) * ch;
                ctx.font = `${Math.round(fs * zoom)}px ui-monospace, "Cascadia Code", Menlo, monospace`;
                const text = (str, wx, wy, color) => { ctx.fillStyle = color; ctx.fillText(str, sx(wx), sy(wy)); };

                // the table: a polished top and a front edge, legs at the ends and every so often
                const top = '_'.repeat(L);
                text(' ' + top, 0, 1, '#c8b27a');
                text('|' + top + '|', 0, 2, '#c8b27a');
                for (let lx = 1; lx < L - 8; lx += 37) text('||', lx, 3, '#8f7c50'), text('||', lx, 4, '#8f7c50');
                text('||', L - 2, 3, '#8f7c50'); text('||', L - 2, 4, '#8f7c50');
                // a shine running along the surface
                ctx.globalAlpha = 0.25;
                text('~'.repeat(L - 4), 2, 1.55, '#fff');
                ctx.globalAlpha = 1;

                // Vova in his chair at the near end: an old man, alone
                const vx = L + 2;
                text(' ___ ', vx, -1, '#e6e0d0');
                text('(ò_ó)', vx, 0, '#e6e0d0');
                text('/|=|\\', vx, 1, '#4b5d8a');
                text(' |_|_', vx, 2, '#4b5d8a');
                text(' |  |', vx, 3, '#6b5a3a');
                if (t < 1.1) text('*nyyh*', vx + 5.5, -0.6, '#f2d34a');

                // the caption lies on the table and drifts past as the camera goes by
                text('naurettavan pitkä pöytä', L * 0.3, 0.3, '#f2d34a');

                // at the far end: the two generals, in their own chairs, very far away
                const gx = -12;
                text('(o_o)  (ò_ò)', gx, 0, '#e0d8c0');
                text('/|█|\\  /|█|\\', gx, 1, '#5b6b3a');
                text(' |_|_   |_|_', gx, 2, '#5b6b3a');
                text(' |  |   |  |', gx, 3, '#6b5a3a');
                ctx.font = `bold ${Math.round(fs * zoom * 0.8)}px ui-monospace, Menlo, monospace`;
                text('ŠOIGU', gx - 0.5, -1.4, '#f2d34a');
                text('GERASIMOV', gx + 5.5, -1.4, '#f2d34a');
                if (t > 6.0) {
                    ctx.font = `${Math.round(fs * zoom * 0.7)}px ui-monospace, Menlo, monospace`;
                    text('"Kaikki menee hyvin, herra presidentti!"', gx - 6, -2.6, '#ddd');
                }
            },
        },
        {
            who: 'SEKÄ', name: 'VKUSNO I TOTŠKA', line: 'Ei ole sama. Jono on silti sama.',
            color: '#e05050',
            art: [
                '  [ VKUSNO I TOTŠKA ]',
                '   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓',
                '   ▓  o o o o o o o o o o o o',
            ],
        },
    ];

    // ---- ASCII video clips made with tools/video2ascii.py (web/video/index.json) ----
    const clips = [];
    function xterm256(n) {
        if (n >= 232) { const v = 8 + (n - 232) * 10; return `rgb(${v},${v},${v})`; }
        n -= 16;
        const lv = (c) => (c === 0 ? 0 : 55 + c * 40);
        return `rgb(${lv(Math.floor(n / 36))},${lv(Math.floor(n / 6) % 6)},${lv(n % 6)})`;
    }
    const PALETTE = Array.from({ length: 256 }, (_, i) => (i < 16 ? '#888' : xterm256(i)));
    function load() {
        return fetch('video/index.json', { cache: 'no-store' })
            .then((r) => (r.ok ? r.json() : []))
            .then((list) => Promise.all(list.map((name) =>
                fetch('video/' + name, { cache: 'no-store' }).then((r) => (r.ok ? r.json() : null)).catch(() => null))))
            .then((loaded) => {
                for (const clip of loaded) {
                    if (!clip || !clip.frames || !clip.frames.length) continue;
                    clip.colors = clip.frames.map((f) => Uint8Array.from(atob(f.k), (c) => c.charCodeAt(0)));
                    clip.seconds = clip.frames.length / clip.fps;
                    clips.push(clip);
                }
            })
            .catch(() => {});
    }
    const useClips = () => clips.length > 0;
    // scenes last SCENE_S unless they say otherwise
    const sceneLen = (sc) => sc.dur || SCENE_S;
    const scenesTotal = () => scenes.reduce((sum, sc) => sum + sceneLen(sc), 0);

    function duration() {
        return (useClips() ? clips.reduce((s, c) => s + c.seconds, 0) : scenesTotal()) + 3.5;
    }

    function drawClip(ctx, W, H, clip, local) {
        const frame = Math.min(clip.frames.length - 1, Math.floor(local * clip.fps));
        const chars = clip.frames[frame].c, colors = clip.colors[frame];
        const cw = W / clip.cols, ch = (H * 0.74) / clip.rows;
        ctx.font = `bold ${Math.max(6, Math.round(ch * 0.95))}px ui-monospace, Menlo, monospace`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        for (let y = 0; y < clip.rows; y++) {
            for (let x = 0; x < clip.cols; x++) {
                const i = y * clip.cols + x;
                const g = chars[i];
                if (g === ' ') continue;
                ctx.fillStyle = PALETTE[colors[i]];
                ctx.fillText(g, x * cw + cw / 2, y * ch + ch / 2);
            }
        }
    }

    function creditBar(ctx, W, H, local, who, name, line) {
        const fs = Math.max(12, Math.round(W / 46));
        const slide = Math.min(1, local / 0.35);
        const barY = H * 0.84;
        const x = -W + slide * (W + fs * 1.5);
        ctx.fillStyle = 'rgba(0,0,0,0.85)';
        ctx.fillRect(0, barY - fs * 1.4, W, fs * 3.4);
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        ctx.font = `bold ${Math.round(fs * 0.7)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#d52b1e';
        ctx.fillText(who, x, barY - fs * 0.6);
        ctx.font = `bold ${Math.round(fs * 1.35)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#f2d34a';
        ctx.fillText(name, x, barY + fs * 0.5);
        ctx.font = `${Math.round(fs * 0.75)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#ddd';
        ctx.fillText(line, x, barY + fs * 1.55);
    }

    // draws the montage at time t (seconds); returns false when it is over
    function draw(ctx, W, H, t) {
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, W, H);
        if (t >= duration()) return false;

        if (useClips()) {
            let start = 0;
            for (const clip of clips) {
                if (t < start + clip.seconds) {
                    const local = t - start;
                    drawClip(ctx, W, H, clip, local);
                    const c = clip.caption || {};
                    creditBar(ctx, W, H, local, c.who || '', c.name || '', c.line || '');
                    if (local < 0.12) {
                        ctx.fillStyle = `rgba(255,255,255,${0.8 * (1 - local / 0.12)})`;
                        ctx.fillRect(0, 0, W, H);
                    }
                    return true;
                }
                start += clip.seconds;
            }
            titleCard(ctx, W, H, t - start);
            return true;
        }

        let index = 0, startAt = 0;
        while (index < scenes.length && t >= startAt + sceneLen(scenes[index])) startAt += sceneLen(scenes[index++]);
        if (index >= scenes.length) {
            titleCard(ctx, W, H, t - startAt);
            return true;
        }
        const s = scenes[index];
        const local = t - startAt;

        // quick zoom-in cut and a white flash between scenes, like the eighties
        const zoom = 1.25 - Math.min(0.25, local * 0.6);
        const fs = Math.max(12, Math.round(W / 46));
        if (s.render) {
            // a scene with its own camera work
            ctx.save();
            ctx.font = `${fs}px ui-monospace, "Cascadia Code", Menlo, monospace`;
            ctx.textBaseline = 'middle';
            ctx.textAlign = 'left';
            s.render(ctx, W, H, local, fs);
            ctx.restore();
        } else {
        ctx.save();
        ctx.translate(W / 2, H * 0.42);
        ctx.scale(zoom, zoom);
        ctx.font = `${fs}px ui-monospace, "Cascadia Code", Menlo, monospace`;
        ctx.textBaseline = 'middle';
        ctx.textAlign = 'left';
        const cw = ctx.measureText('M').width, ch = fs * 1.2;
        const width = Math.max(...s.art.map((l) => l.length)) * cw;
        const x = -width / 2, y = -(s.art.length * ch) / 2;
        ctx.fillStyle = s.color;
        s.art.forEach((line, i) => ctx.fillText(line, x, y + i * ch));
        if (s.anim) s.anim(ctx, local, x, y, cw, ch);
        ctx.restore();
        }

        // the credit bar slides in from the left
        const slide = Math.min(1, local / 0.35);
        const barY = H * 0.78;
        ctx.fillStyle = 'rgba(0,0,0,0.85)';
        ctx.fillRect(0, barY - fs * 1.4, W, fs * 3.4);
        ctx.textAlign = 'left';
        ctx.font = `bold ${Math.round(fs * 0.7)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#d52b1e';
        ctx.fillText(s.who, -W + slide * (W + fs * 1.5), barY - fs * 0.6);
        ctx.font = `bold ${Math.round(fs * 1.35)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#f2d34a';
        ctx.fillText(s.name, -W + slide * (W + fs * 1.5), barY + fs * 0.5);
        ctx.font = `${Math.round(fs * 0.75)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#ddd';
        ctx.fillText(s.line, -W + slide * (W + fs * 1.5), barY + fs * 1.55);

        if (local < 0.12) {
            ctx.fillStyle = `rgba(255,255,255,${0.8 * (1 - local / 0.12)})`;
            ctx.fillRect(0, 0, W, H);
        }
        return true;
    }

    function titleCard(ctx, W, H, t) {
        const fs = Math.max(22, Math.round(W / 18));
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.font = `bold ${fs}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#fff';
        ctx.fillText('P\u041FLTSA\u042FI 2', W / 2, H * 0.4);
        ctx.font = `bold ${Math.round(fs * 0.6)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#7fbf1f';
        ctx.fillText('Russki Mir · Русский Мир', W / 2, H * 0.4 + fs * 0.95);
        if (t < 0.15) {
            ctx.fillStyle = `rgba(255,255,255,${0.9 * (1 - t / 0.15)})`;
            ctx.fillRect(0, 0, W, H);
        }
    }

    load();
    window.Montage = { draw, duration, titleCard };
})();
