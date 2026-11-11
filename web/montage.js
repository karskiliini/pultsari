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

    // the game's whole cast gets a short card; the bigger roles get their own camera work.
    // Every character is an ironic caricature, and every scene has at least one pull-back
    // that reveals something new about the character
    const CARD_S = 1.8 + READ_S;
    // each scene: ASCII art, its colour, an animated layer and the caption
    const scenes = [
        {
            who: 'PÄÄOSASSA', name: 'PULTSARI (SINÄ)',
            line: 'Via dolorosalla El Doradoon: Vovan viinakellariin, jossa kalja virtaa kuin vodka.',
            color: '#c8c8a0',
            dur: 7.4 + READ_S,
            // he staggers on towards El Dorado and walks straight through everyone in his way;
            // each one he bumps topples over flat, and he never even notices. The camera stays
            // tight on his face the whole way; the pull-back at the end shows the ditch full of people
            render(ctx, W, H, t, fs) {
                const SPEED = 7;                                   // cells per second
                const px = -2 + SPEED * t;                         // his left edge
                const wide = ease((t - 5.7) / 0.9);
                const cam = lens(ctx, W, H, fs, 3.0 - 1.9 * wide, (px + 2.5) * (1 - wide) + 40 * wide, -0.6 * (1 - wide) + 1.6 * wide);
                const { text } = cam;
                const skin = '#e6e0d0';

                // a figure drawn from rows of text, tipped over around its feet
                const figure = (x, rows, angle, pivot, dy = 0) => {
                    ctx.save();
                    ctx.translate(cam.sx(x + pivot), cam.sy(1.5 + dy));
                    ctx.rotate(angle);
                    rows.forEach(([s, c], i) => {
                        ctx.fillStyle = c;
                        ctx.fillText(s, -pivot * cam.cw, (i - rows.length + 0.5) * cam.ch);
                    });
                    ctx.restore();
                };

                // the road, and the ditch beside it
                text('_'.repeat(140), px - 30, 1.9, '#555');
                text('.,  ,. ,'.repeat(18), px - 30, 6.1, '#3e4a2e');

                // the bystanders: where they stand, what they look like, what they say going down
                const pink = '#d07a9a', blue = '#6d8bff', grey = '#d8d8d8', beer = '#c9b37a';
                const crowd = [
                    { x: 9,  w: 7, say: 'Oi-voi!', rows: [[' ,/~~\\. ', pink], ['( o o )', skin], ['/|###|\\', pink], [' /___\\ ', pink]] },
                    { x: 20, w: 7, say: '*SEMETŠKAT*', rows: [['(¬_¬)', skin], ['_/|≡|\\_', grey], [' _/ \\_', grey]] },
                    { x: 31, w: 5, say: '*PILLI*', rows: [['[МИЛ]', blue], ['(-_-)', skin], ['/|█|\\', blue], [' / \\ ', blue]] },
                    { x: 42, w: 5, say: 'Harashoo!', rows: [['(^o^)', skin], ['\\|=|/', beer], [' / \\ ', beer]] },
                ];
                crowd.forEach((b) => {
                    const hit = (b.x - 5) / SPEED;                 // his belly reaches them
                    const k = Math.max(0, t - hit);
                    // over hard and off the road into the ditch, then a little bounce there
                    const fall = Math.min(1, (k / 0.35) ** 2);
                    const bounce = k > 0.35 ? Math.max(0, Math.sin((k - 0.35) * 14) * 0.12 * Math.exp(-(k - 0.35) * 6)) : 0;
                    figure(b.x + 3 * fall, b.rows, (Math.PI / 2) * (fall - bounce), b.w, 4.3 * fall);
                    if (k > 0 && k < 0.4) text('*KOPS*', b.x + 1, -3.2, '#f2d34a');
                    else if (k > 0.4 && k < 1.3) text(b.say, b.x + 1, -3.2, '#bbb');
                });

                // the sign at the end of the road
                text('  ______________', 56, -3, '#c9a96a');
                text(' | EL DORADO  → |', 56, -2, '#f2d34a');
                text(' | ВОВА 1200 км |', 56, -1, '#c9a96a');
                text(' |______________|', 56, 0, '#c9a96a');
                text('        ||', 56, 1, '#8f7c50');

                // Pultsari: stagger, sway, bottle in hand, eyes on the horizon
                const sway = Math.sin(t * 5.5) * 0.14;
                const legs = Math.floor(t * 8) % 2 ? ' |\\ ' : ' / \\';
                figure(px, [
                    [' ___', '#6b5a3a'],
                    ['(x_x)', skin],
                    ['/|#|\\_[]', '#c8c8a0'],
                    [legs, '#c8c8a0'],
                ], sway, 2.5);
                if (Math.floor(t * 3) % 2 === 0) text('*hic*', px - 3.5, -2.6 - (t % 0.66), '#f2d34a');
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
            who: 'SEKÄ', name: 'JUOPPO', line: '"Harashoo, tähtiorkesteri soittaa dobri stolisnada!" Juo mitä tahansa.',
            color: '#c9b37a',
            dur: 5.8 + READ_S,
            // he bends down for the bottle at his feet, tips his head back, pours the whole thing
            // down his throat, throws the empty over his shoulder and belches with joy. Close on him
            // until the throw; the pull-back shows him standing on top of a mountain of empties
            render(ctx, W, H, t, fs) {
                const wide = ease((t - 3.6) / 0.8);
                const cam = lens(ctx, W, H, fs, 2.2 - 1.1 * wide, 3 - 0.5 * wide, -1.6 + 4.8 * wide);
                const { text, cw, ch } = cam;
                const skin = '#e6e0d0', beer = '#c9b37a', glass = '#9fc8e8';
                const span = (a, b) => ease((t - a) / (b - a));

                // the upper body leans around the hip: forward for the bottle, back for the drink
                const bend = 0.85 * span(0.4, 0.9) - 1.2 * span(1.15, 1.7) + 0.35 * span(3.6, 3.9);
                const sway = t > 3.9 ? Math.sin((t - 3.9) * 6) * 0.12 : 0;
                const lean = bend + sway;
                const hipX = cam.sx(2.5), hipY = cam.sy(0.6);
                const local = (lx, ly, a = lean) => [hipX + lx * Math.cos(a) - ly * Math.sin(a), hipY + lx * Math.sin(a) + ly * Math.cos(a)];
                const mouth = local(0, -1.6 * ch);
                const shoulder = local(1.1 * cw, -0.7 * ch);

                // the bottle: on the ground, in the hand, upside down over his mouth, then thrown
                const ground = [cam.sx(6.5), cam.sy(0.6)];
                const pourAng = Math.PI + 0.35;
                const pour = [mouth[0] + 0.48 * ch, mouth[1] - 3.0 * ch];
                let bx, by, bang;
                if (t < 1.15) { [bx, by] = ground; bang = 0; }
                else if (t < 1.7) {
                    const k = span(1.15, 1.7);
                    bx = ground[0] + (pour[0] - ground[0]) * k; by = ground[1] + (pour[1] - ground[1]) * k; bang = pourAng * k;
                } else if (t < 3.6) { [bx, by] = pour; bx += Math.sin(t * 20) * 1.5; bang = pourAng; }
                else {
                    // over his shoulder in an arc, spinning, and down behind him
                    const k = Math.min(1, (t - 3.6) / 0.6);
                    const lx = cam.sx(-6), ly = cam.sy(4.2);
                    bx = pour[0] + (lx - pour[0]) * k; by = pour[1] + (ly - pour[1]) * k - Math.sin(k * Math.PI) * 2.5 * ch;
                    bang = k < 1 ? pourAng + k * 9 : Math.PI / 2;
                }
                const neck = [bx - 1.4 * ch * Math.sin(bang) * -1, by - 1.4 * ch * Math.cos(bang)];

                // the mountain of empties he stands on, his legs, the arm that holds the bottle
                const rnd = (i) => { const v = Math.sin(i * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
                const glyphs = ['o=', '=o', '▯', 'Ů', '0', '=]'], tints = [glass, '#6a9a5a', '#8a6a3a', '#b0b0b0'];
                for (let row = 0; row < 9; row++) {
                    const half = 3 + row * 2.6;
                    for (let c = -half, i = 0; c < half; i++) {
                        const g = glyphs[Math.floor(rnd(row * 97 + i) * glyphs.length)];
                        text(g, 2.5 + c, 1.6 + row, tints[Math.floor(rnd(row * 31 + i + 7) * tints.length)]);
                        c += g.length + (rnd(row * 13 + i) < 0.3 ? 1 : 0);
                    }
                }
                text(' / \\ ', 0, 1, beer);
                const holding = t > 1.0 && t < 3.6;
                if (holding) {
                    ctx.strokeStyle = beer;
                    ctx.lineWidth = Math.max(2, cw * 0.14);
                    ctx.beginPath();
                    ctx.moveTo(shoulder[0], shoulder[1]);
                    ctx.lineTo(bx, by + 0.3 * ch * Math.cos(bang));
                    ctx.stroke();
                }

                // head and torso, leaning together
                const face = t < 1.15 ? '(^o^)' : t < 3.6 ? '(°O°)' : '(@‿@)';
                const torso = holding ? '\\|=| ' : t > 4.0 ? '\\|=|/' : '/|=|\\';
                ctx.save();
                ctx.translate(hipX, hipY);
                ctx.rotate(lean);
                ctx.fillStyle = skin; ctx.fillText(face, -2.5 * cw, -1.6 * ch);
                ctx.fillStyle = beer; ctx.fillText(torso, -2.5 * cw, -0.6 * ch);
                ctx.restore();

                // the bottle itself
                ctx.save();
                ctx.translate(bx, by);
                ctx.rotate(bang);
                [[' n ', glass], ['|В|', glass], ['|_|', glass]].forEach(([s, c], i) => {
                    ctx.fillStyle = c; ctx.fillText(s, -1.5 * cw, (i - 1) * ch);
                });
                ctx.restore();

                // the pour: a thin stream from the neck straight into his open mouth
                if (t > 1.7 && t < 3.6) {
                    ctx.fillStyle = glass;
                    for (let i = 0; i < 6; i++) {
                        const k = (t * 3 + i / 6) % 1;
                        ctx.fillRect(neck[0] + (mouth[0] - neck[0]) * k - 1.5, neck[1] + (mouth[1] - neck[1]) * k - 1.5, 3, 4);
                    }
                    if (Math.floor(t * 3) % 2 === 0) text('*glug*', 6, -3.4 + (t % 0.66) * 0.5, '#ddd');
                }
                if (t > 4.15 && t < 4.7) text('*KLINK*', -12, 2.6, '#f2d34a');
                if (t > 4.6 && t < 5.8) text('*RÖÖÖYH*', 5, -2.8, '#f2d34a');
            },
        },
        {
            who: 'VIERAILEVA TÄHTI', name: 'MILIISI', line: '"Palvellakseni ja auttaakseni." Ottaa vastaan vain käteistä.',
            color: '#6d8bff',
            dur: 6.6 + READ_S,
            // close on his face; pull back: a driver hands a note out of his car window, the note
            // goes into the officer's breast pocket, he kicks the car and waves it off, and the
            // driver leaves, shouting out of the window
            render(ctx, W, H, t, fs) {
                const wide = ease((t - 1.2) / 0.6);
                const cam = lens(ctx, W, H, fs, 3.4 - 1.9 * wide, 2.5 + 8.5 * wide, -1 + 1.3 * wide);
                const { text, cw, ch } = cam;
                const blue = '#6d8bff', skin = '#e6e0d0', lada = '#c9c0a0', money = '#7fbf5f';
                const span = (a, b) => ease((t - a) / (b - a));
                const arm = (x0, y0, x1, y1, color) => {
                    ctx.strokeStyle = color;
                    ctx.lineWidth = Math.max(2, cw * 0.14);
                    ctx.beginPath();
                    ctx.moveTo(cam.sx(x0), cam.sy(y0));
                    ctx.lineTo(cam.sx(x1), cam.sy(y1));
                    ctx.stroke();
                };

                // the car: a Lada, driver at the wheel; the kick shakes it and leaves a dent
                const shake = t > 3.45 && t < 3.9 ? Math.sin((t - 3.45) * 45) * 0.25 * (1 - (t - 3.45) / 0.45) : 0;
                // it pulls up beside him while the camera pulls back, so the close-up shows him alone
                const carX = 5 + 20 * (1 - ease((t - 0.8) / 1.0)) + shake + 34 * Math.pow(Math.max(0, (t - 4.6) / 1.4), 2);
                const driverFace = t < 1.8 ? '(ò_ó)' : t < 3.45 ? '(-_-)' : t < 4.6 ? '(°o°)' : '(ò▫ó)';
                text('    ______', carX, -1, lada);
                text('  _/', carX, 0, lada);
                text(driverFace, carX + 4, 0, skin);
                text('\\_____', carX + 9, 0, lada);
                text(t > 3.45 ? ' )___ЛАДА______|' : ' |___ЛАДА______|', carX, 1, lada);
                text('   (O)      (O)', carX, 2, '#999');
                if (t > 4.6) {
                    // exhaust, straight into the officer's face
                    for (let i = 0; i < 4; i++) {
                        const k = (t * 1.6 + i / 4) % 1;
                        text(k < 0.5 ? 'o' : 'O', carX - 0.5 - k * 7, 1.6 - k * 1.2, '#777');
                    }
                    cam.font(0.8);
                    text('"BLYAT!!"', carX + 4, -2.3, '#f2d34a');
                    cam.font();
                }

                // the note: out of the window, into his hand, into his breast pocket
                let nx = null, ny = 0;
                if (t > 1.8 && t < 3.1) {
                    const out = span(1.8, 2.3), pocket = span(2.6, 3.1);
                    nx = carX + 4 + (6.2 - carX - 4) * out + (2.5 - 6.2) * pocket;
                    ny = -0.2 + 0.3 * pocket;
                    if (t < 2.6) arm(carX + 4.5, 0.2, nx, ny, skin);
                    if (t > 2.3) arm(4.2, -0.1, nx + 0.8, ny, blue);
                    text('[₽]', nx - 1, ny, money);
                }
                if (t > 3.0 && t < 3.4) text('*tunk*', -1.5, -2.9, '#999');

                // the officer
                const kicking = t > 3.4 && t < 3.7;
                const pointing = t > 3.75 && t < 4.9;
                const face = t < 2.3 ? '(-_-)' : t < 4.6 ? '(¬‿¬)' : t < 6.0 ? '(×_×)' : '(¬‿¬)';
                text(' _▄_ ', 0, -2, blue);
                text(face, 0, -1, skin);
                text(t > 2.3 && t < 3.1 ? '/|█| ' : pointing ? '/|█|—' : '/|█|\\', 0, 0, blue);
                text(' |_| ', 0, 1, blue);
                text(kicking ? ' |___' : ' / \\ ', 0, 2, blue);
                if (t > 3.45 && t < 3.85) text('*BONK*', 5, -2.6, '#f2d34a');
                if (pointing) text('"Davai, pošol!"', -2, -3.4, '#ddd');
                if (t > 5.0 && t < 6.0) text('*köh köh*', -1.5, -3.4, '#999');
            },
        },
        {
            who: 'SEKÄ', name: 'GOPNIK', line: '"Jest\' zakurit\'?" Kyykkii ja uhkailee. Verkkareissa.',
            color: '#d8d8d8',
            dur: 6.8 + READ_S,
            // close on him squatting, spitting sunflower-seed shells, asking for a smoke; pull back:
            // he squats in a children's sandbox, and his mother, drunk, yells from the balcony that
            // dinner is ready. He gets up, says he has to go, and shuffles home
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const wide = span(2.0, 2.8);
                const cam = lens(ctx, W, H, fs, 3.0 + 0.3 * span(0.9, 1.6) - 2.05 * wide, 3.5 + 15.5 * wide, -1.2 - 2.0 * wide);
                const { text } = cam;
                const grey = '#d8d8d8', skin = '#e6e0d0', brick = '#8a7a6a', wood = '#8f7c50';

                // the block of flats behind the playground: a khrushchyovka, windows in rows
                text('_'.repeat(27), 16, -13, brick);
                for (let y = -12; y <= 1; y++) { text('|', 15.5, y, brick); text('|', 42.5, y, brick); }
                for (let y = -11; y <= -1; y += 2) for (let x = 18; x <= 38; x += 5) text('▒▒', x, y, '#4a5a6a');
                text('|▓|', 27.5, 0.5, wood);

                // mother on the balcony: curlers, dressing gown, swaying, hiccuping between the words
                if (t > 2.6) {
                    const sway = Math.sin(t * 4) * 0.25;
                    text(' @@@ ', 22.5 + sway, -7, '#e0a0c0');
                    text(t > 2.9 ? '(=O=)' : '(=_=)', 22.5 + sway, -6, skin);
                    text('|=======|', 21.5, -5, brick);
                    cam.font(0.75);
                    const shout = [
                        [2.9, '"VITJAAA!"'],
                        [3.4, '*hik*'],
                        [3.7, '"KOTIIN SYÖMÄÄN!"'],
                        [4.3, '*hik*'],
                    ];
                    shout.forEach(([at, s], i) => {
                        if (t > at && t < 5.0) text(s, 31 + (i % 2) * 2, -8.6 + i * 1.1, s[0] === '*' ? '#f2d34a' : '#fff');
                    });
                    cam.font();
                }

                // the playground: the ground, a mushroom shelter, the sandbox with a bucket in it
                text('_'.repeat(90), -30, 1.9, '#555');
                text('.-"""""-.', 8, -3, '#c04040');
                for (let y = -2; y <= 1; y++) text('|', 12, y, wood);
                text('[' + '='.repeat(17) + ']', -4, 1.6, wood);
                text('\\_/', 7.5, 1, '#5a8acf');

                // the shells he spits land in the sand and stay there
                [0.4, 1.0, 1.7].forEach((at, i) => {
                    if (t < at) return;
                    const k = Math.min(1, (t - at) / 0.45);
                    text('‚', 4 + k * (2.5 + i), -1 + k * 2.2 - Math.sin(k * Math.PI) * 0.8, '#bba');
                });

                // Vitja himself: squatting, then standing up and shuffling home
                const walk = Math.max(0, (t - 5.6) / 1.2);
                const gx = 1 + walk * 26;
                if (t < 4.9) {
                    text(' ▄▄▄ ', gx, -2, '#555');
                    text(t < 3.0 ? '(¬_¬)' : '(°_°)', gx, -1, skin);
                    text('_/|≡|\\_', gx - 1, 0, grey);
                    text(' _/ \\_', gx - 1, 1, grey);
                } else if (gx < 28) {
                    text(' ▄▄▄ ', gx, -3, '#555');
                    text('(._.)', gx, -2, skin);
                    text('/|≡|\\', gx, -1, grey);
                    text(' |_| ', gx, 0, grey);
                    text(walk > 0 && Math.floor(t * 8) % 2 ? ' |\\  ' : ' / \\ ', gx, 1, grey);
                }
                if (t > 0.6 && t < 2.0) {
                    text('"Jest\' zakurit\'?"', -2, -3.2, '#fff');
                    cam.font(0.6);
                    text('(Onko tupakkaa?)', 0, -2.6, '#999');
                    cam.font();
                }
                if (t > 5.0 && t < 6.0) text('"Täytyy mennä."', gx - 2, -4.4, '#ddd');
            },
        },
        {
            who: 'SEKÄ', name: 'VARAS', line: '"Hipelöin sua!!" Ja lompakkoasi.',
            color: '#b08cd0',
            dur: 7.8 + READ_S,
            // four shots, each pulling back further: (1) close on him running in with open arms,
            // shouting for his long-lost brother; (2) the hug, close enough to show only his hand
            // lifting the officer's wallet, the officer's pocket left turned out; (3) the officer
            // has had his too: the thief's back pocket hangs turned out as well; (4) they part, each
            // sneaks a look at his loot, and a moth flies out of each wallet
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const run = span(0, 1.2);
                const walk = Math.max(0, t - 4.3) * 2.6;
                const vx = -10 + 10 * run - walk;              // the thief
                const mx = 5 + walk;                            // the officer
                const shots = [
                    { zoom: 6.5, x: vx + 2.5, y: -1 },          // his face
                    { zoom: 6.0, x: 7.2, y: -0.1 },             // his hand, the officer's hip
                    { zoom: 3.0, x: 4.5, y: -0.3 },             // both of them
                    { zoom: 1.5, x: 3.5, y: -0.6 },             // parting ways
                ];
                const cuts = [[1.0, 0.6], [2.9, 0.6], [4.7, 0.8]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text, cw } = cam;
                const purple = '#b08cd0', blue = '#6d8bff', skin = '#e6e0d0', money = '#7fbf5f', lining = '#ddd';
                const arm = (x0, y0, x1, y1, color) => {
                    ctx.strokeStyle = color;
                    ctx.lineWidth = Math.max(2, cw * 0.14);
                    ctx.beginPath();
                    ctx.moveTo(cam.sx(x0), cam.sy(y0));
                    ctx.lineTo(cam.sx(x1), cam.sy(y1));
                    ctx.stroke();
                };
                const hugging = t > 1.2 && t < 4.3;
                const leaving = t > 4.3;
                const peek = t > 5.3;                          // a sneaky look at the loot
                const open = t > 5.9;
                const walking = (t < 1.2 || leaving) && Math.floor(t * 8) % 2;

                // the officer, as in his own scene; walks off to the right
                text(' _▄_ ', mx, -2, blue);
                text(open ? '(._.)' : peek ? '(◔_◔)' : t > 1.2 ? '(¬‿¬)' : '(-_-)', mx, -1, skin);
                text(hugging ? ' |█|\\' : leaving ? '/|█| ' : '/|█|\\', mx, 0, blue);
                text(' |_| ', mx, 1, blue);
                text(walking ? '  /| ' : ' / \\ ', mx, 2, blue);
                if (t > 2.15) text('∪', mx + 2, 1.15, lining);           // his pocket, turned out

                // the thief: running in, arms wide, hugging; walks off to the left
                text(open ? '(._.)' : peek ? '(◔_◔)' : '(^▽^)', vx, -1, skin);
                text(hugging ? '/|▓| ' : t < 1.2 ? '\\|▓|/' : leaving ? ' |▓|\\' : '/|▓|\\', vx, 0, purple);
                text(' |_| ', vx, 1, purple);
                text(walking ? ' |\\  ' : ' / \\ ', vx, 2, purple);
                if (t > 2.6) text('∪', vx + 0.2, 1.15, lining);          // and his, too

                if (hugging) {
                    // his arm round the officer's back; the officer's round his, to the back pocket
                    arm(vx + 4.2, -0.1, mx + 4.6, -0.3, purple);
                    arm(mx + 0.8, -0.1, vx + 0.4, 0.7, blue);
                    if (t > 3.0 && Math.floor(t * 6) % 2) text('*hipel hipel*', 1, -2.6, '#bbb');
                }

                // the officer's wallet: from his hip into the thief's sleeve
                const lift = span(1.9, 2.4);
                if (t < 2.4) text('[₽]', mx + 1 + (vx + 3.4 - mx - 1) * lift, 0.7 - 0.8 * lift, money);
                // the thief's: already in the officer's hand when the camera gets there, then
                // into his breast pocket
                const tuck = span(3.6, 4.0);
                if (t > 2.6 && t < 4.0) text('[₽]', vx + 0.2 + (mx + 1.2 - vx - 0.2) * tuck, 0.7 - 0.8 * tuck, money);

                if (leaving) {
                    // each takes the other's wallet out, holds it close and opens it
                    if (peek) {
                        text(open ? '[  ]' : '[₽]', vx - 3.6, -0.1, money);
                        text(open ? '[  ]' : '[₽]', mx + 5, -0.1, money);
                    }
                    if (open) {
                        [vx - 3.2, mx + 5.4].forEach((x, i) => {
                            const k = t - 5.9;
                            text(Math.floor(t * 12 + i) % 2 ? '}{' : '><', x + Math.sin(k * 7 + i) * 0.8, -0.8 - k * 2.2, '#bba');
                        });
                    }
                    if (t < 5.2) {
                        text('"Davai, brat!"', vx - 5, -3.0, '#ddd');
                        text('"Davai!"', mx + 1, -3.6, '#ddd');
                    }
                }

                if (t > 0.15 && t < 1.6) {
                    cam.font(0.35);
                    text('"BRAT! Pitkästä aikaa!"', vx - 1.5, -2.0, '#fff');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'RYÖSTÄJÄ', line: '"RAHAT TAI HENKI!" Mieluiten molemmat.',
            color: '#d06a4a',
            dur: 10.4 + READ_S,
            // close on him, gun out, shouting his line; pull back: it's a bank counter, and the
            // cashier has bad news. Push back in on him: he turns and looks into the camera. Pull
            // back: an officer has come up behind him and cuffs him, a second one comes in, they
            // carry him out like a log; pull back again: outside waits a Z truck, they throw him in
            // the back and off he goes to the front
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 4.0, x: 3, y: -1.2 },             // him and his gun
                    { zoom: 1.6, x: 12, y: -1 },              // the bank
                    { zoom: 5.0, x: 3.5, y: -1.1 },           // him again, closer
                    { zoom: 2.4, x: -1, y: -0.4 },            // the officer behind him
                    { zoom: 1.0, x: -18, y: -1.5 },           // the street and the truck
                ];
                const cuts = [[1.5, 0.7], [4.2, 0.6], [5.6, 0.6], [7.6, 0.9]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text, cw, ch } = cam;
                const red = '#d06a4a', skin = '#e6e0d0', wood = '#8f7c50', blue = '#6d8bff', khaki = '#5b6b3a';

                // the bank: a cashier's window with the cashier behind the glass, bored
                const bx = 14;
                text('_'.repeat(43), -9, -5, '#666');
                for (let y = -4; y <= -2; y++) text('|', -9, y, '#666');
                text(' _________________', bx, -4, wood);
                text('|    К А С С А    |', bx, -3, '#f2d34a');
                text('|  ┌───────────┐  |', bx, -2, '#9fb7c9');
                text('|  │', bx, -1, '#9fb7c9');
                text('(=_=)', bx + 7, -1, skin);
                text('│  |', bx + 15, -1, '#9fb7c9');
                text('|  └───────────┘  |', bx, 0, '#9fb7c9');
                text('|=================|', bx, 1, wood);
                text('|  $ —.—   € —.—  |', bx, 2, '#888');
                text('_'.repeat(120), -70, 2.6, '#555');
                if (t > 2.3 && t < 4.4) {
                    cam.font(0.8);
                    text('"Dollarit on loppu."', bx + 2, -6.0, '#ddd');
                    if (t > 3.0) text('"Rupla on arvoton."', bx + 2.5, -5.1, '#ddd');
                    cam.font();
                }

                // the officers: one sneaks in behind him while he stares, the other comes later
                const lift = span(6.9, 7.3), carry = span(7.3, 8.5);
                const m1 = -16 + 7 * span(4.3, 5.4) + 4 * span(6.0, 6.3) - 16 * carry;
                const m2 = 22 - 14.5 * span(6.5, 7.1) - 17.5 * carry;
                const officer = (x, walking) => {
                    text(' _▄_ ', x, -2, blue);
                    text('(ò_ó)', x, -1, skin);
                    text(lift > 0 && t < 8.6 ? '\\|█|/' : '/|█|\\', x, 0, blue);
                    text(' |_| ', x, 1, blue);
                    text(walking && Math.floor(t * 8) % 2 ? ' |\\  ' : ' / \\ ', x, 2, blue);
                };

                // the robber: drawn around his middle so he can be lifted, carried and thrown
                const turn = t < 4.55 ? 0 : t < 4.75 ? 1 : 2;
                const cuffed = t > 6.35;
                let rx = 3.5, ry = 0, rang = 0, inTruck = false;
                if (lift > 0) {
                    const mid = (m1 + m2) / 2 + 2.5;
                    rx = 3.5 + (mid - 3.5) * lift; ry = -3 * lift; rang = -Math.PI / 2 * lift;
                }
                const toss = (t - 8.5) / 0.45;
                if (toss > 0) {
                    const k = Math.min(1, toss);
                    const from = (m1 + m2) / 2 + 2.5;
                    rx = from + (-33 - from) * k; ry = -3 + 2.2 * k - Math.sin(k * Math.PI) * 3; rang = -Math.PI / 2 - k * 6;
                    inTruck = toss >= 1;
                }
                if (!inTruck) {
                    ctx.save();
                    ctx.translate(cam.sx(rx), cam.sy(ry));
                    ctx.rotate(rang);
                    const rows = [
                        ['[▀▀▀▀▀]', '#555'],
                        [['(  •_•)', '( •_• )', '( ◉_◉ )'][turn], skin],
                        [cuffed ? '  |█|' : turn < 2 ? ' /|█|\\=¬' : ' /|█|\\', red],
                        ['  |_|', red],
                        ['  / \\', red],
                    ];
                    rows.forEach(([s, c], i) => { ctx.fillStyle = c; ctx.fillText(s, -3.5 * cw, (i - 2) * ch); });
                    if (cuffed) { ctx.fillStyle = '#ccc'; ctx.fillText('∞', -2.3 * cw, 0.4 * ch); }
                    ctx.restore();
                }
                // the gun: held low while he stares, then on the floor
                if (turn === 2) text('¬=', cuffed ? 6 : 5.2, cuffed ? 2.2 : 0.8, red);
                if (t > 6.3 && t < 6.8) text('*KLIK*', -3, -3.3, '#f2d34a');
                if (t > 5.4 && t < 5.9) text('...', 7, -1.6, '#888');
                if (t > 0.2 && t < 1.7) {
                    cam.font(0.5);
                    text('"RAHAT TAI HENKI!"', -1.5, -3.1, '#fff');
                    cam.font();
                }

                // the truck outside: it hides whatever goes in the back, then drives off to the front
                const tx = -46 - 60 * Math.pow(Math.max(0, (t - 9.2) / 1.3), 2);
                ctx.fillStyle = '#000';
                ctx.fillRect(cam.sx(tx + 5), cam.sy(-2), cam.sx(tx + 21) - cam.sx(tx + 5), cam.sy(1.5) - cam.sy(-2));
                text('      ______________', tx, -2, khaki);
                text('  ___|              |', tx, -1, khaki);
                text(' /[] |', tx, 0, khaki);
                text('Z', tx + 12, 0, '#fff');
                text('|', tx + 20, 0, khaki);
                text('|____|______________|', tx, 1, khaki);
                text('  (O)      (O)    (O)', tx, 2, '#999');
                if (t > 8.9 && t < 9.4) text('*TÖMPS*', tx + 10, -3.4, '#f2d34a');
                if (t > 9.2) {
                    for (let i = 0; i < 4; i++) {
                        const k = (t * 2 + i * 0.25) % 1;
                        text(k < 0.5 ? 'o' : 'O', tx + 21 + k * 6, 1.6 - k * 0.8, '#777');
                    }
                }

                const walking = (t > 4.3 && t < 5.4) || (t > 6.0 && t < 6.3) || (t > 6.5 && t < 7.1) || (t > 7.3 && t < 8.5);
                officer(m1, walking);
                officer(m2, walking);
                if (t > 9.0 && t < 10.4) text('"Rintamalle!"', m1 - 2, -3.4, '#ddd');
            },
        },
        {
            who: 'SEKÄ', name: 'DIILERI', line: '"Tässä on hyvää kamaa!" Ei ole.',
            color: '#7fd0a0',
            dur: 10.1 + READ_S,
            // close on him whispering; pull back: his coat opens on a lining full of sanctioned
            // Western goods; pull back again: a customer. The iPhones are sold out, so is the
            // Coca-Cola and the Snickers, but there are IKEA meatballs: from 2021
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 4.5, x: 11, y: -0.8 },            // his face
                    { zoom: 2.6, x: 11, y: 0.6 },             // the open coat
                    { zoom: 1.4, x: 17, y: 0 },               // and the customer
                ];
                const cuts = [[1.2, 0.6], [2.6, 0.6]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text, cw } = cam;
                const green = '#7fd0a0', coat = '#6a6a5a', skin = '#e6e0d0', lining = '#a03030';

                // the dealer in a long coat; the flaps swing open from his shoulders
                const sold = (at) => t > at;
                const open = span(1.1, 1.5);
                text(t < 1.1 ? '(¬_¬)' : '(¬‿¬)', 8.5, -1, skin);
                text('/|▒|\\', 8.5, 0, coat);
                text(' |▒| ', 8.5, 1, coat);
                text(' / \\ ', 8.5, 2, coat);
                if (t < 1.1 && Math.floor(t * 5) % 2) text('✦', 11.3, 0.6, '#fff');
                const flap = (side, rows) => {
                    if (open <= 0) return;
                    const pivot = side < 0 ? cam.sx(8.6) : cam.sx(13.4);
                    ctx.save();
                    ctx.translate(pivot, 0);
                    ctx.scale(open, 1);
                    ctx.translate(-pivot, 0);
                    rows.forEach(([s, c], i) => text(s, side < 0 ? -2.4 : 13.4, i, c));
                    ctx.restore();
                };
                const item = (name, gone, sale) => [sale ? '│  LOPPU  │' : gone ? '│         │' : name, sale ? '#e05050' : '#f2d34a'];
                flap(-1, [
                    ['┌─────────┐', lining],
                    item('│ iPhone  │', false, sold(4.0)),
                    item('│Snickers │', false, sold(7.0)),
                    ['└─────────┘', lining],
                ]);
                flap(1, [
                    ['┌─────────┐', lining],
                    item('│Coca-Cola│', false, sold(5.5)),
                    item('│  IKEA   │', sold(8.5), false),
                    ['└─────────┘', lining],
                ]);

                // the customer
                const ux = 31;
                text(t > 9.6 ? '(x_x)' : t > 9.1 ? '(°_°)' : '(o_o)', ux, -1, skin);
                text(t > 8.8 ? '\\|=| ' : '/|=|\\', ux, 0, '#8a9ab0');
                text(' |_| ', ux, 1, '#8a9ab0');
                text(' / \\ ', ux, 2, '#8a9ab0');

                // the meatballs: out of the coat into his hand, and they smell like 2021
                if (t > 8.5) {
                    const k = span(8.5, 8.9);
                    const px = 15 + (ux - 10.5 - 15) * k, py = 2 - 1.7 * k;
                    text('[IKEA 2021]', px, py, '#8a9a3a');
                    if (t > 8.9) {
                        for (let i = 0; i < 3; i++) {
                            const s = (t * 1.5 + i / 3) % 1;
                            text('~', px + 2 + i * 3 + Math.sin(s * 9) * 0.4, py - 0.8 - s * 1.6, '#8a9a3a');
                        }
                        text(Math.floor(t * 10) % 2 ? '·' : '˙', px + 9 + Math.sin(t * 13) * 1.2, py - 1.5 + Math.cos(t * 11) * 0.6, '#ccc');
                    }
                }

                // the deal
                cam.font(t < 1.2 ? 0.45 : 0.8);
                if (t > 0.15 && t < 1.4) text('"Psst… hyvää kamaa!"', 7.5, -2.3, '#ddd');
                cam.font(0.8);
                const lines = [
                    [3.4, 4.8, 'u', '"iPhone?"'],
                    [4.0, 5.5, 'd', '"iPhonet on loppu."'],
                    [4.9, 6.3, 'u', '"Coca-Colaa?"'],
                    [5.5, 7.0, 'd', '"Coca-Colakin on loppu."'],
                    [6.4, 7.8, 'u', '"Snickersiä?"'],
                    [7.0, 8.5, 'd', '"Snickersitkin on loppu."'],
                    [7.9, 10.1, 'u', '"IKEA-lihapullia?"'],
                    [8.5, 10.1, 'd', '"Tässä. Nämä ovat vuodelta 2021."'],
                ];
                lines.forEach(([from, until, who, s]) => {
                    if (t > from && t < until) text(s, who === 'u' ? ux - 6 : 2, who === 'u' ? -3.2 : -4.2, who === 'u' ? '#ddd' : green);
                });
                cam.font();
            },
        },
        {
            who: 'SEKÄ', name: 'SKINHEAD', line: '"TAPAN SINUT!!" Sanoo sen kaikille.',
            color: '#c06060',
            dur: 5.6 + READ_S,
            // close on him screaming; pull back: a whole row of him, skull tattoos, screaming
            // together; pull back again: the banner above them reads DENAZIFICATION
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 5.5, x: 2.5, y: -0.9 },           // his face
                    { zoom: 1.6, x: 2.5, y: -0.5 },           // the row of them
                    { zoom: 1.0, x: 2.5, y: -3 },             // and the banner
                ];
                const cuts = [[1.3, 0.6], [3.0, 0.7]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#e6d0c0', jacket = '#6a7a52', red = '#c06060';

                // the row: five of the same man, skull on the arm, boots laced to the knee
                const screaming = (t > 0.2 && t < 1.3) || (t > 1.9 && t < 3.0);
                [-20, -10, 0, 10, 20].forEach((x, i) => {
                    const shake = screaming ? Math.sin(t * 40 + i) * 0.08 : 0;
                    text(screaming ? '(Ò□Ó)' : '(Ò_Ó)', x + shake, -1, screaming ? '#e8b0a0' : skin);
                    text('/|█|\\', x, 0, jacket);
                    text('☠', x - 0.1, 0.35, '#ddd');
                    text(' |_| ', x, 1, '#2a3a5a');
                    text('▐▌ ▐▌', x, 2, '#555');
                });
                text('_'.repeat(70), -28, 2.6, '#555');

                // the scream: his alone, then the whole row in unison
                if (t > 0.2 && t < 1.3) {
                    cam.font(0.4);
                    text('"TAPAN SINUT!!"', -0.6, -2.0, '#fff');
                    cam.font();
                }
                if (t > 1.9 && t < 3.0) [-20, -10, 0, 10, 20].forEach((x) => text('"TAPAN SINUT!!"', x - 4.5, -2.6 - (x % 20 === 0 ? 0 : 0.9), red));

                // the banner unrolls from its poles above them
                const unroll = span(3.4, 3.9);
                for (let y = -8; y <= 2; y++) { text('|', -23, y, '#8f7c50'); text('|', 27.5, y, '#8f7c50'); }
                if (unroll > 0) {
                    const top = cam.sy(-8.4), bottom = cam.sy(-8.4 + 3 * unroll);
                    ctx.fillStyle = '#9a1c1c';
                    ctx.fillRect(cam.sx(-22.3), top, cam.sx(27.3) - cam.sx(-22.3), bottom - top);
                    if (unroll > 0.8) {
                        ctx.textAlign = 'center';
                        cam.font(2);
                        ctx.fillStyle = '#fff';
                        ctx.fillText('DENAZIFICATION', cam.sx(2.5), cam.sy(-6.9));
                        ctx.textAlign = 'left';
                        cam.font();
                    }
                }
            },
        },
        {
            who: 'SEKÄ', name: 'PAKOVANKI', line: 'Karkasi SIZOsta. Kiekuralenkki tekee vahvaksi.',
            color: '#c8c8c8',
            dur: 5.8 + READ_S,
            // close on him running, ball and chain bouncing behind, shouting for freedom; pull
            // back: he has just crawled out through a hole in the remand prison wall; pull back
            // again: around everything runs a barbed-wire fence with a sign, РОССИЯ
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const px = -5 + 4.6 * Math.min(t, 4.6);
                const shots = [
                    { zoom: 4.5, x: px + 2.5, y: -0.9 },      // him, running
                    { zoom: 2.0, x: -1, y: -0.8 },            // the prison wall behind him
                    { zoom: 0.6, x: 10, y: -5 },              // the fence round the whole country
                ];
                const cuts = [[1.6, 0.6], [3.2, 0.9]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const brick = '#7a5a4a', grey = '#c8c8c8', skin = '#e6e0d0', wire = '#8a8a8a';

                text('_'.repeat(160), -70, 2.6, '#555');

                // the fence round everything: posts, barbed wire, watchtowers, searchlights
                const L = -50, R = 70;
                text('+—'.repeat((R - L) / 2), L, -12, wire);
                text('~x'.repeat((R - L) / 2), L, -11, '#666');
                for (let y = -12; y <= 2; y++) { text('┼', L, y, wire); text('┼', R, y, wire); }
                [L, R].forEach((x, i) => {
                    text('╔═╗', x - 1, -14, '#999');
                    text('╚╦╝', x - 1, -13, '#999');
                    // searchlight sweeping over the land
                    const a = Math.PI / 2 + Math.sin(t * 1.3 + i * 2) * 0.6;
                    const ox = cam.sx(x + 0.5), oy = cam.sy(-13.5), len = cam.cw * 40;
                    ctx.fillStyle = 'rgba(255,255,200,0.08)';
                    ctx.beginPath();
                    ctx.moveTo(ox, oy);
                    ctx.lineTo(ox + Math.cos(a - 0.12) * len, oy + Math.sin(a - 0.12) * len);
                    ctx.lineTo(ox + Math.cos(a + 0.12) * len, oy + Math.sin(a + 0.12) * len);
                    ctx.fill();
                });
                // the sign on the wire
                const sx0 = cam.sx(1), sx1 = cam.sx(19);
                ctx.fillStyle = '#1a1a1a';
                ctx.fillRect(sx0, cam.sy(-15.6), sx1 - sx0, cam.sy(-11.6) - cam.sy(-15.6));
                ctx.strokeStyle = '#ddd';
                ctx.lineWidth = 2;
                ctx.strokeRect(sx0, cam.sy(-15.6), sx1 - sx0, cam.sy(-11.6) - cam.sy(-15.6));
                ctx.textAlign = 'center';
                cam.font(3);
                ctx.fillStyle = '#fff';
                ctx.fillText('РОССИЯ', cam.sx(10), cam.sy(-13.6));
                ctx.textAlign = 'left';
                cam.font();

                // the remand prison: a brick wall with barbed wire on top and a fresh hole at its foot
                text('~x~x~x~x~', -14, -7, wire);
                for (let y = -6; y <= 2; y++) text(y >= 1 ? '▓▓▓▓▓▓▓  ' : '▓▓▓▓▓▓▓▓▓', -14, y, brick);
                text('СИЗО', -12.5, -4, '#ddd');
                text('∴ .', -5.5, 2.2, brick);

                // him: striped, ball and chain bouncing behind
                const running = t < 4.6;
                const bob = running ? Math.abs(Math.sin(t * 9)) * 0.5 : 0;
                text('O', px - 4, 2 - bob, '#888');
                text('-o-', px - 3, 2.1, '#777');
                text(running ? '(o_O)' : '(._.)', px, -1, skin);
                text('/|≡|\\', px, 0, grey);
                text(' |≡| ', px, 1, grey);
                text(running && Math.floor(t * 10) % 2 ? ' |\\  ' : ' / \\ ', px, 2, grey);
                if (t > 0.3 && t < 1.8) {
                    cam.font(0.45);
                    text('"VAPAUS!"', px + 0.5, -2.2, '#fff');
                    cam.font();
                }
                if (t > 4.7) {
                    cam.font(2.4);
                    text('"...blyat."', px - 3, -4, '#ddd');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'YEVGENI', line: '"Kuka haluaa puhaltaa?" Koulubussin kuljettaja.',
            color: '#a0c0e0',
            dur: 11.6 + READ_S,
            // close on him, swaying; pull back: he holds the bus's alcohol interlock, which says
            // BLOW, and asks who wants to; pull back: a bus full of children, all hands up. Dimitri
            // blows 0.7 per mille, the engine starts, Yevgeni has a swig of vodka; pull back: he
            // puts on his driver's cap and sits down; pull back: the bus coughs off towards school
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 5.0, x: 26.5, y: -1 },            // his face
                    { zoom: 2.6, x: 27, y: -0.3 },            // the interlock in his hand
                    { zoom: 1.3, x: 15, y: -0.5 },            // the children
                    { zoom: 1.05, x: 16, y: -0.6 },           // the whole bus
                    { zoom: 0.55, x: 50, y: -2 },             // the road to school
                ];
                const cuts = [[1.0, 0.6], [2.6, 0.6], [7.0, 0.6], [8.3, 0.8]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const skin = '#e6e0d0', yellow = '#f2c94a', blue = '#a0c0e0';

                // the drive: slow, coughing, nearly stalling halfway, then on again
                const s = Math.max(0, t - 8.4);
                const eff = s < 1.2 ? s : s < 1.9 ? 1.2 + (s - 1.2) * 0.12 : 1.284 + (s - 1.9);
                const bo = 8 * eff + (s > 0 ? Math.sin(s * 14) * 0.25 : 0);
                const running = t > 5.1;
                const by = running && (s < 1.2 || s > 1.9) ? Math.sin(t * 50) * 0.06 : 0;
                const text = (str, x, y, c) => cam.text(str, x + bo, y + by, c);
                const free = cam.text;                       // things that are not on the bus

                // the road and the school at the end of it
                free('_'.repeat(200), -40, 3.9, '#555');
                free('  _________', 98, -7, '#c8b8a0');
                free(' |  ШКОЛА  |', 98, -6, '#f2d34a');
                free(' | ▒  ▒  ▒ |', 98, -5, '#4a5a6a');
                free(' | ▒  ▒  ▒ |', 98, -4, '#4a5a6a');
                free(' | ▒  ▒  ▒ |', 98, -3, '#4a5a6a');
                free(' | ▒ |█| ▒ |', 98, -2, '#4a5a6a');
                free(' |___|_|___|', 98, -1, '#c8b8a0');

                // the bus, cut open: roof, back, windscreen, floor, wheels
                text('_'.repeat(56), -12, -4.6, yellow);
                for (let y = -4; y <= 2; y++) text('|', -12, y, yellow);
                for (let y = -4; y <= -2; y++) text('\\', 44 + (y + 4) * 0.5, y, yellow);
                for (let y = -1; y <= 2; y++) text('|', 45.5, y, yellow);
                text('='.repeat(58), -12, 2.5, yellow);
                text('▀▀ Д Е Т И ▀▀', 2, 3.2, '#222');
                text('(O)', -7, 3.4, '#999');
                text('(O)', 36, 3.4, '#999');
                text('(+)', 38, -0.4, '#999');            // the steering wheel
                text('▄▄▄', 41.5, 0.6, '#666');           // the dashboard

                // the children: four seats, four kids; Dimitri is the one at the front
                const dimWalk = span(4.0, 4.5) - span(5.7, 6.2);
                const handsUp = t > 3.0 && t < 4.2;
                [-12, -5, 2, 9].forEach((x, i) => {
                    text('█', x - 0.8, 0, '#3a3a3a');
                    text('█', x - 0.8, 1, '#3a3a3a');
                    const kx = i === 3 ? x + 3.5 * dimWalk : x;
                    text(t > 5.1 ? '(^.^)' : '(o.o)', kx, -1, skin);
                    text(handsUp ? '\\|_|/' : '/|_|\\', kx, 0, ['#e07070', '#70b0e0', '#90d070', '#e0c060'][i]);
                    text(' ╯╯ ', kx, 1, '#888');
                    if (handsUp) text('"MINÄ!"', kx - 0.5, -2.3 - (i % 2) * 0.8, '#fff');
                });

                // Yevgeni: standing in the aisle, then on his seat with his cap on
                const seated = t > 7.9;
                const walk = span(7.6, 7.9);
                const yx = 24 + 7 * walk;
                const capOn = t > 7.3;
                const face = t > 6.9 ? '(^_^)' : t > 6.4 ? '(-o-)' : '(o_ô)';
                const sway = t < 6.4 ? Math.sin(t * 3) * 0.15 : 0;
                if (capOn) text('_▄▄▄_', yx + sway, -2 + (1 - span(7.3, 7.5)) * -2, '#334');
                text(face, yx + sway, -1, skin);
                text('/|=|\\', yx, 0, blue);
                text(seated ? ' |_|_' : ' |_| ', yx, 1, blue);
                if (!seated) text(' / \\ ', yx, 2, blue);
                if (t > 0.2 && t < 1.0 && Math.floor(t * 4) % 2) text('*hik*', yx + 5.5, -1.8, '#f2d34a');

                // the interlock: in his hand, then back on the dashboard; wired to the dash
                const dx = seated ? 40.5 : yx - 6, dy = seated ? -1.7 : t < 0.9 ? 4 : 1.4;   // kept out of the close-up
                ctx.strokeStyle = '#555';
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(cam.sx(dx + 5.4 + bo), cam.sy(dy + by));
                ctx.quadraticCurveTo(cam.sx(dx + 3 + bo), cam.sy(2 + by), cam.sx(42.5 + bo), cam.sy(0.6 + by));
                ctx.stroke();
                cam.font(0.6);
                const ok = t > 5.1;
                text(ok ? '[0,7‰ OK]' : '[PUHALLA]', dx, dy, ok ? '#7fe07f' : '#e07070');
                cam.font();

                // Dimitri blows
                if (t > 4.6 && t < 5.1) for (let i = 0; i < 3; i++) text('~', 16.6 + ((t * 4 + i / 3) % 1) * 1.4, -0.5 + ((t * 4 + i / 3) % 1) * 1.7, '#ddd');
                // the vodka
                if (t > 6.4 && t < 6.9) {
                    text('[В]', yx + 4, -1.6, '#9fc8e8');
                    if (Math.floor(t * 6) % 2) text('*glug*', yx + 7, -2.6, '#ddd');
                }

                // the engine: dead until the child blows, then alive, more or less
                if (t > 5.1 && t < 5.7) free('*BRRRM!*', 38 + bo, -6, '#f2d34a');
                if (running) {
                    for (let i = 0; i < 4; i++) {
                        const k = (t * 1.6 + i / 4) % 1;
                        text(k < 0.5 ? 'o' : 'O', -13 - k * 6, 2.8 - k * 1.5, s > 1.0 && s < 2.0 ? '#222' : '#666');
                    }
                }
                cam.font(t > 8.3 ? 2.2 : 1);
                if (s > 0.3 && s < 0.9) free('*köh!*', 30 + bo, -9, '#ddd');
                if (s > 1.2 && s < 1.9) free('*pöt… pöt…*', 26 + bo, -9, '#ddd');
                if (s > 1.9 && s < 2.5) free('*PÖRP!*', 36 + bo, -9, '#f2d34a');
                cam.font();

                // the talk
                cam.font(t < 1.0 ? 0.4 : 0.75);
                if (t > 1.6 && t < 3.2) text('"Kuka haluaa puhaltaa?"', 21, -3.4, '#fff');
                if (t > 5.3 && t < 6.5) {
                    text('"Hyvä tulos, Dimitri."', 18, -3.6, '#fff');
                    if (t > 5.8) text('"Nyt istukaa paikoillenne."', 17, -2.8, '#fff');
                }
                cam.font();
            },
        },
        {
            who: 'SEKÄ', name: 'KOTIÄITI', line: '"Vanjuska... missä sä..." *hik*',
            color: '#e0a0c0',
            dur: 7.8 + READ_S,
            // close on her crying over the vodka; pull back: an officer knocks, brings a medal and
            // the news that her son fell a hero; pull back: in the yard, a new Lada with a bow on
            // it, the compensation. She runs out and hugs it
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 4.5, x: 2.5, y: -1.2 },           // her, crying
                    { zoom: 2.2, x: 10, y: -0.8 },            // the officer at the door
                    { zoom: 1.1, x: 20, y: -1.5 },            // the yard and the Lada
                ];
                const cuts = [[1.4, 0.6], [4.4, 0.7]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const pink = '#e0a0c0', skin = '#e6e0d0', wood = '#8f7c50', khaki = '#7e8c6a', lada = '#c9c0a0';

                // the kitchen: ceiling, the wall with the door, the table with the bottle on it
                text('_'.repeat(24), -7, -4.5, '#666');
                for (let y = -4; y <= -3; y++) text('|', 17, y, '#666');
                const doorOpen = t > 2.6;
                if (!doorOpen) for (let y = -2; y <= 2; y++) text('▐', 17, y, wood);
                else for (let y = -2; y <= 2; y++) text('/', 17.4 + (y + 2) * 0.25, y, wood);
                text('_________', 5, 0.3, wood);
                text('|       |', 5, 1.2, wood);
                text('|       |', 5, 2.1, wood);
                text('[В]', 7.5, -0.45, '#9fc8e8');
                text('u', 10.5, -0.3, '#9fc8e8');
                text('_'.repeat(90), -20, 2.6, '#555');

                // the yard: a brand-new Lada with a big red bow on the roof
                const lx = 31;
                text('    ______', lx, -1, lada);
                text('  _/(    )\\_____', lx, 0, lada);
                text(' |___ЛАДА______|', lx, 1, lada);
                text('   (O)      (O)', lx, 2, '#999');
                text('>❀<', lx + 5.5, -1.9, '#e03030');

                // the officer at the door: a medal, the news, then the gift
                if (doorOpen) {
                    const pointing = t > 4.6 && t < 5.8;
                    text(' _▄_ ', 19, -2, khaki);
                    text('(ò_ó)', 19, -1, skin);
                    text(pointing ? '/|█|—' : '/|█|\\', 19, 0, khaki);
                    text(' |_| ', 19, 1, khaki);
                    text(' / \\ ', 19, 2, khaki);
                    if (t > 3.5 && t < 5.8) text('✪', 18, -0.3, '#f2d34a');
                }
                if (t > 2.2 && t < 2.7) text('*KOP KOP*', 15, -5.4, '#f2d34a');

                // her: at the table, then off out of the door to the car, arms wide
                const run = span(5.8, 6.6);
                const mx = 25.5 * run;
                const hugging = t > 6.6;
                const face = hugging ? '(^3^)' : t > 5.7 ? '(^o^)' : t > 5.3 ? '(°o°)' : t > 2.9 ? '(;_;)' : '(=_=)';
                text(',~~~,', mx, -2, pink);
                text(face, mx, -1, skin);
                text(t > 5.7 ? '\\|▼|/' : '/|▼|\\', mx, 0, pink);
                if (t > 5.7) text(run > 0 && run < 1 && Math.floor(t * 10) % 2 ? ' |\\  ' : ' / \\ ', mx, 1, pink);
                if (t < 2.9) {
                    // tears
                    for (let i = 0; i < 2; i++) text('\'', 1 + i * 2.6, -0.6 + ((t * 1.5 + i * 0.5) % 1) * 1.2, '#9fc8e8');
                }
                if (hugging) for (let i = 0; i < 3; i++) {
                    const k = (t * 0.8 + i / 3) % 1;
                    text('♥', mx + 1 + i * 1.8 + Math.sin(k * 6) * 0.4, -2.4 - k * 2.5, '#e05080');
                }

                // the talk
                if (t > 0.2 && t < 1.6) {
                    cam.font(0.5);
                    text('"Vanjuska... missä sä..."', -2.5, -2.9, '#ddd');
                    if (Math.floor(t * 3) % 2) text('*hik*', 6, -1.9, '#f2d34a');
                    cam.font();
                }
                cam.font(0.7);
                if (t > 2.9 && t < 4.4) text('"Poikanne kaatui sankarina."', 6, -3.6, '#ddd');
                if (t > 4.6 && t < 5.8) text('"Ja tässä korvaus."', 17, -3.6, '#ddd');
                cam.font();
                if (t > 5.7 && t < 6.6) text('"LADA!!!"', mx - 1, -3.6, '#f2d34a');
            },
        },
        {
            who: 'SEKÄ', name: 'LAPSI', line: '"Ei saa koskea! ÄITIII!"',
            color: '#f0d080',
            dur: 6.8 + READ_S,
            // close on him screaming; pull back: what he clutches is a toy tank with a Z on it;
            // pull back: his whole kindergarten group marches in, in uniform with toy rifles, the
            // teacher blows her whistle and they all march on the spot: the Yunarmiya
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 5.0, x: 2.5, y: -1.3 },           // his face
                    { zoom: 2.6, x: 3, y: -0.3 },             // the toy tank
                    { zoom: 1.25, x: -2.5, y: -1.5 },         // the whole group
                ];
                const cuts = [[1.4, 0.6], [3.2, 0.7]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', green = '#6a7a4a', pink = '#e0a0c0';
                const marching = t > 5.1;
                const step = (i) => marching && Math.floor(t * 4 + i) % 2;

                text('_'.repeat(90), -45, 1.7, '#555');
                // the kindergarten's banner and its flag
                cam.font(0.9);
                text('ДЕТСКИЙ САД № 3', -27, -6, '#f2d34a');
                cam.font();
                for (let y = -6; y <= 1; y++) text('|', -32, y, '#8f7c50');
                text('▐Z▌', -31.6, -6, '#fff');

                // a little soldier: cap, uniform, toy rifle on the shoulder
                const soldier = (x, i, rifle = true) => {
                    text(' ▄▄▄', x, -2, green);
                    text('(o.o)', x, -1, skin);
                    text(rifle ? ' |▓|╱' : '/|▓|\\', x, 0, green);
                    text(step(i) ? ' |\\ ' : ' / \\', x, 1, green);
                };
                // the group marches in from the left and lines up beside him
                const come = span(3.2, 4.6);
                [-30, -24, -18, -12, -6].forEach((x, i) => soldier(x - 26 * (1 - come), i));

                // him, clutching the tank
                text(' ▄▄▄', 0, -2, green);
                text(t > 5.1 ? '(ò.ó)' : '(O.O)', 0, -1, skin);
                text('/|▓|\\', 0, 0, green);
                text(step(5) ? ' |\\ ' : ' / \\', 0, 1, green);
                if (t > 1.3) {   // kept out of the close-up, under the caption bar
                    text('[Z]═', 3.6, 0.8, '#8a9a5a');
                    text('ooo', 3.7, 1.35, '#666');
                }

                // the teacher, with her whistle
                text(' _@_', 20, -2, '#a08060');
                text('(ò_ó)', 20, -1, skin);
                text('-•', 18.6, -0.8, '#ccc');
                text('/|▼|\\', 20, 0, pink);
                text(' / \\', 20, 1, pink);

                if (t > 0.2 && t < 1.6) {
                    cam.font(0.4);
                    text('"Ei saa koskea! ÄITIII!"', -1.6, -2.3, '#fff');
                    cam.font();
                }
                if (t > 4.6 && t < 5.2) text('*PRIIIT!*', 16, -3.6, '#f2d34a');
                if (t > 5.2) text(Math.floor(t * 2) % 2 ? '"Raz!"' : '"Dva!"', 18, -3.6, '#ddd');
            },
        },
        {
            who: 'SEKÄ', name: 'DIMA, TAKSIKUSKI', line: 'Navigaattori näyttää Kiovaa. Taas.',
            color: '#f2d34a',
            dur: 8.2 + READ_S,
            // close on Dima at the wheel while a navigator's voice slides in from the side; pull
            // back: the "navigator" is a soldier jogging beside the taxi with a paper map; pull
            // back: a whole tank column follows the taxi, and nobody knows how far Kyiv is
            render(ctx, W, H, t, fs) {
                const shots = [
                    { zoom: 4.5, x: 15, y: -1.2 },            // Dima's face in the window
                    { zoom: 1.7, x: 19, y: -1 },              // the taxi and its navigator
                    { zoom: 0.85, x: -3, y: -1.8 },           // the column behind them
                ];
                const cuts = [[2.6, 0.7], [5.3, 0.8]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', yellow = '#f2d34a', green = '#6a7a4a', voice = '#8fd0ff';
                const bob = Math.sin(t * 13) * 0.04;
                const turn = Math.floor(t * 8) % 2;

                // the road rolls by under the column
                text('_'.repeat(160), -100, 1.7, '#555');
                for (let i = 0; i < 24; i++) {
                    const x = ((i * 7.3 - t * 9) % 168 + 168) % 168 - 100;
                    text(i % 3 ? '.' : ',', x, 2.1, '#666');
                }

                // the column: Z tanks, nose to tail, as far as the eye can see
                [-14, -28, -42, -56, -70].forEach((x) => {
                    text('   ▄█Z█▄═══', x, -1 + bob, '#7a8a5a');
                    text(' ▐████████▌', x, 0 + bob, '#5a6a3a');
                    text(turn ? ' (oOoOoOoO)' : ' (OoOoOoOo)', x, 1, '#666');
                });

                // the taxi, Dima at the wheel
                text('      _[TAXI]______', 0, -2 + bob, yellow);
                text('  ___/ [  ] |', 0, -1 + bob, yellow);
                text(t > 4.8 ? '(ಠ_ಠ)' : '(-_-)', 12.6, -1 + bob, skin);
                text('\\____', 19, -1 + bob, yellow);
                text(' |______________________|', 0, 0 + bob, yellow);
                if (t > 2.4) text(turn ? '   (+)              (+)' : '   (×)              (×)', 0, 1, '#aaa');   // not under the caption in the close-up

                // the navigator: a soldier jogging alongside with a paper map
                const flip = t > 4.5;
                text(' ▄▄▄', 26, -2.2, green);
                text(t > 3.3 ? '(ºOº)' : '(º_º)', 26, -1.2, skin);
                text(flip ? '[ВƎИʞ]' : '[КИЕВ]', 31.2, -1.2, '#e8e0c0');
                text('/|▓|‾', 26, -0.2, green);
                text(turn ? ' / |' : ' | \\', 26, 0.8, green);

                // close-up: the "navigator's" voice slides in from the right edge and fades
                const say = (str, a, b, row) => {
                    if (t < a || t > b) return;
                    ctx.font = `${Math.round(fs * 1.05)}px ui-monospace, "Cascadia Code", Menlo, monospace`;
                    const w = ctx.measureText(str).width;
                    ctx.globalAlpha = Math.min(1, (b - t) / 0.35);
                    ctx.fillStyle = voice;
                    ctx.fillText(str, W - 24 - w * ease((t - a) / 0.45), H * row);
                    ctx.globalAlpha = 1;
                    cam.font();
                };
                say('◁))  Käänny vasemmalle 300 metrin päästä.', 0.2, 1.5, 0.14);
                say('◁))  Perillä Kiovassa: 3 päivää.', 1.2, 2.7, 0.24);
                if (t > 1.9 && t < 2.7) {
                    cam.font(0.35);
                    text('"Taas..."', 12.4, -2.9, '#ddd');
                    cam.font();
                }

                // the reveal: the voice was him all along
                cam.font(0.6);
                if (t > 3.4 && t < 4.5) text('"Käänny vasemmalle!"', 15, -3.4, voice);
                if (t > 4.5 && t < 5.4) text('"...tai oikealle."', 17, -3.4, voice);
                cam.font();
                if (t > 6.0) {
                    cam.font(1.3);
                    const dots = '.'.repeat(1 + Math.floor(t * 3) % 3);
                    text(t > 6.6 ? `"Perillä: lasketaan${dots}"` : '"Perillä:"', 6, -4.4, voice);
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'ROTTA', line: 'Pakotteet eivät pure. Parmesaani maistuu.',
            color: '#a89080',
            dur: 7.6 + READ_S,
            // close on a rat nibbling cheese; pull back: it sits on a gilded throne-chair, a
            // napkin on, and the cheese is sanctioned Parmesan on a gold plate; pull back: the
            // ridiculously long Kremlin table, and at its far end Vova wishes it bon appétit
            render(ctx, W, H, t, fs) {
                const shots = [
                    { zoom: 5.0, x: 13.5, y: 0.5 },          // the rat at its cheese
                    { zoom: 2.2, x: 12, y: -1.2 },           // the throne, the plate, the label
                    { zoom: 0.7, x: -25, y: -1.5 },          // the whole table, and Vova
                ];
                const cuts = [[2.0, 0.7], [4.2, 0.9]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const gold = '#e0b84a', wood = '#c8b27a', rat = '#a89080';
                const chew = Math.floor(t * 7) % 2;

                // chandeliers along the hall
                for (let x = -50; x <= 10; x += 20) {
                    text('  |', x, -7, '#8f7c50');
                    text('\\╋╋/', x, -6, gold);
                    text(' ▽▽', x, -5.3, '#fff3c0');
                }

                // the table, as long as ever
                const L = 72, top = '_'.repeat(L);
                text(' ' + top, -60, 1, wood);
                text('|' + top + '|', -60, 2, wood);
                for (let lx = -59; lx < 12; lx += 35) { text('||', lx, 3, '#8f7c50'); text('||', lx, 4, '#8f7c50'); }
                text('||', 10, 3, '#8f7c50'); text('||', 10, 4, '#8f7c50');

                // the cheese on its gold plate, a bite smaller every moment
                const left = Math.max(2, 5 - Math.floor(t / 0.6));
                text('◢' + '█'.repeat(left), 12 - left, 0.35, '#f0d060');
                text('‿‿‿‿‿‿‿', 6.5, 0.8, gold);
                if (t > 1.9) {   // the label stays out of the close-up
                    text('|', 10.6, -0.5, '#ccc');
                    cam.font(0.5);
                    text('PARMESAN · ITALIA', 6.5, -1.4, '#fff');
                    cam.font();
                }
                if (t > 3.0) {
                    const thump = 1 + 0.6 * Math.max(0, 1 - (t - 3.0) / 0.2);
                    cam.font(0.5 * thump);
                    text('✕ PAKOTELISTALLA', 6.8 - 4 * (thump - 1), -2.3, '#e05050');
                    cam.font();
                    if (t < 3.5) text('*LÄTS*', 2, -3.3, '#ddd');
                }

                // the rat on its throne, napkin on, nibbling
                text(chew && t < 4.2 ? '<:^__)~~' : '<·^__)~~', 12.6, 0.35, rat);
                text('▼', 14, 0.75, '#fff');
                text('♛', 18.6, -2.4, gold);
                for (let y = -1.6; y <= 1; y += 1) text('║', 19, y, gold);
                text('╘════╛', 13.6, 1.2, gold);
                text('|    |', 13.6, 2.2, '#8f7c50');
                text('|    |', 13.6, 3.2, '#8f7c50');
                if (t < 2.0) {
                    cam.font(0.3);
                    if (chew) text('·', 11.4 - (t * 3 % 1), -0.1 + (t * 5 % 1) * 0.5, '#f0d060');
                    text('*nom nom nom*', 11.5, -0.9, '#ddd');
                    cam.font();
                }
                if (t > 3.7 && t < 4.4) text('*röyh*', 15.5, -1.2, '#ddd');

                // at the far end: Vova, keeping his distance from this one too
                const vx = -66;
                text(' ___ ', vx, -1, '#e6e0d0');
                text('(ò_ó)', vx, 0, '#e6e0d0');
                text('/|=|\\', vx, 1, '#4b5d8a');
                text(' |_|_', vx, 2, '#4b5d8a');
                text(' |  |', vx, 3, '#6b5a3a');
                if (t > 5.4) {
                    cam.font(1.2);
                    text('"Hyvää ruokahalua, toveri."', vx - 1, -2.6, '#ddd');
                    cam.font();
                }
                if (t > 6.6) text('*viik!*', 16, -3.6, '#ddd');
            },
        },
        {
            who: 'SEKÄ', name: 'PANKKIRYÖSTÄJÄ', line: '"KAIKKI MAAHAN! *hik*"',
            color: '#c0a060',
            dur: 8.6 + READ_S,
            // close on him shouting; pull back: he is shouting at a vault door with nobody around,
            // lights the dynamite, KABOOM; pull back: the vault is empty, an oligarch's note on
            // the wall, and two earlier robbers already lying on the floor. He lies down too
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 4.5, x: 0.5, y: -1.0 },          // his face
                    { zoom: 1.7, x: 7, y: -1.2 },            // him and the vault door
                    { zoom: 0.9, x: 22, y: -1.4 },           // the vault, inside
                ];
                const cuts = [[1.5, 0.7], [3.6, 0.8]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const mask = '#8a8a8a', cloth = '#6a6a8a', steel = '#9aa4b0', wall = '#5a5a60';
                const blown = t > 2.9;

                text('_'.repeat(90), -30, 1.7, '#555');

                // the vault: a steel wall with its round door, the room behind it
                for (let y = -5; y <= 1; y++) {
                    if (!blown || y < -4) text('█', 8.5, y, wall);   // blown open: just the lintel is left
                    if (!blown) text('█', 20.5, y, wall);
                    text('█', 58, y, wall);
                }
                text('▀'.repeat(50), 8.5, -5.6, wall);
                if (!blown) {
                    text('  .-‾‾‾-.', 9.5, -4, steel);
                    text(' /  \\ /  \\', 9.5, -3, steel);
                    text('|  --O--  |', 9.5, -2, steel);
                    text(' \\  / \\  /', 9.5, -1, steel);
                    text("  '-...-'", 9.5, 0, steel);
                    text('|||', 13, 0.9, '#d04040');
                    // the fuse runs back to him, burning down
                    const burnt = span(1.8, 2.9);
                    if (t > 1.5) text('~'.repeat(Math.max(0, Math.round(8 * (1 - burnt)))), 13 - 8 * (1 - burnt), 1.3, '#c8b27a');
                    if (t > 1.8 && Math.floor(t * 12) % 2) text('*', 12.6 - 8 * (1 - burnt), 1.0, '#ffb040');
                    if (t > 1.9) text('*tsss*', 10.6 - 8 * (1 - burnt), 0.1, '#ddd');
                } else {
                    text('(==--O--==)', 0, 1.3, steel);      // the door, blown flat
                }
                if (blown && t < 4.4) {
                    ctx.globalAlpha = Math.max(0, 1 - (t - 2.9) / 1.5);
                    text(' ░▒▓▒░', 10, -3.4, '#aaa'); text('▒▓█▓▒░', 9.5, -1.6, '#999'); text(' ░▒░', 11, 0.2, '#888');
                    ctx.globalAlpha = 1;
                }
                if (blown && t < 3.6) {
                    cam.font(1.2);
                    text('*KABOOM!*', 4, -6.8, '#ffb040');
                    cam.font();
                }

                // inside: empty shelves, the note, and the two who got here first, already down
                if (t > 2.9) {
                    for (let y = -4; y <= 0; y += 2) text('|______|', 48.5, y, '#8f7c50');
                    cam.font(0.8);
                    text('RAHAT SIIRRETTY LONTOOSEEN.', 23, -3.8, '#f2efe0');
                    text('   – Oligarkki', 23, -2.8, '#f2efe0');
                    cam.font();
                    const lying = (x) => { text('[-_-]', x, 1, mask); text('▓▓▓≡', x + 5, 1, cloth); };
                    lying(24); lying(33);
                    if (t > 4.6 && t < 6.1) text('"Ollaan jo maassa."', 22, -0.6, '#ddd');
                    if (t > 6.1 && t < 7.4) text('"Myöhästyit, brat."', 25, -0.6, '#ddd');
                }

                // him: from the doorway into the vault, where he reads the note and lies down too
                const rx = -6 * span(2.2, 2.6) + 50 * span(4.6, 6.4);
                if (t < 7.6) {
                    text(' ▄▄▄', rx, -2, mask);
                    text(t > 2.3 && t < 3.2 ? '[>_<]' : t > 6.2 ? '[._.]' : '[ò_ó]', rx, -1, mask);
                    text(t < 1.6 ? '/|▓|═╦' : t > 2.3 && t < 3.2 ? '\\|▓|/' : '/|▓|\\', rx, 0, cloth);
                    text(t > 4.6 && t < 6.4 && Math.floor(t * 6) % 2 ? ' |\\' : ' / \\', rx, 1, cloth);
                } else {
                    text('[-_-]', rx - 2, 1, mask); text('▓▓▓≡', rx + 3, 1, cloth);
                }
                if (t > 0.2 && t < 1.5) {
                    cam.font(0.4);
                    text('"KAIKKI MAAHAN! *hik*"', -2.6, -2.4, '#fff');
                    cam.font();
                }
                if (t > 6.6 && t < 7.6) text('"...*hik*"', rx - 1, -3, '#ddd');
                if (t > 7.6) text('*zzz*', rx, -0.4, '#888');

                // the blast lights up the whole screen for a moment
                if (blown && t < 3.25) {
                    ctx.fillStyle = `rgba(255, 240, 200, ${0.8 * (1 - (t - 2.9) / 0.35)})`;
                    ctx.fillRect(0, 0, W, H);
                }
            },
        },
        {
            who: 'SEKÄ', name: 'VETERAANI', line: '"TANKIT!! DROONIT!!" Sokea, mutta kuulee askeleesi.',
            color: '#b0a080',
            dur: 7.4 + READ_S,
            // close on him shouting; pull back: medals cover him down to the ground and drag
            // behind him, clinking at every step (that is what he hears), all of them for the
            // same battle; pull back: a street banner of him, the hero, 25 years old. A pigeon lands
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 5.0, x: 2.5, y: -1.4 },          // his face
                    { zoom: 1.9, x: 2, y: -0.4 },            // the medals
                    { zoom: 0.9, x: 10, y: -3.2 },           // the banner over the street
                ];
                const cuts = [[1.8, 0.7], [4.6, 0.8]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', green = '#6a7a4a', gold = '#e0b84a', ribbon = '#e07a3a';
                const duck = t > 6.4;

                text('_'.repeat(100), -40, 1.7, '#555');

                // he shuffles forward, the cane tapping ahead
                const x = 1.8 * span(2.2, 4.4);
                const stepping = t > 2.2 && t < 4.4;
                const clink = stepping && Math.floor(t * 5) % 2;

                // the street banner of the hero, out of the earlier shots
                if (t > 4.5) {
                    for (let y = -5; y <= 1; y++) { text('|', 7, y, '#8f7c50'); text('|', 26, y, '#8f7c50'); }
                    text('╔══════════════════╗', 6.5, -10, '#ddd');
                    text('║    ГЕРОЙ  СВО    ║', 6.5, -9, '#ddd');
                    text('║      (o_o)       ║', 6.5, -8, '#ddd');
                    text('║   PAŠA, 25 v.    ║', 6.5, -7, '#ddd');
                    text('╚══════════════════╝', 6.5, -6, '#ddd');
                    text('ГЕРОЙ  СВО', 11.5, -9, '#e05050');
                    text('PAŠA, 25 v.', 10.5, -7, '#f2d34a');
                    // a pigeon flies in and lands on the banner
                    const land = span(5.6, 6.2);
                    text(land < 1 ? 'ᵛ>' : '>v<', 40 - 24 * land, -13 + 2.1 * land, '#bbb');
                    if (t > 6.2 && t < 6.9) text('*kurr*', 18, -12, '#ddd');
                }

                // the medals: a train of them dragging along the ground behind him
                for (let i = 0; i < 14; i++) {
                    const mx = x - 1.2 - i * 1.1;
                    text(i % 3 ? '◉' : '✪', mx, 1.45, gold);
                }
                // and hanging off him, all the way down
                const sway = clink ? 0.15 : -0.15;
                ['✪◉✪', '◉✪◉', '✪◉✪'].forEach((row, i) => text(row, x + 1 + sway * i, 0.1 + i * 0.5, gold));
                text('▼▼▼', x + 1, -0.35, ribbon);

                // him: side cap, dark glasses, a white cane
                const hy = duck ? 0.5 : 0;
                text(' ▄▄▄', x, -2 + hy, green);
                text(duck ? '(▀o▀)' : '(▀_▀)', x + (t < 1.8 ? (Math.floor(t * 6) % 2 ? 0.2 : -0.2) : 0), -1 + hy, skin);
                text('/   \\', x, -0.1 + hy, green);
                text(' / \\', x, 1, green);
                text('╲', x + 5, 0.5, '#eee');
                text('╲', x + 5.5, 1.2, '#eee');

                if (t > 0.2 && t < 1.8) {
                    cam.font(0.4);
                    text('"TANKIT!! DROONIT!!"', 0.2, -2.6, '#fff');
                    cam.font();
                }
                if (t > 0.6 && t < 1.8 && Math.floor(t * 4) % 2) {
                    cam.font(0.3);
                    text('*kilin*', 5.6, -0.7, '#ddd');
                    cam.font();
                }
                if (clink) { cam.font(0.7); text('*KILIN KILAN*', x - 13, 0.4, '#ddd'); cam.font(); }
                if (t > 3.0 && t < 3.6) text('"DROONIT!!"', x - 1, -3, '#fff');

                // every medal is for the same battle
                if (t > 3.4) {
                    cam.font(0.45);
                    ['BAHMUT 2023', 'BAHMUT 2023', 'BAHMUT 2023'].forEach((s, i) => text('← ' + s, x + 5, -0.7 + i * 0.55, '#f2efe0'));
                    cam.font();
                }

                if (t > 6.4) {
                    cam.font(1.2);
                    text('"DROONIT!!"', x - 3, -4.4, '#fff');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'SOTILAS', line: '"Rintamalle, svinya!" Tarvitsee miehiä. Sinut.',
            color: '#7e8c6a',
            dur: 8.8 + READ_S,
            // close on his face, all going to plan; pull back: an assault rifle in his hands;
            // pull back: he sits in an armoured personnel carrier; pull back: the carrier is on
            // fire with a track off, drones buzzing round, burning wrecks everywhere
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 5.0, x: 2.5, y: -1.1 },          // his face
                    { zoom: 3.2, x: 6, y: -0.6 },            // the rifle
                    { zoom: 1.2, x: 6, y: -1 },              // inside the carrier
                    { zoom: 0.65, x: 4, y: -4.5 },           // the battlefield
                ];
                const cuts = [[1.6, 0.6], [3.2, 0.7], [5.3, 1.0]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', green = '#6a7a4a', hull = '#5a6a3a';
                const fireCols = ['#ff7a2a', '#ffcc40', '#d03020'];
                const flames = (x, y, n, seed, size = 1) => {
                    cam.font(size);
                    for (let i = 0; i < n; i++) {
                        const f = Math.floor(t * 10 + i * 3 + seed) % 3;
                        text('▲ʌ^'[f], x + i * size * 0.7, y - (f === 1 ? 0.3 * size : 0), fireCols[(f + i) % 3]);
                    }
                    cam.font();
                };
                const smoke = (x, y, seed) => {
                    ctx.globalAlpha = 0.35;
                    for (let i = 0; i < 6; i++) {
                        const k = ((t * 0.6 + i / 6 + seed) % 1);
                        text('░▒░', x + k * 4, y - k * 12, '#888');
                    }
                    ctx.globalAlpha = 1;
                };
                const battle = t > 5.3;

                text('_'.repeat(200), -100, 3.1, '#555');

                // burning wrecks all round, out of every shot but the last
                if (battle) {
                    [-72, -44, 44, 68, 92].forEach((x, i) => {
                        text(i % 2 ? '  ▄▄█▄═══' : '       ▄▄', x, 0.8, '#4a4a3a');
                        text('▐███████▌', x, 1.8, '#3a3a2a');
                        text('(o o o o)', x, 2.6, '#555');
                        flames(x + 1, -0.3, 7, i * 5);
                        smoke(x + 2, -1.5, i * 0.37);
                    });
                    text('▄██▄', -52, 2.6, '#4a4a3a');    // a turret, blown clean off
                }

                // the carrier: drawn as a cutaway until the last pull-back closes its side
                if (t > 3.1) {
                    text('▄'.repeat(41), -14, -4.5, hull);
                    text('  ▄██▄', 0, -5.5, hull);
                    text('════════════', 6, -5.5, '#777');
                    for (let y = -4; y <= 1; y++) { text('█', -14.5, y, hull); text('█', 26.5, y, hull); }
                    text('▀'.repeat(41), -14, 2.0, hull);
                    // tracks, and in the last shot one of them is off and lying behind
                    if (battle) {
                        text('(◎◎◎◎◎◎◎◎◎◎◎◎◎  ◎◎◎', -12.5, 2.6, '#666');
                        text('▬▬▬▬▬▬▬▬', -28, 2.9, '#666');
                    } else {
                        text('(◎◎◎◎◎◎◎◎◎◎◎◎◎◎◎◎◎◎)', -12.5, 2.6, '#666');
                    }
                    // the crew: one praying, one asleep
                    text(' ▄▄▄', -12.5, -2, green);
                    text('(ú_ù)', -12.5, -1, skin);
                    text('/|▓|\\', -12.5, 0, green);
                    text(' | |', -12.5, 1, green);
                    text(' ▄▄▄', 20.5, -2, green);
                    text('(-_-)', 20.5, -1, skin);
                    text('/|▓|\\', 20.5, 0, green);
                    text(' | |', 20.5, 1, green);
                    if (t < 5.6) text('zz', 25.5, -2.4, '#888');
                    // a red warning light
                    if (Math.floor(t * 3) % 2) text('●', 6, -3.6, '#ff3030');
                }
                text('▀'.repeat(14), -1, 1.4, hull);   // the floor under him, in every shot

                // him, rifle in his hands
                text(' ▄▄▄', 0, -2, green);
                text(t > 4.3 && t < 5.3 ? '(ò_ó;)' : '(ò_ó)', 0, -1, skin);
                text('/|▓|', 0, 0, green);
                text('▄█▬▬═══', 4, 0, '#777');
                text('▌', 6, 0.6, '#777');
                text(' | |', 0, 1, green);

                // the side panel slides shut over the cutaway as the camera pulls back
                if (battle) {
                    ctx.globalAlpha = span(5.4, 6.0);
                    ctx.fillStyle = '#4a5a30';
                    ctx.fillRect(cam.sx(-14.5), cam.sy(-4.4), cam.sx(27.5) - cam.sx(-14.5), cam.sy(1.6) - cam.sy(-4.4));
                    cam.font(2);
                    text('Z', 3, -1.5, '#fff');
                    cam.font();
                    ctx.globalAlpha = 1;
                    flames(-12, -5.6, 7, 1, 2.2);
                    flames(13, -5.6, 8, 7, 2.2);
                    smoke(-6, -7, 0.1);
                    smoke(16, -7, 0.6);
                    // drones buzzing round it
                    for (let i = 0; i < 5; i++) {
                        const a = t * (1.3 + i * 0.2) + i * 1.9;
                        const dx = 6 + Math.cos(a) * (14 + i * 6), dy = -13 + Math.sin(a * 1.4) * 4 - i * 1.5;
                        text('-╳-', dx, dy, '#ccc');
                        if (Math.floor(t * 6 + i) % 2) text('bzz', dx + 3, dy - 0.8, '#888');
                    }
                }

                cam.font(0.4);
                if (t > 0.2 && t < 1.6) text('"Kaikki sujuu suunnitelman mukaan!"', -4.6, -2.5, '#fff');
                cam.font();
                if (t > 2.1 && t < 3.1) {
                    cam.font(0.5);
                    text('*klik-klak*', 6.5, -1.4, '#ddd');
                    cam.font();
                }
                if (t > 4.0 && t < 5.3) text('"Miksi täällä on näin kuuma?"', -14, -3.3, '#ddd');
                if (t > 6.6) {
                    cam.font(1.8);
                    text('"...Tarvitaan lisää miehiä."', -22, -15, '#fff');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'ILMATORJUNTAMIES', line: '"Kohde havaittu!" Sirpaleita sataa taivaalta.',
            color: '#8a9a6a',
            dur: 8.6 + READ_S,
            // close on him shouting and firing; pull back: the target is one of our own planes,
            // red star and all, and the pilot bails out screaming; pull back: the tally board by
            // the gun, all own planes and no enemies, and the pilot lands next to him. Again.
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                // the next kill breaks up, and one burning shard of it falls away: the camera
                // follows it down into the refinery scene
                const sd = Math.max(0, t - 8.7);
                const shardX = 34 + 3.5 * sd, shardY = -19.5 + 4 * sd * sd;
                const shots = [
                    { zoom: 5.0, x: 2.5, y: -1.2 },          // his face
                    { zoom: 1.3, x: 16, y: -5.5 },           // the gun and the sky
                    { zoom: 0.55, x: 0, y: -4 },             // the tally board, the wrecks
                    { zoom: 3.5, x: shardX, y: shardY },     // after the falling shard
                ];
                const cuts = [[1.6, 0.8], [5.0, 1.0], [9.2, 0.8]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', green = '#8a9a6a', steel = '#8a8a8a', red = '#e03030';
                const fireCols = ['#ff7a2a', '#ffcc40', '#d03020'];
                const flames = (x, y, n, seed) => {
                    for (let i = 0; i < n; i++) {
                        const f = Math.floor(t * 10 + i * 3 + seed) % 3;
                        text('▲ʌ^'[f], x + i, y - (f === 1 ? 0.3 : 0), fireCols[(f + i) % 3]);
                    }
                };
                const smoke = (x, y, seed) => {
                    ctx.globalAlpha = 0.35;
                    for (let i = 0; i < 6; i++) {
                        const k = ((t * 0.6 + i / 6 + seed) % 1);
                        text('░▒░', x + k * 4, y - k * 10, '#888');
                    }
                    ctx.globalAlpha = 1;
                };
                const firing = (t > 0.8 && t < 3.0) || (t > 7.9 && t < 8.8);
                const hit = t > 3.0;

                text('_'.repeat(220), -110, 1.7, '#555');

                // wrecks of earlier "kills", out of every shot but the last
                if (t > 5.0) {
                    text('▄█★█▄▄', 53, 1.2, '#4a4a3a');
                    text('★', 55, 1.2, red);
                    flames(53, 0.2, 5, 3);
                    smoke(54, -1, 0.4);
                }

                // the tally board on its two legs, far off to the left
                if (t > 5.0) {
                    for (let y = -2; y <= 1; y++) { text('║', -50, y, '#8f7c50'); text('║', -27, y, '#8f7c50'); }
                    cam.font(2.0);
                    text('╔════════════════╗', -56, -9, '#ddd');
                    text('║                ║', -56, -7, '#ddd');
                    text('║                ║', -56, -5, '#ddd');
                    text('╚════════════════╝', -56, -3, '#ddd');
                    const stars = t > 6.4 ? 8 : 7;
                    text('OMAT:', -52, -7, '#f2efe0');
                    text('★'.repeat(stars), -40, -7, red);
                    text('VIHOLLISET: 0', -52, -5, '#f2efe0');
                    cam.font();
                    if (t > 6.4 && t < 7.4) {
                        cam.font(1.6);
                        text('+1  *sutt*', -22, -8 - 1.5 * (t - 6.4), red);
                        cam.font();
                    }
                }

                // the plane: ours, red star on the side. Flies in, gets hit, goes down to the right
                if (t > 1.6 && t < 4.7) {
                    if (!hit) {
                        const px = 60 - 24 * (t - 1.6);
                        text('◄▄▄▄██ ██▄▄▄◣', px, -10, '#aab4c0');
                        text('★', px + 6, -10, red);
                        text('  ╲╲    ╲╲', px + 1, -9, '#aab4c0');
                    } else {
                        const k = Math.min(1, (t - 3.0) / 1.6);
                        const px = 26 + 21 * k, py = -10 + 11 * k * k;
                        const spin = Math.floor(t * 8) % 2;
                        text(spin ? '▄█★█▄' : '▀█★█▀', px, py, '#aab4c0');
                        text('★', px + 2, py, red);
                        flames(px - 2, py - 0.6, 3, 0);
                        // a smoke trail behind it
                        ctx.globalAlpha = 0.4;
                        for (let i = 1; i < 6; i++) {
                            const kk = Math.max(0, k - i * 0.06);
                            text('▒', 26 + 21 * kk, -10 + 11 * kk * kk - 0.5, '#888');
                        }
                        ctx.globalAlpha = 1;
                    }
                }
                if (t > 3.0 && t < 3.8) {
                    text('✸', 27, -10, '#ffcc40');
                    cam.font(1.0);
                    text('*BADABUM!*', 12, -11.2, '#ffb040');
                    cam.font();
                }
                // and there it lies, out of the second shot
                if (t > 4.7) {
                    text('▄█★▄▄█▀', 45, 1.2, '#4a4a3a');
                    text('★', 47, 1.2, red);
                    flames(45, 0.2, 6, 9);
                    smoke(46, -1, 0.8);
                }

                // the pilot: ejects, drifts down on his parachute, lands next to the gun
                if (t > 3.15) {
                    const k = Math.min(1, (t - 3.15) / 3.85);
                    const landed = t > 7.0;
                    const px = 25 - 12 * k + (landed ? 0 : Math.sin(t * 2) * 0.6);
                    const fy = -9.5 + 8.5 * k;
                    if (!landed) {
                        text('▄▀▀▀▀▀▄', px - 1, fy - 2, '#eee');
                        text(' \\ | /', px - 1, fy - 1, '#aaa');
                    } else {
                        text('▁▂▃▂▁', px + 4, 1.2, '#eee');
                    }
                    text(t > 7.9 ? '(°o°)' : '(ಠ_ಠ)', px, fy, skin);
                    text(' /█\\', px, fy + 1, '#7a8aa0');
                    text(' / \\', px, fy + 2, '#7a8aa0');
                    if (t > 3.4 && t < 4.4) text('"BLYAT!"', px + 5, fy - 2.5, '#fff');
                    if (t > 4.4 && t < 5.6) text('"SVOI!! OMA KONE!!"', px + 5, fy - 2.5, '#fff');
                    if (t > 7.1 && t < 7.9) {
                        cam.font(1.8);
                        text('"Taas sinä, Vasja?!"', px - 4, fy - 4.5, '#fff');
                        cam.font();
                    }
                }

                // the gun: twin barrels pointing up and right
                for (let i = 0; i < 7; i++) text('╱╱', 7 + i, -0.6 - i, steel);
                text('▐█', 5.6, -1.0, green);
                text('▄██▄', 5, 0.3, steel);
                text('/  \\', 5, 1, steel);
                if (firing && Math.floor(t * 14) % 2) text('✶', 14.2, -7.6, '#ffcc40');
                // spent casings popping out
                if (firing) {
                    for (let i = 0; i < 3; i++) {
                        const k = (t * 2.5 + i / 3) % 1;
                        text('▮', 6 + k * 2.5, -0.4 + k * k * 1.6, '#c8a040');
                    }
                }
                // tracers up to the plane
                if (t > 1.6 && t < 3.0) {
                    const px = 60 - 24 * (t - 1.6) + 6;
                    for (let i = 0; i < 8; i++) {
                        const k = ((t * 3 + i / 8) % 1);
                        text('·', 14.5 + (px - 14.5) * k, -7.8 + (-10 + 7.8) * k, '#ffd24a');
                    }
                }
                if (t > 7.9 && t < 8.7) {
                    for (let i = 0; i < 8; i++) {
                        const k = ((t * 3 + i / 8) % 1);
                        text('·', 14.5 + 20 * k, -7.8 - 12 * k, '#ffd24a');
                    }
                }
                if (t > 8.7 && t < 9.2) text('✸', 33.5, -19.5, '#ffcc40');
                if (t > 8.7) {
                    ctx.globalAlpha = 0.4;
                    for (let i = 1; i < 6; i++) text('▒', shardX - 0.3 * i, shardY - 0.9 * i, '#888');
                    ctx.globalAlpha = 1;
                    flames(shardX - 1.5, shardY - 0.8, 3, 4);
                    text('▄█ ▀', shardX - 1.5, shardY, '#aab4c0');
                    text('★', shardX, shardY, red);
                }

                // shrapnel raining down after the hit
                if (t > 3.4 && t < 5.4) {
                    for (let i = 0; i < 10; i++) {
                        const k = ((t - 3.4) * 0.8 + i * 0.13) % 1;
                        text('▪¬▫'[i % 3], -6 + ((i * 7) % 24), -14 + k * 15, '#aaa');
                    }
                }

                // him
                const shake = firing ? (Math.floor(t * 14) % 2 ? 0.12 : -0.12) : 0;
                const cheer = t > 3.1 && t < 4.4;
                const dazed = t > 4.4 && t < 5.2;
                text(' ▄▄▄', shake, -2, green);
                text(dazed ? '(@_@)' : cheer ? '(^o^)' : '(ò_ó)', shake, -1, skin);
                text(cheer ? '\\|▓|/' : '/|▓|═', shake, 0, green);
                text(' | |', 0, 1, green);

                if (t > 0.2 && t < 1.6) {
                    cam.font(0.4);
                    text('"Kohde havaittu!"', -2.2, -2.6, '#fff');
                    cam.font();
                }
                if (t > 0.9 && t < 1.6) {
                    cam.font(0.35);
                    text('*TRA-TA-TA*', 4.6, -2.0, '#ffd24a');
                    cam.font();
                }
                if (cheer) text('"Torjuttu!"', -3, -3.2, '#fff');
                if (t > 4.4 && t < 5.2) text('*KLONK*', -1, -3.4, '#ddd');
                if (t > 7.9) {
                    cam.font(2.2);
                    text('"Kohde havaittu!"', -18, -12, '#fff');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'НПЗ · ÖLJYNJALOSTAMO', line: 'Palaa taas. Savu on "länsimaista propagandaa".',
            color: '#c9a96a',
            dur: 9.4 + READ_S,
            noFlash: true,           // it carries on straight from the anti-aircraft gunner's shard
            // follow the burning shard down onto a refinery that is not on fire yet; it hits, and
            // the place goes up while two workers watch; zoom in: they are roasting sausages on a
            // tiny campfire that will not catch; pull back: the whole huge refinery is ablaze round them
            render(ctx, W, H, t, fs) {
                // the shard and a second piece of the plane, coming down
                const fall = Math.min(t, 2.33);
                const shardX = 8 + 3.5 * (fall - 2.33), shardY = -8.6 + 12 * (fall - 2.33);
                const shots = [
                    { zoom: 3.5, x: shardX, y: shardY },     // after the shard
                    { zoom: 0.8, x: 6, y: -4.5 },            // the refinery
                    { zoom: 2.6, x: -17.5, y: 0 },           // the workers' campfire
                    { zoom: 0.6, x: 8, y: -6 },              // all of it, burning
                ];
                const cuts = [[0.9, 0.9], [4.3, 0.4], [6.4, 1.2]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', steel = '#8a8a8a', red = '#e03030', hat = '#e0c040', vest = '#e07a30';
                const fireCols = ['#ff7a2a', '#ffcc40', '#d03020'];
                const flames = (x, y, n, seed) => {
                    for (let i = 0; i < n; i++) {
                        const f = Math.floor(t * 10 + i * 3 + seed) % 3;
                        text('▲ʌ^'[f], x + i, y - (f === 1 ? 0.3 : 0), fireCols[(f + i) % 3]);
                    }
                };
                // a fire that grows taller the longer it has burnt
                const blaze = (x, y, n, since, seed, max = 3) => {
                    if (t < since) return;
                    const h = Math.min(max, 1 + Math.floor((t - since) * 1.5));
                    for (let r = 0; r < h && n - r > 0; r++) flames(x + r * 0.5, y - r, n - r, seed + r);
                };
                const smoke = (x, y, seed, tall = 10) => {
                    ctx.globalAlpha = 0.35;
                    for (let i = 0; i < 8; i++) {
                        const k = ((t * 0.5 + i / 8 + seed) % 1);
                        text('░▒▓▒░', x + k * 5, y - k * tall, '#777');
                    }
                    ctx.globalAlpha = 1;
                };
                const huge = t > 6.4;          // in the last shot everything is burning hard

                // a few specks of sky, so the fall reads as a fall
                ctx.globalAlpha = 0.5;
                for (let i = 0; i < 14; i++) text('·', -6 + ((i * 7) % 20), -40 + i * 2.1, '#556');
                ctx.globalAlpha = 1;

                text('_'.repeat(260), -130, 1.7, '#555');

                // the refinery. Each part: where it stands, how it looks and where its fire sits
                const ignite = (x) => 2.35 + Math.abs(x - 8) * 0.07;
                const tower = (x, top, far) => {
                    text(' ▄█▄', x, top - 0.4, steel);
                    for (let y = Math.ceil(top); y <= 1; y++) text(' ║█║', x, y, steel);
                    blaze(x - (huge ? 1 : 0), top - 1.2, huge ? 6 : 4, far ? 4.2 : ignite(x), x, huge ? 6 : 3);
                    if (t > (far ? 4.2 : ignite(x)) + 0.5) smoke(x + 1, top - 3, x * 0.13, huge ? 18 : 8);
                };
                const tank = (x, far) => {
                    text('╭─────╮', x, -1, steel);
                    text('│ ▓▓▓ │', x, 0, '#7a7a7a');
                    text('╰─────╯', x, 1, steel);
                    blaze(x - (huge ? 1 : 0), -2, huge ? 9 : 7, far ? 4.2 : ignite(x), x, huge ? 6 : 2);
                };
                const stack = (x, far) => {
                    for (let y = -11; y <= 1; y++) text('▐█▌', x, y, y % 2 ? '#c84040' : '#ddd');
                    if (t > (far ? 4.2 : ignite(x))) smoke(x, -13, x * 0.07, huge ? 16 : 6);
                };
                text('═'.repeat(34), 1, -3, '#777');
                tower(0, -10); tower(5, -8); stack(11); tank(15);
                text('┌──────────┐', 24, -4, '#9a8a6a');
                text('│  Н П З   │', 24, -3, '#9a8a6a');
                text('│ ▒▒   ▒▒  │', 24, -2, '#9a8a6a');
                text('│ ▒▒   ▒▒  │', 24, -1, '#9a8a6a');
                text('└──────────┘', 24, 0.7, '#9a8a6a');
                text('Н П З', 27, -3, red);
                blaze(huge ? 23 : 24, -5, huge ? 13 : 11, ignite(30), 7, huge ? 7 : 2);
                if (huge) [-46, -20, 37, 53].forEach((x, i) => blaze(x, -4, 6, 6.4, i * 3, 4));   // the pipes too
                // the rest of it, out of every shot but the last
                if (t > 6.4) {
                    text('═'.repeat(130), -55, -3, '#777');
                    tank(-50, true); tank(-41, true); tower(-33, -9, true); tower(-28, -7, true);
                    tower(44, -10, true); tank(49, true); tank(57, true); stack(65, true); tower(69, -9, true);
                }

                // the shard: falls, hits the top of the second tower
                if (t < 2.33) {
                    ctx.globalAlpha = 0.4;
                    for (let i = 1; i < 6; i++) text('▒', shardX - 0.3 * i, shardY - 0.9 * i, '#888');
                    ctx.globalAlpha = 1;
                    flames(shardX - 1.5, shardY - 0.8, 3, 4);
                    text('▄█ ▀', shardX - 1.5, shardY, '#aab4c0');
                    text('★', shardX, shardY, red);
                }
                // a second piece comes down on the tank
                if (t > 1.6 && t < 2.6) {
                    const k = (t - 1.6) / 1.0;
                    text('▪', 13 + 5 * k, -14 + 12.5 * k, '#aab4c0');
                    flames(12.5 + 5 * k, -14.8 + 12.5 * k, 1, 2);
                }
                const boom = (at, x, y) => {
                    if (t < at || t > at + 0.6) return;
                    text('✸', x, y, '#ffcc40');
                    cam.font(1.2);
                    text('*BUM!*', x - 2.5, y - 1.6, '#ffb040');
                    cam.font();
                };
                boom(2.33, 7, -9.2); boom(2.6, 18, -2);

                // the two workers. They watch it go up; then they are at their own little fire
                const camp = t > 4.5;
                if (!camp) {
                    const scared = t > 2.4;
                    [-23, -14.3].forEach((x, i) => {
                        text(' ▄▄▄', x, -2, hat);
                        text(scared ? '(O_O)' : '(o_o)', x, -1, skin);
                        text(i === 1 && t > 1.9 && t < 2.4 ? '/|▒|/' : '/|▒|\\', x, 0, vest);
                        text(' | |', x, 1, '#5a5a7a');
                    });
                    if (t > 1.9 && t < 2.4) text('?', -13, -3, '#fff');
                    if (scared) text('!!', -22, -3, '#fff');
                } else {
                    // roasting sausages, sticks out over a fire that will not catch
                    const blow = t > 5.2 && t < 6.2;
                    text(' ▄▄▄', -23, -2, hat);
                    text(blow ? '(°3°)' : '(^_^)', -23, -1, skin);
                    text('/|▒|──', -23, 0, vest);
                    text('▬', -17.6, 0, '#b05030');
                    text(' | |', -23, 1, '#5a5a7a');
                    text(' ▄▄▄', -14.3, -2, hat);
                    text('(^_^)', -14.3, -1, skin);
                    text('──|▒|\\', -16.3, -0.35, vest);
                    text('▬', -16.9, -0.35, '#b05030');
                    text(' | |', -14.3, 1, '#5a5a7a');
                    // the campfire: two sticks and a flicker, until it finally catches
                    text('╳', -17.4, 1.15, '#8f6a3a');
                    if (t > 8.8) flames(-18, 0.5, 3, 1);
                    else if (Math.floor(t * 4) % 3 === 0) {
                        cam.font(0.5);
                        text('ʌ', -17.2, 0.6, '#ff7a2a');
                        cam.font();
                    }
                    // a radio on a crate, out of the close-up
                    if (t > 6.4) {
                        text('▄▄▄', -7, 1, '#8f7c50');
                        text('[◘]', -7, 0, '#aaa');
                    }
                }

                if (camp && t < 6.4) {
                    cam.font(0.4);
                    if (t > 4.8 && t < 6.0) text('"Tuli ei meinaa syttyä..."', -23.4, -2.6, '#fff');
                    cam.font(0.35);
                    if (t > 5.2 && t < 6.2) text('*puh puh*', -17.9, -1.0, '#ddd');
                    if (t > 6.0) text('*tsss*', -18.2, -0.9, '#ddd');
                    cam.font();
                }
                if (t > 7.6 && t < 8.8) {
                    cam.font(1.6);
                    text('◁))  "Savu on länsimaista propagandaa."', -36, -15.5, '#8fd0ff');
                    cam.font();
                }
                if (t > 8.8) {
                    cam.font(1.8);
                    text('"Nyt syttyi!"', -40, -4.5, '#fff');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'FSB-AGENTTI', line: '"FSB valvoo." Kaikki on hallinnassa.',
            color: '#9a9aa8',
            dur: 8.6 + READ_S,
            // close on his eyes peering through holes in a newspaper; pull back: he is tailing
            // another agent; pull back: a whole ring of agents tailing each other round a park,
            // while the real suspect gets up off the bench in the middle and strolls away
            render(ctx, W, H, t, fs) {
                // the ring: N agents on an ellipse, each tailing the one ahead of him
                const N = 9, RX = 24, RY = 6, RC = -7.5, W0 = 0.18;
                const at = (i) => {
                    const a = Math.PI / 2 + W0 * t + i * (2 * Math.PI / N);
                    return { x: RX * Math.cos(a), y: RC + RY * Math.sin(a), back: Math.sin(a) < 0 };
                };
                const me = at(0);
                const shots = [
                    { zoom: 4.5, x: me.x + 3.4, y: me.y - 2 },   // his eyes
                    { zoom: 1.5, x: me.x - 5, y: me.y - 3.2 },   // him and the man he tails
                    { zoom: 0.7, x: 0, y: -7.5 },                // the whole ring
                ];
                const cuts = [[1.6, 0.7], [3.6, 1.2]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const hat = '#4a4a4a', paper = '#c8c8c0', coat = '#7a7a6a';

                // the park: a bench and the man everyone is supposed to be watching
                if (t > 3.4) {
                    const walk = Math.max(0, t - 5.5);
                    const sx = -1.5 + 9 * walk, sy = -8 + Math.min(6.5, 2.4 * walk);
                    text('(¬‿¬)', sx, sy, '#f0e0c8');
                    text(walk > 0 && Math.floor(t * 5) % 2 ? ' ╱╲[$]' : ' ││[$]', sx, sy + 1, '#5a6a8a');
                    // the bench goes on top of him while he still sits on it
                    text('▄▄▄▄▄▄▄▄', -4, -7, '#8f6a3a');
                    text('╨      ╨', -4, -6.3, '#8f6a3a');
                    if (walk > 0.2 && walk < 2.4) {
                        cam.font(1.2);
                        text('♪ *vihellys*', sx + 5, sy - 1.6, '#ddd');
                        cam.font();
                    }
                }

                // the agents, back of the ring first
                const ring = [];
                for (let i = 0; i < N; i++) ring.push({ i, ...at(i) });
                ring.sort((a, b) => a.y - b.y);
                ring.forEach(({ i, x, y, back }) => {
                    ctx.globalAlpha = back ? 0.7 : 1;
                    const step = Math.floor(t * 4 + i) % 2;
                    const look = Math.floor(t * 1.5 + i * 0.7) % 2;
                    text('  ▄██▄', x, y - 3, hat);
                    text('▐      ▌', x, y - 2, paper);
                    text(look ? '◐ ◐' : '◑ ◑', x + 2, y - 2, '#fff');
                    text('▐      ▌', x, y - 1, paper);
                    text('ПРАВДА', x + 1, y - 1, '#c03030');
                    text(step ? '  ╱ ╲' : '  │ │', x, y, coat);
                    ctx.globalAlpha = 1;
                });

                if (t > 0.2 && t < 1.6) {
                    cam.font(0.4);
                    text('"FSB valvoo."', me.x + 0.6, me.y - 3.7, '#fff');
                    cam.font();
                }
                if (t > 1.9 && t < 3.6) {
                    cam.font(0.7);
                    if (Math.floor(t * 4) % 2) text('*hiip*', me.x + 1, me.y + 0.9, '#aaa');
                    if (t > 2.3) text('"Kohde ei huomaa mitään."', me.x + 8, me.y - 2, '#fff');
                    cam.font();
                }
                cam.font(1.1);
                // over the radio, from one of them to all of them
                if (t > 6.0 && t < 7.2) text('◁))  "Kohde liikkuu ympyrää."', -16.5, -10.6, '#8fd0ff');
                if (t > 7.2) text('◁))  "Kaikki on hallinnassa."', -16.5, -10.6, '#8fd0ff');
                cam.font();
            },
        },
        {
            who: 'SEKÄ', name: 'JOHTAJA, 61', line: 'Kaatui ikkunasta. Ihan itse.',
            color: '#cfcfcf',
            dur: 8.2 + READ_S,
            // close on him falling from his office window, calm about it; pull back: a queue of new
            // directors at the door, briefcases ready, and the glazier fixing the window. Again;
            // pull back: the queue runs off down the street. The next one looks out of the window
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const fk = Math.min(1, t / 1.8);
                const dx = 19 + 2 * fk, dy = -9 + 8 * fk * fk;     // the falling director
                const shots = [
                    { zoom: 4.0, x: dx + 2.2, y: dy + 0.5 },  // him, falling
                    { zoom: 0.85, x: 4, y: -3.5 },            // the building, the door, the queue
                    { zoom: 0.45, x: -25, y: -6 },            // the whole street
                ];
                const cuts = [[2.0, 0.9], [5.0, 1.0]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', suit = '#4a4a5a', wall = '#8a8a8a', glass = '#9fc8e0';
                const fixed = t > 4.4;

                text('_'.repeat(240), -120, 1.7, '#555');

                // the office block, his window on the top floor
                text('╔═══ NAFTA OY ═══╗', 0, -11, '#cfcfcf');
                for (let y = -10; y <= 1; y++) { text('█', 0, y, wall); text('█', 17, y, wall); }
                for (let y = -9; y <= -3; y += 2) [2, 7.5, 13].forEach((x) => {
                    if (x === 13 && y === -9) return;
                    text('▒▒▒', x, y, glass);
                });
                if (fixed) text('▒▒▒', 13, -9, glass);
                else { text('◥ ◣', 13, -9, glass); }
                text('┌──┐', 7, -0.3, '#9a8a6a');
                text('│  │', 7, 0.7, '#9a8a6a');

                // a bush he lands in, glass coming down with him
                text('▓▓▓▓▓▓', 19, 0.4, '#4a7a3a');
                text('▓▓▓▓▓▓', 19, 1.2, '#3a6a2a');
                if (t < 1.8) {
                    text('◢', dx - 1.6, dy - 1.5, glass);
                    text('◣', dx + 5.4, dy + 0.6, glass);
                    text('\\(・_・)/', dx - 1, dy, skin);
                    text('|▼|', dx + 1, dy + 1, suit);
                    text('▼', dx + 2, dy + 1, '#c03030');
                    text('/ \\', dx + 1, dy + 2, suit);
                    if (t > 0.3) {
                        cam.font(0.4);
                        text('"Ihan itse..."', dx - 1.5, dy - 1.4, '#fff');
                        cam.font();
                    }
                } else {
                    // just his legs out of the top of the bush
                    text('╿ ╿', 21.5, -0.4, suit);
                    if (t < 2.8) {
                        cam.font(0.8);
                        text('*TÖMPS*', 18.5, -2.2, '#ddd');
                        cam.font();
                    }
                }

                // the glazier on his ladder, out of the close-up
                if (t > 2.0) {
                    for (let y = -8; y <= 1; y++) text('╫', 18.2, y, '#8f6a3a');
                    text(' ▄▄▄', 17.5, -11.2, '#e0c040');
                    text('(-_-)', 17.5, -10.2, skin);
                    text(fixed ? '/|▒|' : '▯|▒|', 17.5, -9.2, '#5a7a9a');
                    if (t > 3.6 && t < 4.4 && Math.floor(t * 6) % 2) text('*kop*', 22.5, -9.6, '#ddd');
                    if (t > 2.9 && t < 4.2) text('"Taas."', 23, -11.4, '#fff');
                    // and his van
                    text('▄▄▄▄▄▄▄▄▄▄▄▄█▄', 27, -0.4, '#d0d0d0');
                    text('█ LASI OY 24/7 █', 26.5, 0.6, '#d0d0d0');
                    text(' ◎          ◎', 27, 1.4, '#666');
                }

                // the queue of new directors, briefcases ready, shuffling up one when the door opens
                const shuffle = 6 * span(3.6, 4.6);
                for (let i = 0; i < 16; i++) {
                    const x = 1 - 6 * i + shuffle;
                    if (i === 0 && t > 4.0) continue;      // the first one has gone in
                    text(' ▄▄', x, -1.8, '#3a3a3a');
                    text('(•_•)', x, -1, skin);
                    text('/|▼|▄▆', x, 0, suit);
                    text('▼', x + 2, 0, '#c03030');
                    text(' / \\', x, 1, suit);
                }
                if (t > 2.6 && t < 3.6) text('"Seuraava!"', 4.5, -4, '#fff');

                // down the street: the job advert, and the ticket machine at the back of the queue
                if (t > 5.0) {
                    text('┌─────────────────────────┐', -90, -11, '#ddd');
                    text('│  AVOIN TYÖPAIKKA:       │', -90, -10, '#ddd');
                    text('│  JOHTAJA (taas)         │', -90, -9, '#ddd');
                    text('│  Ikkunanäköala.         │', -90, -8, '#ddd');
                    text('└─────────────────────────┘', -90, -7, '#ddd');
                    for (let y = -6; y <= 1; y++) { text('║', -86, y, '#8f7c50'); text('║', -68, y, '#8f7c50'); }
                    text('JOHTAJA (taas)', -87, -9, '#f2d34a');
                    text('[№ 17]', -96, 0, '#e05050');
                    text('  ▐▌', -96, 1, '#888');
                }

                // and the next one looks out of the window, down at the bush
                if (t > 6.6) {
                    text('(°_°)', 12.5, -9, skin);
                    cam.font(1.6);
                    if (t > 7.0 && t < 7.8) text('*glup*', 10, -13.5, '#ddd');
                    if (t > 7.8) text('"...ihan itse?"', 2, -14, '#fff');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'VARTIJA', line: '"Stoi! Lattiaan!" Kremlin vartio.',
            color: '#c0c0e0',
            dur: 9.0 + READ_S,
            // close on a guard shouting; pull back: a huge row of guards in front of a red wall,
            // the Kremlin surely; pull back: it is Vova's own dacha on the Black Sea, and there
            // were no guards left for anywhere else; pull back: dozens of anti-aircraft guns round it
            render(ctx, W, H, t, fs) {
                const shots = [
                    { zoom: 8.0, x: 2.5, y: -1.5 },          // his face
                    { zoom: 0.7, x: 0, y: -3 },              // the row of guards at the wall
                    { zoom: 0.35, x: 18, y: -10 },           // the dacha by the sea
                    { zoom: 0.2, x: 15, y: -13 },            // the guns all round it
                ];
                const cuts = [[1.5, 0.8], [3.8, 1.0], [6.2, 1.2]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', uniform = '#5a6a8a', brick = '#8a2a22', gold = '#e0b84a';
                const volley = t > 8.0;

                // the hill behind the wall, and the dacha on top of it
                for (let y = -15; y <= -6; y++) {
                    const hw = 10 + (y + 15) * 4.3;
                    text('▓'.repeat(Math.round(2 * hw)), -hw, y, '#24402a');
                }
                text('VOVAN DACHA', -5.5, -24, '#e05050');
                text('           ▄█▄', -12.5, -22, gold);
                text('         ▄█████▄', -12.5, -21, gold);
                text('▄▄▄▄▄▄▄▄▄█▀▀▀▀▀█▄▄▄▄▄▄▄▄▄', -12.5, -20, '#e8dcc0');
                for (let y = -19; y <= -17; y++) text('█ ║ ▒▒ ║ ▒▒ ║ ▒▒ ║ ▒▒ ║ █', -12.5, y, '#e8dcc0');
                text('█▄║▄▄▄▄║▄▄▄▄║▄▄▄▄║▄▄▄▄║▄█', -12.5, -16, '#e8dcc0');

                // the land, the beach, the Black Sea
                text('_'.repeat(240), -160, 1.7, '#555');
                text('░░░░░░░░░░', 80, 1.3, '#c8b27a');
                for (let r = 0; r < 16; r++) {
                    const off = (t * 1.5 + r * 0.7) % 2;
                    text('≈ ∽ '.repeat(30), 90 + off, -12.5 + r * 0.9, '#3a7ab0');
                }
                [81, 86].forEach((x) => {
                    for (let y = -2; y <= 1; y++) text('│', x + 1, y, '#8f6a3a');
                    text('\\\\|//', x - 1, -3, '#4a9a4a');
                });

                // the sign: there was nobody left for anywhere else
                text('│', 46, -6.9, '#8f7c50');
                cam.font(2);
                text('← KREML:    0 vartijaa', 22, -11, '#f2efe0');
                text('← RINTAMA:  0 miestä', 22, -9, '#f2efe0');
                cam.font();

                // the wall, Kremlin-red, swallowtail battlements and all
                text('▐█▌ '.repeat(40), -80, -5.9, brick);
                for (let y = -5; y <= 0.5; y++) text('█'.repeat(160), -80, y, brick);

                // the row of guards in front of it
                for (let i = 0; i <= 20; i++) {
                    const x = -75 + 7.5 * i, me = i === 10;
                    const shout = me ? t > 0.2 && t < 1.5 : t > 2.2 + Math.abs(i - 10) * 0.08 && t < 3.0 + Math.abs(i - 10) * 0.08;
                    const shake = me && t < 1.5 ? (Math.floor(t * 8) % 2 ? 0.15 : -0.15) : 0;
                    text(' ▄▄▄', x, -2, '#3a4a6a');
                    text(shout ? '(òOó)' : '(ò_ó)', x + shake, -1, skin);
                    text('/|█|╿', x, 0, uniform);
                    text(' | |', x, 1, uniform);
                    if (!me && shout) text('stoi!', x - 0.5, -3, '#ddd');
                }
                if (t > 0.2 && t < 1.5) {
                    cam.font(0.35);
                    text('"Stoi! Lattiaan!"', -0.9, -2.4, '#fff');
                    cam.font();
                }

                // the guns: a mountain of them to the west, barges of them out at sea
                const gun = (x, y, i) => {
                    cam.font(2);
                    text('   ╱╱', x, y - 2, '#8a8a8a');
                    text(' ▄██▄', x, y, '#6a7a4a');
                    if (volley && Math.floor(t * 12 + i) % 2) text('     ✶', x, y - 4, '#ffcc40');
                    cam.font();
                };
                for (let y = -15; y <= 1; y++) {
                    const hw = 6 + (y + 15) * 2.4;
                    text('▓'.repeat(Math.round(2 * hw)), -120 - hw, y, '#3a3a3a');
                }
                let g = 0;
                for (let x = -152; x <= -92; x += 10) gun(x, 1, g++);
                for (let x = -142; x <= -98; x += 11) gun(x, -4.5, g++);
                for (let x = -134; x <= -106; x += 14) gun(x, -10, g++);
                gun(-124, -15.5, g++);
                const barge = (x, y) => { gun(x, y, g++); cam.font(2); text('▀▀▀▀▀', x, y + 1.2, '#555'); cam.font(); };
                for (let x = 118; x <= 170; x += 13) barge(x, 0.2);
                for (let x = 124; x <= 164; x += 13) barge(x, -5.3);
                for (let x = 130; x <= 160; x += 15) barge(x, -10.8);

                // a seagull flies over, and every gun on the coast opens up
                if (t > 7.4) {
                    const gx = 150 - 60 * (t - 7.4);
                    cam.font(2.5);
                    text(Math.floor(t * 4) % 2 ? 'ᵛ' : '~v~', gx, -32, '#ddd');
                    cam.font();
                }
                if (volley) {
                    cam.font(5);
                    text('"STOI!!"', -35, -30, '#fff');
                    cam.font();
                }
            },
        },
        {
            who: 'SEKÄ', name: 'AVUSTAJA', line: 'Henkilökohtainen avustaja. Henkilökohtaisesti kännissä.',
            color: '#a0b0d0',
            dur: 8.6 + READ_S,
            // close on him carrying the nuclear briefcase with great care; pull back: he is drunk
            // and the case is full of vodka; pull back: he staggers after Vova down a Kremlin
            // corridor, and the real briefcase sits forgotten on the bar behind them
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 5.0, x: 3.8, y: -0.7 },          // him and the case
                    { zoom: 1.8, x: 3, y: -1.5 },            // drunk, the case open
                    { zoom: 0.9, x: -11, y: -3 },             // the corridor, the bar
                ];
                const cuts = [[1.6, 0.7], [4.2, 1.0]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', suit = '#3a3a4a', red = '#e03030';

                text('_'.repeat(140), -70, 1.7, '#555');

                // the corridor: portraits on the wall, a pillar, and the bar
                if (t > 4.2) {
                    for (let x = -60; x <= 40; x += 16) {
                        if (x === 4 && t > 5.6) {           // he bumped the wall: this one hangs crooked
                            text('  ╱╲', x, -7.4, '#b0903a'); text(' ╱◕ ╱', x, -6.4, '#b0903a'); text(' ╲╱', x, -5.4, '#b0903a');
                            continue;
                        }
                        text('╔═══╗', x, -7, '#b0903a');
                        text('║ ◕ ║', x, -6, '#b0903a');
                        text('╚═══╝', x, -5, '#b0903a');
                    }
                    // the bar, the barman, and the real nuclear briefcase used as a coaster
                    text('▄▄▄▄▄▄▄▄▄▄▄▄▄▄', -40, -0.4, '#8f6a3a');
                    text('█            █', -40, 0.5, '#6a4a2a');
                    text('█            █', -40, 1.3, '#6a4a2a');
                    text(' ▄▄', -36, -3.2, '#ddd');
                    text('(•_•)', -36, -2.4, skin);
                    text('[ ]', -32, -1.2, '#888');
                    text('☢', -31, -1.2, '#ffcc40');
                    if (t > 5.6) text('▯', -31, -2.0, '#9fc8e0');     // a glass put down on it
                    cam.font(1.2);
                    text('UNOHTUI', -35, -4.4, red);
                    cam.font();
                }

                // Vova, walking on ahead
                if (t > 4.2) {
                    const vx = 15 + 4 * span(4.4, 6.8);
                    const turned = t > 6.8;
                    text(turned ? '(ಠ_ಠ)' : '(¬_¬)', vx, -1, skin);
                    text('/|▼|\\', vx, 0, '#2a2a3a');
                    text(' / \\', vx, 1, '#2a2a3a');
                }

                // him: steady in the close-up, then swaying, then staggering along behind
                const drunk = t > 1.6;
                const ax = 4 * span(4.6, 6.6) + (drunk ? Math.sin(t * 3) * 0.6 : 0);
                const lean = drunk ? Math.sin(t * 3) * 0.25 : 0;
                const swig = t > 3.0 && t < 3.6;
                text(!drunk ? '(-_-)' : swig ? '(ˆoˆ)' : '(ˇ‿ˇ)', ax + lean, -1, skin);
                if (drunk) text('*', ax + lean + 0.6, -0.6, '#e07070');
                text(swig ? '/|=|╲▮' : '/|=|\\_', ax, 0, suit);
                text(' / \\', ax, 1, suit);
                // the case: shut and solemn, then open and full of bottles
                const open = t > 2.4;
                text(open ? '[▮▮▮]' : '[   ]', ax + 6, 0.2, open ? '#7a9a7a' : '#888');
                if (!open) text('☢', ax + 7, 0.2, '#ffcc40');
                else {
                    text('▯▯▯', ax + 7, -0.6, '#9fc8a0');
                    text('╱▔▔▔╲', ax + 6, -1.3, '#888');
                }

                if (t > 0.2 && t < 1.6) {
                    cam.font(0.4);
                    text('"Ydinsalkku."', ax + 1, -2.2, '#fff');
                    cam.font();
                }
                if (drunk && t < 4.2 && Math.floor(t * 2) % 2) text('*hik*', ax - 3, -2.4, '#ddd');
                if (t > 2.4 && t < 3.0) {
                    cam.font(0.7);
                    text('*kilin kilan*', ax + 5, -2.4, '#ddd');
                    cam.font();
                }
                if (swig) text('*glug*', ax + 6, -2.2, '#ddd');
                if (t > 5.6 && t < 6.2) {
                    cam.font(1.2);
                    text('*BONK*', 2, -9.6, '#ddd');
                    cam.font();
                }
                cam.font(1.4);
                if (t > 6.8 && t < 7.6) text('"Missä salkku?"', 4, -4.5, '#fff');
                if (t > 7.6) text('"*hik* ...turvassa."', -12, -4.5, '#fff');
                cam.font();
            },
        },
        {
            who: 'VIERAILEVA TÄHTI', name: 'MEDVEDEV', line: 'Kirjoittaa Telegramiin. ISOILLA KIRJAIMILLA.',
            color: '#e0e0e0',
            dur: 8.2 + READ_S,
            // close on him on the throne, proud; pull back: the throne says it is reserved for
            // V.V.P.; pull back: Vova with a stopwatch. "Warm? Move." He hops down onto his own
            // little stool, and Vova sits down
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 5.0, x: 4.0, y: -1.8 },          // him on the throne
                    { zoom: 2.6, x: 4.5, y: -3.4 },          // the reservation on the throne
                    { zoom: 1.0, x: -1, y: -2 },              // Vova and the stool
                ];
                const cuts = [[1.6, 0.7], [4.0, 1.0]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', gold = '#e0b84a', velvet = '#a02030', suit = '#2a2a3a';

                text('_'.repeat(100), -50, 1.7, '#555');

                // the throne, and on top of it the reservation
                text('   ▄♛▄', 0, -5, gold);
                for (let y = -4; y <= -1; y++) text('█▓▓▓▓▓▓▓█', 0, y, y === -4 ? gold : velvet);
                text('█▄▄▄▄▄▄▄█', 0, 0, gold);
                text('▌       ▐', 0, 1, gold);
                cam.font(0.7);
                text('PAIKKA VARATTU – V.V.P.', -3.5, -6.2, '#f2efe0');
                cam.font();

                // his own seat, out of the earlier shots
                text('▄▄▄▄▄▄', -18, 0.3, '#8f6a3a');
                text('╥    ╥', -18, 1.1, '#8f6a3a');
                text('DIMI', -17, 1.1, '#f2d34a');

                // Vova with his stopwatch, walking over to sit down once it is warm
                const sitV = span(6.2, 6.9);
                const vx = 24 - 22.5 * sitV, vy = -1 * sitV;
                text(t > 6.9 ? '(¬‿¬)' : '(¬_¬)', vx, -1 + vy, skin);
                text(t > 6.9 ? '/|▼|\\' : '/|▼|◷', vx, 0 + vy, suit);
                if (t < 6.9) text(' / \\', vx, 1, suit);

                // him: on the throne, then hop down onto the stool
                const hop = span(5.6, 6.2);
                const mx = 1.5 - 18 * hop, my = -2 + 1.0 * hop - Math.sin(hop * Math.PI) * 2.5;
                const nervous = t > 2.6 && t < 5.6;
                text(t > 6.2 ? '(ò_ó)' : nervous ? '(°_°;)' : '(¬‿¬)', mx, my, skin);
                text(t > 6.2 ? '/|▓|▯' : '/|▓|\\', mx, my + 1, '#4a4a5a');

                if (t > 0.2 && t < 1.6) {
                    cam.font(0.4);
                    text('"Minä olen presidentti."', 0.2, -3.1, '#fff');
                    cam.font();
                }
                if (t > 3.0 && t < 5.0 && Math.floor(t * 4) % 2) text('*tik tak*', 18, -2.6, '#ddd');
                cam.font(1.3);
                if (t > 4.6 && t < 5.8) text('"Lämmin? Siirry."', 8, -4.5, '#fff');
                cam.font();
                // and on his stool he does what he does: Telegram, in capitals
                if (t > 7.0) {
                    cam.font(1.1);
                    text('✈ "MINÄ PÄÄTÄN KAIKESTA!!!"', -31, -4, '#8fd0ff');
                    cam.font();
                }
            },
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
            dur: 8.6 + READ_S,
            // close on a kid getting the toy out of his kids' meal; pull back: it is a toy tank,
            // the box says PATRIOTIC MEAL and the menu offers a meal with call-up papers; pull
            // back: a recruiter stands at the till handing his father the summons with the tray
            render(ctx, W, H, t, fs) {
                const span = (a, b) => ease((t - a) / (b - a));
                const shots = [
                    { zoom: 6.0, x: 1.5, y: -1.6 },          // the kid and his toy
                    { zoom: 2.0, x: 10, y: -3.0 },            // the box, the menu board
                    { zoom: 0.9, x: -11, y: -3 },             // the till and the recruiter
                ];
                const cuts = [[1.6, 0.8], [4.4, 1.0]];
                let cz = shots[0].zoom, cx = shots[0].x, cy = shots[0].y;
                cuts.forEach(([at, len], i) => {
                    const k = ease((t - at) / len), to = shots[i + 1];
                    cz += (to.zoom - cz) * k; cx += (to.x - cx) * k; cy += (to.y - cy) * k;
                });
                const cam = lens(ctx, W, H, fs, cz, cx, cy);
                const { text } = cam;
                const skin = '#f0e0c8', red = '#d03030', khaki = '#6a7a4a';

                text('_'.repeat(120), -60, 1.7, '#555');

                // the menu board over them
                text('╔══════════════════════╗', -1, -7.5, '#ddd');
                text('║                      ║', -1, -6.5, '#ddd');
                text('║                      ║', -1, -5.5, '#ddd');
                text('╚══════════════════════╝', -1, -4.5, '#ddd');
                text('ATERIA + KUTSUNNAT', 1.5, -6.5, '#f2d34a');
                text('vain 199 ₽', 6.5, -5.5, '#f2efe0');

                // the kid with his box and his toy
                text('▄▄▄▄▄▄▄▄▄', 2, 0.6, '#8f6a3a');                  // a little table
                text('┌─────┐', 4, -0.9, red);
                text('│  ★  │', 4, -0.1, red);
                if (t > 1.6) {
                    cam.font(0.45);
                    text('ISÄNMAALLINEN', 3.9, -1.5, '#f2d34a');
                    text('ATERIA', 5.5, 0.4, '#f2d34a');
                    cam.font();
                }
                text(t < 0.6 ? '(•o•)' : '(^o^)', -1, -1, skin);
                text('/|▒|', -1, 0, '#4a7ab0');
                text(' | |', -1, 1, '#4a7ab0');
                // the toy, held up high
                const lift = span(0.3, 0.8);
                text('▄█▄═', 3, -1.2 - 0.8 * lift, khaki);
                if (t > 0.8 && t < 1.6) {
                    cam.font(0.4);
                    text('"Jee! Tankki!"', -1.2, -2.6, '#fff');
                    cam.font();
                }

                // his father, and the summons that comes with the tray
                const dad = t > 6.4 ? '(°_°)' : '(-_-)';
                text(dad, 11, -1, skin);
                text('/|▓|\\', 11, 0, '#5a5a6a');
                text(' / \\', 11, 1, '#5a5a6a');

                // the till: the cashier, and the recruiter beside her
                text('▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄', -28, -0.2, '#b0b0b0');
                text('█                █', -28, 0.7, '#8a8a8a');
                text('█                █', -28, 1.4, '#8a8a8a');
                text(' ▄▄▄', -25, -2.8, red);
                text('(^_^)', -25, -1.9, skin);
                text(' ▄▄▄', -18, -2.8, khaki);
                text('(ò_ó)', -18, -1.9, skin);
                text(t > 5.8 && t < 6.6 ? '/|█|═' : '/|█|\\', -18, -1, khaki);
                cam.font(1.2);
                text('VKUSNO I TOTŠKA', -32, -9, '#e05050');
                cam.font();
                // the queue behind, all men of call-up age
                for (let i = 0; i < 5; i++) {
                    const x = -36 - 6 * i;
                    text('(-_-)', x, -1, skin);
                    text('/|▓|\\', x, 0, '#5a5a6a');
                    text(' / \\', x, 1, '#5a5a6a');
                }
                // the summons flies over to him
                if (t > 5.8) {
                    const k = span(5.8, 6.4);
                    text('▭ ПОВЕСТКА', -14 + 24 * k, -2.4 + Math.sin(k * Math.PI) * -2, '#f2efe0');
                }
                cam.font(1.3);
                if (t > 6.6 && t < 7.6) text('"Tilasin vain kahvin."', -6, -3.4, '#fff');
                if (t > 7.6) text('"Ateriaan kuuluu."', -26, -5.2, '#fff');
                cam.font();
            },
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

        if (local < 0.12 && !s.noFlash) {
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
