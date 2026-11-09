// Synthesized sound effects for PПLTSAЯI 2. The game asks for them with an
// OSC 777 escape ("snd;<name>;<volume>") that the page intercepts.
(function () {
    let ctx = null;
    let master = null;
    let muted = false;
    const lastPlayed = {};
    // continuous sources are re-announced every NPC tick; play them sparingly
    const minGapMs = { siren: 1600, radio: 900, fire: 2500, vomit: 900, glug: 450, explosion: 250, drone: 380 };

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

    // Original eighties action-show synth theme: driving bass arpeggios under a
    // square-wave lead in A minor. Returns a function that fades it out.
    function startTheme(seconds) {
        if (!ctx || muted) return () => {};
        const out = ctx.createGain();
        out.gain.value = 0.22;
        out.connect(master);
        const t0 = ctx.currentTime + 0.05;
        const step = 60 / 150 / 2; // eighth notes at 150 bpm
        const note = (n) => 440 * Math.pow(2, (n - 69) / 12);
        // A minor, F, C, G, two bars each
        const chords = [[45, 52, 57], [41, 48, 53], [48, 55, 60], [43, 50, 55]];
        const lead = [81, 79, 76, 79, 81, 84, 81, 79, 77, 76, 72, 74, 76, 79, 76, 74,
                      72, 74, 76, 79, 84, 83, 81, 79, 79, 81, 83, 86, 83, 81, 79, 76];
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
        const steps = Math.floor(seconds / step);
        for (let i = 0; i < steps; i++) {
            const t = t0 + i * step;
            const chord = chords[Math.floor(i / 16) % 4];
            voice('sawtooth', note(chord[i % 3] - 12), t, step * 0.9, 0.5, 700);
            if (i % 2 === 0) voice('square', note(lead[(i / 2) % lead.length]), t, step * 1.8, 0.16, 2400);
            if (i % 4 === 0) { burst(t, 0.6, { type: 'lowpass', freq: 160, decay: 0.12, dur: 0.13 }); }
            if (i % 4 === 2) { burst(t, 0.25, { type: 'highpass', freq: 5000, decay: 0.05, dur: 0.06 }); }
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
