// Synthesized sound effects for Grand Tyr Moscow. The game asks for them with an
// OSC 777 escape ("snd;<name>;<volume>") that the page intercepts.
(function () {
    let ctx = null;
    let master = null;
    let muted = false;
    const lastPlayed = {};
    // continuous sources are re-announced every NPC tick; play them sparingly
    const minGapMs = { splat: 140, splatter: 90, siren: 1600, radio: 900, fire: 2500, vomit: 900, glug: 450, explosion: 250, drone: 380 };

    function init() {
        if (!ctx) {
            ctx = new (window.AudioContext || window.webkitAudioContext)();
            master = ctx.createGain();
            master.gain.value = 0.6;
            master.connect(ctx.destination);
        }
        if (ctx.state === 'suspended') ctx.resume();
    }

    const level = (v) => Math.max(0.0002, v);

    function noise(dur) {
        const buf = ctx.createBuffer(1, Math.ceil(ctx.sampleRate * dur), ctx.sampleRate);
        const data = buf.getChannelData(0);
        for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
        const src = ctx.createBufferSource();
        src.buffer = buf;
        return src;
    }

    function envelope(t, attack, peak, decay) {
        const g = ctx.createGain();
        g.gain.setValueAtTime(0.0001, t);
        g.gain.exponentialRampToValueAtTime(level(peak), t + attack);
        g.gain.exponentialRampToValueAtTime(0.0001, t + attack + decay);
        return g;
    }

    function burst(t, vol, { dur = 0.12, type = 'highpass', freq = 800, q = 0.7, peak = 1, decay = 0.1 } = {}) {
        const n = noise(dur + 0.05);
        const f = ctx.createBiquadFilter();
        f.type = type;
        f.frequency.value = freq;
        f.Q.value = q;
        n.connect(f).connect(envelope(t, 0.002, peak * vol, decay)).connect(master);
        n.start(t);
        n.stop(t + dur + 0.05);
    }

    function tone(t, vol, { type = 'sine', f0 = 440, f1 = f0, dur = 0.2, peak = 0.5 } = {}) {
        const o = ctx.createOscillator();
        o.type = type;
        o.frequency.setValueAtTime(f0, t);
        o.frequency.exponentialRampToValueAtTime(Math.max(1, f1), t + dur);
        o.connect(envelope(t, 0.005, peak * vol, dur)).connect(master);
        o.start(t);
        o.stop(t + dur + 0.05);
    }

    const thump = (t, vol, f = 120, dur = 0.15) => tone(t, vol, { f0: f, f1: 40, dur, peak: 0.9 });

    function gurgle(t, vol, dur, freq) {
        const n = noise(dur);
        const f = ctx.createBiquadFilter();
        f.type = 'bandpass';
        f.frequency.setValueAtTime(freq, t);
        f.frequency.linearRampToValueAtTime(freq * 0.6, t + dur);
        f.Q.value = 2;
        const g = ctx.createGain();
        g.gain.setValueAtTime(0.0001, t);
        g.gain.exponentialRampToValueAtTime(level(0.9 * vol), t + 0.08);
        g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
        const lfo = ctx.createOscillator();
        lfo.frequency.value = 18;
        const depth = ctx.createGain();
        depth.gain.value = 0.5 * vol;
        lfo.connect(depth).connect(g.gain);
        n.connect(f).connect(g).connect(master);
        n.start(t);
        n.stop(t + dur);
        lfo.start(t);
        lfo.stop(t + dur);
    }

    function pluck(t, freq, vol) {
        [1, 2, 3].forEach((harmonic, k) => {
            const o = ctx.createOscillator();
            o.type = k === 0 ? 'triangle' : 'sine';
            o.frequency.value = freq * harmonic;
            o.connect(envelope(t, 0.003, vol * (k === 0 ? 0.5 : 0.15 / k), 0.25)).connect(master);
            o.start(t);
            o.stop(t + 0.35);
        });
    }

    const sounds = {
        // pistol: one sharp bang
        pistol(t, v) { burst(t, v, { freq: 1200, decay: 0.12 }); thump(t, v, 150, 0.12); },
        // assault rifle: a short rattle
        rifle(t, v) {
            for (let i = 0; i < 4; i++) {
                burst(t + i * 0.075, v, { freq: 1500, decay: 0.06, dur: 0.07 });
                thump(t + i * 0.075, v * 0.7, 110, 0.07);
            }
        },
        // machine gun: many fast heavy bangs
        mg(t, v) {
            for (let i = 0; i < 9; i++) {
                burst(t + i * 0.055, v, { freq: 900, decay: 0.045, dur: 0.05 });
                thump(t + i * 0.055, v * 0.8, 90, 0.06);
            }
        },
        aa(t, v) {
            for (let i = 0; i < 2; i++) {
                thump(t + i * 0.35, v, 70, 0.5);
                burst(t + i * 0.35, v * 0.7, { type: 'lowpass', freq: 500, decay: 0.45, dur: 0.5 });
            }
        },
        explosion(t, v) {
            burst(t, v, { type: 'lowpass', freq: 400, decay: 1.3, dur: 1.4, peak: 1.2 });
            thump(t, v, 80, 0.8);
        },
        siren(t, v) {
            const o = ctx.createOscillator();
            o.type = 'sawtooth';
            for (let i = 0; i < 3; i++) {
                o.frequency.setValueAtTime(650, t + i * 0.5);
                o.frequency.linearRampToValueAtTime(950, t + i * 0.5 + 0.25);
                o.frequency.linearRampToValueAtTime(650, t + i * 0.5 + 0.5);
            }
            const f = ctx.createBiquadFilter();
            f.type = 'lowpass';
            f.frequency.value = 2000;
            const g = ctx.createGain();
            g.gain.setValueAtTime(level(0.12 * v), t);
            g.gain.setValueAtTime(level(0.12 * v), t + 1.4);
            g.gain.exponentialRampToValueAtTime(0.0001, t + 1.55);
            o.connect(f).connect(g).connect(master);
            o.start(t);
            o.stop(t + 1.6);
        },
        radio(t, v) {
            for (let i = 0; i < 5; i++) {
                burst(t + i * 0.06 + Math.random() * 0.03, v * 0.35,
                    { type: 'bandpass', freq: 1800 + Math.random() * 800, q: 3, decay: 0.05, dur: 0.05 });
            }
        },
        fire(t, v) {
            for (let i = 0; i < 8; i++) {
                burst(t + Math.random() * 0.8, v * 0.25, { type: 'bandpass', freq: 2500, q: 1, decay: 0.03, dur: 0.03 });
            }
            burst(t, v * 0.25, { type: 'lowpass', freq: 300, decay: 0.8, dur: 0.9 });
        },
        // a moped-engine buzz, like a Shahed overhead
        drone(t, v) {
            const o = ctx.createOscillator();
            o.type = 'sawtooth';
            o.frequency.setValueAtTime(110 + Math.random() * 10, t);
            const wob = ctx.createOscillator();
            wob.frequency.value = 23;
            const wobGain = ctx.createGain();
            wobGain.gain.value = 6;
            wob.connect(wobGain).connect(o.frequency);
            const f = ctx.createBiquadFilter();
            f.type = 'lowpass';
            f.frequency.value = 900;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.0001, t);
            g.gain.exponentialRampToValueAtTime(level(0.18 * v), t + 0.08);
            g.gain.setValueAtTime(level(0.18 * v), t + 0.36);
            g.gain.exponentialRampToValueAtTime(0.0001, t + 0.45);
            o.connect(f).connect(g).connect(master);
            o.start(t); wob.start(t);
            o.stop(t + 0.5); wob.stop(t + 0.5);
        },
        glass(t, v) {
            for (let i = 0; i < 9; i++) {
                burst(t + Math.random() * 0.35, v * 0.5, { type: 'bandpass', freq: 3000 + Math.random() * 4000, q: 6, decay: 0.08, dur: 0.1 });
            }
            thump(t, v, 90, 0.25);
        },
        cough(t, v) {
            for (let i = 0; i < 2; i++) {
                burst(t + i * 0.22, v * 0.8, { type: 'bandpass', freq: 700, q: 1.2, decay: 0.14, dur: 0.16 });
            }
        },
        vomit(t, v) { gurgle(t, v, 1.0, 500); },
        vomit2(t, v) { gurgle(t, v, 1.3, 380); },
        // a wet slap: a low, short burst of mush with a few lumps after it
        splat(t, v) {
            burst(t, v * 0.9, { type: 'lowpass', freq: 420 + Math.random() * 200, q: 1.5, decay: 0.12, dur: 0.14 });
            burst(t, v * 0.5, { type: 'bandpass', freq: 900 + Math.random() * 500, q: 2.5, decay: 0.05, dur: 0.06 });
            for (let i = 0; i < 3; i++) {
                burst(t + 0.03 + Math.random() * 0.12, v * 0.35, { type: 'bandpass', freq: 500 + Math.random() * 900, q: 4, decay: 0.035, dur: 0.04 });
            }
        },
        // a splatter: small wet ticks
        splatter(t, v) {
            for (let i = 0; i < 4; i++) {
                burst(t + Math.random() * 0.08, v * 0.4, { type: 'bandpass', freq: 1400 + Math.random() * 1600, q: 5, decay: 0.025, dur: 0.03 });
            }
        },
        drip(t, v) { tone(t, v, { f0: 1400, f1: 350, dur: 0.09, peak: 0.35 }); },
        glug(t, v) { for (let i = 0; i < 4; i++) tone(t + i * 0.13, v, { f0: 180, f1: 420, dur: 0.1, peak: 0.4 }); },
        // a tremolo-picked folk motif
        balalaika(t, v) {
            const notes = [440, 659, 659, 587, 523, 494, 440, 494, 523, 440];
            notes.forEach((freq, i) => {
                for (let r = 0; r < 3; r++) pluck(t + i * 0.24 + r * 0.06, freq, v * 0.5);
            });
        },
    };

    function play(name, vol) {
        if (!ctx || muted || !sounds[name]) return;
        const now = performance.now();
        const gap = minGapMs[name] ?? 80;
        if (lastPlayed[name] && now - lastPlayed[name] < gap) return;
        lastPlayed[name] = now;
        sounds[name](ctx.currentTime + 0.01, Math.max(0.05, Math.min(1, vol)));
    }

    // The intro theme: an eighties action-show synth groove carrying the
    // Russian folk tune Korobeiniki (public domain), with balalaika tremolo
    // on the chords. Returns a function that fades it out.
    function startTheme(seconds) {
        if (!ctx || muted) return () => {};
        const out = ctx.createGain();
        out.gain.value = 0.22;
        out.connect(master);
        const t0 = ctx.currentTime + 0.05;
        const eighth = 60 / 150 / 2;
        const note = (n) => 440 * Math.pow(2, (n - 69) / 12);
        // Korobeiniki in A minor as [midi note, length in eighths]
        const tune = [
            [76, 2], [71, 1], [72, 1], [74, 2], [72, 1], [71, 1], [69, 2], [69, 1], [72, 1], [76, 2], [74, 1], [72, 1],
            [71, 3], [72, 1], [74, 2], [76, 2], [72, 2], [69, 2], [69, 4],
            [74, 3], [77, 1], [81, 2], [79, 1], [77, 1], [76, 3], [72, 1], [76, 2], [74, 1], [72, 1],
            [71, 2], [71, 1], [72, 1], [74, 2], [76, 2], [72, 2], [69, 2], [69, 4],
        ];
        // a chord per bar under the tune: Am E Am E Dm Am E Am
        const bars = [[45, 52, 57], [40, 47, 52], [45, 52, 57], [40, 47, 52], [38, 45, 50], [45, 52, 57], [40, 47, 52], [45, 52, 57]];
        const voice = (type, freq, t, dur, vol, cutoff) => {
            const o = ctx.createOscillator();
            o.type = type;
            o.frequency.value = freq;
            const f = ctx.createBiquadFilter();
            f.type = 'lowpass';
            f.frequency.value = cutoff;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.0001, t);
            g.gain.exponentialRampToValueAtTime(vol, t + 0.01);
            g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
            o.connect(f).connect(g).connect(out);
            o.start(t);
            o.stop(t + dur + 0.02);
        };
        const pluckOut = (t, freq, vol) => {
            const o = ctx.createOscillator();
            o.type = 'triangle';
            o.frequency.value = freq;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.0001, t);
            g.gain.exponentialRampToValueAtTime(vol, t + 0.004);
            g.gain.exponentialRampToValueAtTime(0.0001, t + 0.14);
            o.connect(g).connect(out);
            o.start(t);
            o.stop(t + 0.16);
        };
        const steps = Math.floor(seconds / eighth);
        // groove: bass, drums and balalaika, every eighth
        for (let i = 0; i < steps; i++) {
            const t = t0 + i * eighth;
            const chord = bars[Math.floor(i / 8) % bars.length];
            voice('sawtooth', note(chord[0] - 12 + (i % 2 ? 12 : 0)), t, eighth * 0.85, 0.45, 650);
            if (i % 4 === 0) burst(t, 0.7, { type: 'lowpass', freq: 150, decay: 0.12, dur: 0.13 });
            if (i % 4 === 2) burst(t, 0.35, { type: 'bandpass', freq: 1800, q: 0.8, decay: 0.09, dur: 0.1 });
            burst(t, 0.12, { type: 'highpass', freq: 7000, decay: 0.03, dur: 0.04 });
            // balalaika tremolo: quick repeated plucks of the chord
            for (let r = 0; r < 3; r++) pluckOut(t + r * eighth / 3, note(chord[(i + r) % 3] + 12), 0.07);
        }
        // the tune on a square-wave lead, looping
        for (let pos = 0, k = 0; pos < steps; k++) {
            const [n, len] = tune[k % tune.length];
            const t = t0 + pos * eighth;
            voice('square', note(n), t, len * eighth * 0.92, 0.14, 2600);
            voice('sawtooth', note(n) * 1.005, t, len * eighth * 0.92, 0.05, 1800); // slight detune for width
            pos += len;
        }
        return () => {
            const now = ctx.currentTime;
            out.gain.cancelScheduledValues(now);
            out.gain.setValueAtTime(out.gain.value, now);
            out.gain.linearRampToValueAtTime(0.0001, now + 0.6);
            setTimeout(() => out.disconnect(), 800);
        };
    }

    window.SFX = {
        init,
        play,
        startTheme,
        toggleMute() { muted = !muted; return muted; },
    };
})();
