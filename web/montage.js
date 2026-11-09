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
            art: [
                '   |▒▒|▒▒|',
                '   |▒▒|  |',
                '   |▒▒|▒▒|',
                '   |▒▒|▒▒|',
                '   |▒▒|▒▒|',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#fff';
                const fall = Math.min(1, t / 1.6);
                ctx.fillText('\\o/', x + cw * 8, y + ch * (1 + fall * 5));
            },
        },
        {
            who: 'ERITYISESSÄ ROOLISSA', name: 'PRESIDENTTI', line: 'Neuvottelee nukkejen kanssa.',
            color: '#f2d34a',
            art: [
                ' P ═════════════════════════════  z  o',
                '      (naurettavan pitkä pöytä)',
            ],
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
    function duration() {
        return (useClips() ? clips.reduce((s, c) => s + c.seconds, 0) : scenes.length * SCENE_S) + 3.5;
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

        const index = Math.floor(t / SCENE_S);
        if (index >= scenes.length) {
            titleCard(ctx, W, H, t - scenes.length * SCENE_S);
            return true;
        }
        const s = scenes[index];
        const local = t - index * SCENE_S;

        // quick zoom-in cut and a white flash between scenes, like the eighties
        const zoom = 1.25 - Math.min(0.25, local * 0.6);
        const fs = Math.max(12, Math.round(W / 46));
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
