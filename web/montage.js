// An action-show opening-credits parody: quick ASCII-art cuts of everyday
// Russia, each with a "starring" caption, cut to a synth theme.
(function () {
    // every scene gets 1.5 s on top of its action for reading the caption
    const READ_S = 1.5;
    const SCENE_S = 2.4 + READ_S;

    const ease = (k) => (k <= 0 ? 0 : k >= 1 ? 1 : k * k * (3 - 2 * k));
    // a camera over a stage measured in character cells (zoom 1 = the normal scene size);
    // call it first in render(), while the montage's own font is still set
    function lens(ctx, W, H, fs, zoom, camX, camY) {
        const cw = ctx.measureText('M').width * zoom, ch = fs * 1.2 * zoom;
        const sx = (wx) => W / 2 + (wx - camX) * cw;
        const sy = (wy) => H * 0.42 + (wy - camY) * ch;
        const font = (scale = 1) => { ctx.font = `${Math.round(fs * zoom * scale)}px ui-monospace, "Cascadia Code", Menlo, monospace`; };
        const text = (str, wx, wy, color) => { ctx.fillStyle = color; ctx.fillText(str, sx(wx), sy(wy)); };
        font();
        return { cw, ch, sx, sy, font, text };
    }

    // the game's whole cast gets a short card; the bigger roles get their own camera work
    const CARD_S = 1.8 + READ_S;
    // each scene: ASCII art, its colour, an animated layer and the caption
    const scenes = [
        {
            who: 'PÄÄOSASSA', name: 'PULTSARI (SINÄ)', line: 'Kerää tyhjiä pulloja. Matkalla Vovan viinikellariin.',
            color: '#c8c8a0',
            dur: CARD_S,
            art: [
                '     ___',
                '    (x_x)',
                '    /|#|\\_[]',
                '     / \\',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#f2d34a';
                if (Math.floor(t * 3) % 2 === 0) ctx.fillText('*hic*', x + cw * 11, y + ch * (0.6 - (t % 0.66)));
            },
        },
        {
            who: 'PÄÄOSISSA', name: 'VANJA, 19', line: 'Värvätty.',
            color: '#9fb36a',
            dur: 6.8 + READ_S,
            // close on Vanja's face with nobody else in the shot; pull back to show only the
            // recruiting officer, the form and Vanja: he writes his name on it. Pull back again:
            // two men stand right beside him and grab him under the arms. Pull back once more:
            // the truck comes into view, they drag him to it and throw him in the back
            render(ctx, W, H, t, fs) {
                // each shot frames a stretch of the stage: [first column, last column]
                const cw1 = ctx.measureText('M').width;
                const shot = ([a, b]) => ({ zoom: W / (cw1 * (b - a)), x: (a + b) / 2 });
                const shots = [
                    { zoom: 3.6, x: 26.5, y: -1 },             // Vanja's face
                    { ...shot([-1.5, 30.5]), y: -0.4 },        // officer, form, Vanja
                    { ...shot([-2, 44]), y: -0.4 },            // and the two men beside him
                    { ...shot([-4, 68]), y: -0.4 },            // and the truck
                ];
                const cuts = [[0.9, 0.6], [2.9, 0.4], [3.9, 0.6]];   // [start, length] of each pull-back
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#e6e0d0', green = '#9fb36a', officer = '#7e8c6a', suit = '#6a7080';

                // the recruiting officer behind his desk, the form on it
                text(' _▄_ ', 1, -2, officer);
                text('(ò_ó)', 1, -1, skin);
                text('/|█|\\', 1, 0, officer);
                text('______________', 6, 0, '#8f7c50');
                text('|            |', 6, 1, '#8f7c50');
                text('|            |', 6, 2, '#8f7c50');
                text('[NIMI:_____]', 7, -0.35, '#eee');
                if (t > 1.5 && t < 2.7) text('"Nimi tähän."', -1, -3.4, '#ddd');

                // he writes his name, one letter at a time
                const written = Math.max(0, Math.min(5, (t - 1.85) / 0.18));
                if (t > 1.85) text('VANJA'.slice(0, Math.ceil(written)), 13, -0.35, '#7fa8ff');

                // the grab, the drag to the truck and the throw
                const rush = ease((t - 3.3) / 0.4);
                const drag = 13 * ease((t - 3.85) / 1.0);
                const vx = 24 + drag;
                const thrown = (t - 4.85) / 0.4;
                const truckX = 44 + 40 * Math.pow(Math.max(0, (t - 5.7) / 1.1), 2);

                const vanja = (x, y, angle, legs) => {
                    ctx.save();
                    ctx.translate(cam.sx(x + 2.5), cam.sy(y));
                    ctx.rotate(angle);
                    const rows = [[' ___ ', '#6b5a3a'], ['(o_o)', skin], ['/|=|\\', green], [' |_| ', green], [legs, green]];
                    rows.forEach(([s, c], i) => { ctx.fillStyle = c; ctx.fillText(s, -2.5 * cam.cw, (i - 2) * cam.ch); });
                    // the pen never makes it back to the desk
                    if (t >= 3.3) { ctx.fillStyle = '#7fa8ff'; ctx.fillText('✎', -3.4 * cam.cw, 0); }
                    ctx.restore();
                };
                if (thrown < 0) {
                    vanja(vx, 0, 0, t > 3.85 ? ' / / ' : ' / \\ ');
                    if (t > 1.85 && t < 3.3) {
                        // his arm across the desk to the pen on the form
                        const penX = 13 + written;
                        ctx.strokeStyle = green;
                        ctx.lineWidth = Math.max(2, cam.cw * 0.12);
                        ctx.beginPath();
                        ctx.moveTo(cam.sx(24.3), cam.sy(-0.1));
                        ctx.lineTo(cam.sx(penX), cam.sy(-0.45));
                        ctx.stroke();
                        text('✎', penX - 0.1, -0.95, '#7fa8ff');
                    }
                    if (t > 3.85) text('~~', vx - 1.5, 2.4, '#666');
                } else if (thrown < 1) {
                    // up and over, into the back of the truck
                    vanja(37 + 10 * thrown, -0.5 - 3.4 * 4 * thrown * (1 - thrown), thrown * 7, ' / / ');
                }
                if (t > 3.7 && t < 4.15) text('*NAPS!*', vx - 1, -3.3, '#f2d34a');

                // two men stand just out of the first shots, right beside him; the second pull-back
                // shows them, they take him under the arms, and in the end jump in after him
                const goon = (x, moving) => {
                    text('(▀_▀)', x, -1, skin);
                    text('/|█|\\', x, 0, suit);
                    text(' |_| ', x, 1, suit);
                    text(moving && Math.floor(t * 10) % 2 ? ' |\\  ' : ' / \\ ', x, 2, suit);
                };
                const hop = ease((t - 5.3) / 0.35);
                [[31, 19.5], [37.5, 29]].forEach(([from, to], i) => {
                    if (t < 2.9 || t > 5.65 + i * 0.05) return;
                    let x = from + (to - from) * rush + drag;
                    if (t > 5.3) x += (45 + i * 2 - x) * hop;
                    goon(x, (t > 3.3 && t < 3.7) || (t > 3.85 && t < 4.85) || t > 5.3);
                });
                if (t > 5.25 && t < 5.8) text('*TÖMPS*', truckX + 3, -3.2, '#f2d34a');

                // the truck: its cargo box hides whatever goes in the back
                ctx.fillStyle = '#000';
                ctx.fillRect(cam.sx(truckX), cam.sy(-2), cam.sx(truckX + 16) - cam.sx(truckX), cam.sy(1.5) - cam.sy(-2));
                text(' ______________', truckX, -2, '#5b6b3a');
                text('|              |___', truckX, -1, '#5b6b3a');
                text('|  ВОЕНКОМАТ   |  []\\', truckX, 0, '#5b6b3a');
                text('|______________|_____|', truckX, 1, '#5b6b3a');
                text('  (O)     (O)     (O)', truckX, 2, '#999');
                if (t > 5.6) {
                    for (let i = 0; i < 4; i++) {
                        const k = (t * 2 + i * 0.25) % 1;
                        text(k < 0.5 ? 'o' : 'O', truckX - 1 - k * 6, 1.6 - k * 0.8, '#777');
                    }
                }
            },
        },
        {
            who: 'SEKÄ', name: 'BABUŠKA, 84', line: 'Toivoo saavansa säkin perunoita – ja onnistuu!',
            color: '#d07a9a',
            dur: 7.2 + READ_S,
            // she stares into the camera; pull back: a man brings her a sack of potatoes for her
            // service, she watches him go, then turns back to stare into the camera again
            render(ctx, W, H, t, fs) {
                const wide = ease((t - 0.9) / 0.6) * (1 - ease((t - 5.7) / 0.6));
                const cam = lens(ctx, W, H, fs, 3.4 - 2.4 * wide, 4 + 13 * wide, 0.3 * wide);
                const { text } = cam;
                const pink = '#d07a9a', skin = '#e6e0d0', coat = '#7e8c6a';

                // the man: walks in from the right, puts the sack down, says his piece, walks off
                const manX = t < 5.3 ? 58 - 43 * Math.min(1, Math.max(0, (t - 1.6) / 1.3)) : 15 + 45 * ((t - 5.3) / 0.9);
                const walking = (t > 1.6 && t < 2.9) || t > 5.3;
                const watching = t > 1.9 && t < 5.7;

                text(' ,/  \\. ', 0, -1, pink);
                text(watching ? '(   o o)' : '(  o o )', 0, 0, skin);
                text(' \\_~~_/ ', 0, 1, pink);
                text('/|####|\\', 0, 2, pink);
                text(' /____\\ ', 0, 3, pink);

                if (manX < 60) {
                    text(' _▄_ ', manX, -1, coat);
                    text('(-_-)', manX, 0, skin);
                    text('/|█|\\', manX, 1, coat);
                    text(' |_| ', manX, 2, coat);
                    text(walking && Math.floor(t * 8) % 2 ? ' |\\  ' : ' / \\ ', manX, 3, coat);
                }
                // the sack: carried in, then set down at her feet
                const drop = Math.min(1, Math.max(0, (t - 2.9) / 0.15));
                if (t < 2.9) text('[3 KG]', manX + 5, 1.5, '#c9a96a');
                else text('[3 KG]', 8.8, 1.5 + 1.5 * drop, '#c9a96a');

                if (t > 3.1 && t < 5.3) {
                    cam.font(0.8);
                    text('"Tässä on 3 kg perunoita."', -1, -3.4, '#ddd');
                    if (t > 3.9) text('"Kiitos palveluksestanne Äiti-Venäjälle."', -1, -2.4, '#ddd');
                }
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
            who: 'SEKÄ', name: 'JUOPPO', line: '"Aitoa, brat!" Juo mitä tahansa.',
            color: '#c9b37a',
            dur: CARD_S,
            art: [
                '    (^o^)   "Aitoa, brat!"',
                '    \\|=|/',
                '     / \\    [ВОДКА]',
            ],
        },
        {
            who: 'VIERAILEVA TÄHTI', name: 'MILIISI', line: 'Ottaa vastaan vain käteistä.',
            color: '#6d8bff',
            art: [
                '    [МИЛИЦИЯ]',
                '     (-_-)   _____',
                '     /|█|\\  |₽ ₽ ₽|',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'GOPNIK', line: 'Kyykkii ja uhkailee. Verkkareissa.',
            color: '#d8d8d8',
            dur: CARD_S,
            art: [
                '     (¬_¬)   семечки',
                '   _/|≡|\\_',
                '    _/ \\_',
            ],
        },
        {
            who: 'SEKÄ', name: 'VARAS', line: '"Hipelöin sua!!" Ja lompakkoasi.',
            color: '#b08cd0',
            dur: CARD_S,
            art: [
                '     (ò_ô)',
                '    -/|▓|\\-->[₽]',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'RYÖSTÄJÄ', line: '"RAHAT TAI HENKI!" Mieluiten molemmat.',
            color: '#d06a4a',
            dur: CARD_S,
            art: [
                '     [▀▀▀]',
                '     (•_•)',
                '     /|█|\\=¬',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'DIILERI', line: '"Tässä on hyvää kamaa!" Ei ole.',
            color: '#7fd0a0',
            dur: CARD_S,
            art: [
                '     (¬_¬)   "Haluatko ostaa jotain?"',
                '     /|▒|\\[]',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'SKINHEAD', line: '"TAPAN SINUT!!" Sanoo sen kaikille.',
            color: '#c06060',
            dur: CARD_S,
            art: [
                '     (Ò_Ó)',
                '     /|█|\\',
                '     ▐▌ ▐▌',
            ],
        },
        {
            who: 'SEKÄ', name: 'PAKOVANKI', line: 'Karkasi SIZOsta. Kiekuralenkki tekee vahvaksi.',
            color: '#c8c8c8',
            dur: CARD_S,
            art: [
                '     (o_O)',
                '    /|≡≡|\\',
                '     / \\__O',
            ],
        },
        {
            who: 'SEKÄ', name: 'YEVGENI', line: 'Kysyy miliisiltä, saisiko puhaltaa.',
            color: '#a0c0e0',
            dur: CARD_S,
            art: [
                '     (o_o)   "Saanko puhaltaa?"',
                '     /|=|\\',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'KOTIÄITI', line: '"Vanjuska... missä sä..." *hik*',
            color: '#e0a0c0',
            dur: CARD_S,
            art: [
                '     ,~~~,',
                '     (=_=)',
                '     /|▼|\\  [ВОДКА]',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'LAPSI', line: '"Ei saa koskea! ÄITIII!"',
            color: '#f0d080',
            dur: CARD_S,
            art: [
                '     (o.o)',
                '     /|_|\\',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'DIMA, TAKSIKUSKI', line: 'Navigaattori näyttää Kiovaa. Taas.',
            color: '#f2d34a',
            dur: CARD_S,
            art: [
                '     [TAXI]',
                '     (-_-)  ~[≡]~',
                '     /|=|\\',
            ],
        },
        {
            who: 'SEKÄ', name: 'ROTTA', line: 'Puraisee nilkkaan.',
            color: '#a89080',
            dur: CARD_S,
            art: [
                '   ~~(__^·>',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#ddd';
                if (Math.floor(t * 4) % 2 === 0) ctx.fillText('*viik viik*', x + cw * 3, y - ch * 1.2);
            },
        },
        {
            who: 'SEKÄ', name: 'PANKKIRYÖSTÄJÄ', line: '"KAIKKI MAAHAN! *hik*"',
            color: '#c0a060',
            dur: CARD_S,
            art: [
                '     [▓▓▓]',
                '     (O_O)',
                '     /|█|\\==¬',
                '      / \\     [₽₽₽]',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#ffb040';
                if (Math.floor(t * 8) % 2 === 0) ctx.fillText('RÄKÄKÄKÄ!', x + cw * 13, y + ch * 2);
            },
        },
        {
            who: 'SEKÄ', name: 'VETERAANI', line: '"TANKIT!! DROONIT!!" Sokea, mutta kuulee askeleesi.',
            color: '#b0a080',
            dur: CARD_S,
            art: [
                '     (-_-)',
                '     /|*|\\',
                '      |  |',
            ],
        },
        {
            who: 'SEKÄ', name: 'SOTILAS', line: '"Rintamalle, svinya!" Tarvitsee miehiä. Sinut.',
            color: '#7e8c6a',
            dur: CARD_S,
            art: [
                '     [▄▄]',
                '     (ò_ó)',
                '     /|▓|\\=====',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'ILMATORJUNTAMIES', line: '"Kohde havaittu!" Sirpaleita sataa taivaalta.',
            color: '#8a9a6a',
            dur: CARD_S,
            art: [
                '          //',
                '      ___//',
                '     [===]',
                '     (ò_ó)',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#ffd24a';
                for (let i = 0; i < 3; i++) if (Math.floor(t * 5 + i) % 3 === 0) ctx.fillText('*', x + cw * (12 + i * 3), y - ch * (1 + i * 0.7));
            },
        },
        {
            who: 'SEKÄ', name: 'НПЗ · ÖLJYNJALOSTAMO', line: 'Palaa taas. Savu on "länsimaista propagandaa".',
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
            who: 'SEKÄ', name: 'FSB-AGENTTI', line: '"FSB valvoo." Kaikki on hallinnassa.',
            color: '#9a9aa8',
            dur: CARD_S,
            art: [
                '     ▄███▄',
                '     (▀_▀)',
                '     /|█|\\',
                '      / \\',
            ],
        },
        {
            who: 'SEKÄ', name: 'JOHTAJA, 61', line: 'Kaatui ikkunasta. Ihan itse.',
            color: '#cfcfcf',
            dur: 4.2 + READ_S,
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
            who: 'SEKÄ', name: 'VARTIJA', line: '"Stoi! Lattiaan!" Kremlin vartio.',
            color: '#c0c0e0',
            dur: CARD_S,
            art: [
                '     [ ▲ ]',
                '     (ò_ó)',
                '     /|█|\\',
                '      | |',
            ],
        },
        {
            who: 'SEKÄ', name: 'AVUSTAJA', line: 'Henkilökohtainen avustaja. Henkilökohtaisesti kännissä.',
            color: '#a0b0d0',
            dur: CARD_S,
            art: [
                '     (-_-)',
                '     /|=|\\_[■]',
                '      / \\',
            ],
            anim(ctx, t, x, y, cw, ch) {
                ctx.fillStyle = '#ddd';
                ctx.fillText('z'.repeat(1 + (Math.floor(t * 3) % 3)), x + cw * 6, y - ch * (0.6 + (t % 0.33)));
            },
        },
        {
            who: 'VIERAILEVA TÄHTI', name: 'MEDVEDEV', line: 'Kirjoittaa Telegramiin. ISOILLA KIRJAIMILLA.',
            color: '#e0e0e0',
            dur: CARD_S,
            art: [
                '     (ò_ó)   "Ydinase Lontooseen!"',
                '     /|█|\\  [TELEGRAM]',
                '      / \\',
            ],
        },
        {
            who: 'ERITYISESSÄ ROOLISSA', name: 'VOVA', line: 'Pitää kenraalinsa turvav\u00e4lin p\u00e4\u00e4ss\u00e4.',
            color: '#f2d34a',
            dur: 7.0 + READ_S,
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
            // the joke, shot like a film: close-ups, hard cuts, subtitles, letterbox
            who: '', name: '', line: '', color: '#ddd', dur: 7.2, noCredit: true,
            render(ctx, W, H, t, fs) {
                const faces = {
                    vova: { color: '#e6dccb', art: [
                        '    _________    ',
                        '   /         \\   ',
                        '  |  ò     ó  |  ',
                        '  |     ^     |  ',
                        '  |   _____   |  ',
                        '   \\_________/   ',
                        '     |=====|     ',
                    ] },
                    vovaSquint: { color: '#e6dccb', art: [
                        '    _________    ',
                        '   /         \\   ',
                        '  |  -     -  |  ',
                        '  |     ^     |  ',
                        '  |   _____   |  ',
                        '   \\_________/   ',
                        '     |=====|     ',
                    ] },
                    gerasimov: { color: '#b9c08f', art: [
                        '   _[==★==]_     ',
                        '  /         \\    ',
                        ' |  ò     ò  |   ',
                        ' |     o     |   ',
                        ' |   \\___/   |   ',
                        '  \\_________/    ',
                        '   |✱ ✱ ✱ ✱|     ',
                    ] },
                    shoigu: { color: '#b9c08f', art: [
                        '   _[==★==]_     ',
                        '  /         \\  , ',
                        ' |  °     °  | \' ',
                        ' |     -     |   ',
                        ' |    ~~~    |   ',
                        '  \\_________/    ',
                        '   |✱ ✱ ✱ ✱|     ',
                    ] },
                };
                const shots = [
                    { until: 1.8, face: 'vova',       who: 'VOVA',      say: 'Kuinka sota menee? Yhdellä sanalla.' },
                    { until: 2.9, face: 'gerasimov',  who: 'GERASIMOV', say: 'Hyvin.' },
                    { until: 4.4, face: 'vovaSquint', who: 'VOVA',      say: 'Entä kahdella sanalla?' },
                    { until: 5.7, face: 'shoigu',     who: 'ŠOIGU',     say: 'Ei hyvin.' },
                    { until: 99,  face: null,         who: '',          say: '' },
                ];
                let k = 0;
                while (t >= shots[k].until) k++;
                const shot = shots[k];
                const from = k ? shots[k - 1].until : 0;
                const lt = t - from;

                ctx.fillStyle = '#000';
                ctx.fillRect(0, 0, W, H);
                ctx.textAlign = 'center';
                if (shot.face) {
                    // a slow push-in on each close-up
                    const f = faces[shot.face];
                    const size = Math.round(fs * (2.1 + lt * 0.08));
                    ctx.font = `${size}px ui-monospace, Menlo, monospace`;
                    ctx.fillStyle = f.color;
                    f.art.forEach((line, i) => ctx.fillText(line, W / 2, H * 0.46 + (i - f.art.length / 2) * size * 1.05));
                    // Šoigu sweats
                    if (shot.face === 'shoigu') {
                        ctx.fillStyle = '#9fc8e8';
                        const dy = (lt * 1.4 % 1) * size * 3;
                        ctx.fillText('\'', W / 2 + size * 4.6, H * 0.46 - size * 1.6 + dy);
                    }
                } else {
                    // the wide shot: the table, the two ends, nobody says anything
                    ctx.font = `${Math.round(fs * 0.9)}px ui-monospace, Menlo, monospace`;
                    ctx.fillStyle = '#c8b27a';
                    ctx.fillText('o o ' + '_'.repeat(70) + ' o', W / 2, H * 0.48);
                    ctx.fillText('|' + ' '.repeat(74) + '|', W / 2, H * 0.48 + fs);
                    ctx.fillStyle = '#888';
                    if (lt > 0.4) ctx.fillText('. . .', W / 2, H * 0.38);
                }

                // letterbox
                const bar = H * 0.13;
                ctx.fillStyle = '#000';
                ctx.fillRect(0, 0, W, bar);
                ctx.fillRect(0, H - bar, W, bar);
                // subtitles in the bottom bar
                if (shot.say && lt > 0.15) {
                    ctx.font = `italic ${Math.round(fs * 0.95)}px ui-monospace, Menlo, monospace`;
                    ctx.fillStyle = '#f2d34a';
                    ctx.fillText(shot.who + ':', W / 2, H - bar * 0.68);
                    ctx.fillStyle = '#fff';
                    ctx.fillText(shot.say, W / 2, H - bar * 0.3);
                }
                // film grain and a vignette
                ctx.fillStyle = 'rgba(255,255,255,0.05)';
                for (let i = 0; i < 160; i++) ctx.fillRect(Math.random() * W, bar + Math.random() * (H - 2 * bar), 1.5, 1.5);
                const vg = ctx.createRadialGradient(W / 2, H / 2, H * 0.25, W / 2, H / 2, H * 0.75);
                vg.addColorStop(0, 'rgba(0,0,0,0)');
                vg.addColorStop(1, 'rgba(0,0,0,0.6)');
                ctx.fillStyle = vg;
                ctx.fillRect(0, bar, W, H - 2 * bar);
                ctx.textAlign = 'left';
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

        if (s.noCredit) {
            if (local < 0.12) {
                ctx.fillStyle = `rgba(255,255,255,${0.8 * (1 - local / 0.12)})`;
                ctx.fillRect(0, 0, W, H);
            }
            return true;
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
        ctx.fillText('GRAND TYR MOSCOW', W / 2, H * 0.4);
        ctx.font = `bold ${Math.round(fs * 0.6)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#7fbf1f';
        ctx.fillText('Russki Mir · Русский Мир', W / 2, H * 0.4 + fs * 0.95);
        ctx.font = `italic ${Math.round(fs * 0.32)}px ui-monospace, Menlo, monospace`;
        ctx.fillStyle = '#9a9a9a';
        ctx.fillText('"Venäjällä kaikki on paskaa, paitsi kusi."', W / 2, H * 0.4 + fs * 1.75);
        if (t < 0.15) {
            ctx.fillStyle = `rgba(255,255,255,${0.9 * (1 - t / 0.15)})`;
            ctx.fillRect(0, 0, W, H);
        }
    }

    load();
    window.Montage = { draw, duration, titleCard };
})();
