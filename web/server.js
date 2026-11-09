const http = require('http');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { spawn } = require('child_process');
const WebSocket = require('ws');

const PORT = process.env.PORT || 3000;
const ROOT = path.join(__dirname, '..');
const GAME = path.join(ROOT, 'pultsari');
const GAME_ARGS = ['--rt', '--los', '--anims', '--path'];

const STATIC = {
    '/': [path.join(__dirname, 'index.html'), 'text/html; charset=utf-8'],
    '/game.js': [path.join(__dirname, 'game.js'), 'text/javascript'],
    '/sfx.js': [path.join(__dirname, 'sfx.js'), 'text/javascript'],
    '/intro.js': [path.join(__dirname, 'intro.js'), 'text/javascript'],
    '/story.js': [path.join(__dirname, 'story.js'), 'text/javascript'],
    '/montage.js': [path.join(__dirname, 'montage.js'), 'text/javascript'],
    '/adart.js': [path.join(__dirname, 'adart.js'), 'text/javascript'],
    '/adart-data.js': [path.join(__dirname, 'adart-data.js'), 'text/javascript'],
};

const MAX_GAMES = Number(process.env.MAX_GAMES || 20);
const MAX_GAMES_PER_IP = Number(process.env.MAX_GAMES_PER_IP || 3);
let running = 0;
const perIp = new Map();

function clientIp(req) {
    return req.socket.remoteAddress || 'unknown';
}

// The game runs as a plain child process: it reads key presses on stdin and
// writes one JSON message per line on stdout, each starting with 0x1E.
// Returns null when the server or this client already runs too many games.
function spawnGame(ip, onMessages, onExit) {
    if (running >= MAX_GAMES || (perIp.get(ip) || 0) >= MAX_GAMES_PER_IP) return null;
    let child;
    try {
        child = spawn(GAME, GAME_ARGS, {
            cwd: ROOT,
            env: { ...process.env, LANG: 'C.UTF-8', PULTSARI_WEB: '1' },
            stdio: ['pipe', 'pipe', 'inherit'],
        });
    } catch (e) {
        console.error('Pelin käynnistys epäonnistui:', e.message);
        return null;
    }
    running++;
    perIp.set(ip, (perIp.get(ip) || 0) + 1);

    let pending = '';
    child.stdout.setEncoding('utf8');
    child.stdout.on('data', (chunk) => {
        pending += chunk;
        const lines = pending.split('\n');
        pending = lines.pop();
        const messages = lines.filter((l) => l.charCodeAt(0) === 0x1e).map((l) => l.slice(1));
        if (messages.length) onMessages(messages);
    });
    child.on('error', (e) => console.error('Peliprosessin virhe:', e.message));
    child.on('exit', () => {
        running--;
        const n = (perIp.get(ip) || 1) - 1;
        if (n > 0) perIp.set(ip, n); else perIp.delete(ip);
        onExit();
    });
    child.stdin.on('error', () => {});
    return {
        write: (keys) => { if (!child.killed && child.stdin.writable) child.stdin.write(keys); },
        kill: () => { try { child.kill(); } catch (e) {} },
    };
}

const END_MSG = '{"t":"end"}';

// HTTP polling sessions for proxies that don't pass WebSockets.
const sessions = new Map();
const SESSION_IDLE_MS = 60000;

function newSession(ip) {
    const id = crypto.randomBytes(16).toString('hex');
    const s = { game: null, queue: [], waiter: null, done: false, seen: Date.now() };
    s.game = spawnGame(ip,
        (messages) => { s.queue.push(...messages); flush(s); },
        () => { s.queue.push(END_MSG); s.done = true; flush(s); });
    if (!s.game) return null;
    sessions.set(id, s);
    return id;
}

function flush(s) {
    if (!s.waiter || (!s.queue.length && !s.done)) return;
    const res = s.waiter;
    s.waiter = null;
    clearTimeout(res.timer);
    // a slow poller only needs the latest frame, plus every non-frame message
    const lastFrame = s.queue.map((m) => m.startsWith('{"t":"frame"')).lastIndexOf(true);
    const out = s.queue.filter((m, i) => i === lastFrame || !m.startsWith('{"t":"frame"'));
    s.queue = [];
    res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
    res.end(JSON.stringify({ messages: out, done: s.done }));
}

setInterval(() => {
    const now = Date.now();
    for (const [id, s] of sessions) {
        if (now - s.seen > SESSION_IDLE_MS) {
            s.game.kill();
            sessions.delete(id);
        }
    }
}, 10000);

function readBody(req, cb) {
    let body = '';
    req.on('data', (c) => { body += c; if (body.length > 4096) req.destroy(); });
    req.on('end', () => cb(body));
}

const server = http.createServer((req, res) => {
    const url = new URL(req.url, 'http://x');
    const p = url.pathname.replace(/^.*\/(api\/)/, '/$1');

    if (p === '/api/new' && req.method === 'POST') {
        const id = newSession(clientIp(req));
        if (!id) { res.writeHead(503); res.end(); return; }
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ id }));
        return;
    }
    if (p === '/api/poll' || p === '/api/input') {
        const s = sessions.get(url.searchParams.get('id'));
        if (!s) { res.writeHead(404); res.end(); return; }
        s.seen = Date.now();
        if (p === '/api/input') {
            readBody(req, (body) => { if (!s.done) s.game.write(body); res.writeHead(204); res.end(); });
            return;
        }
        if (s.waiter) { s.waiter.writeHead(204); s.waiter.end(); clearTimeout(s.waiter.timer); }
        s.waiter = res;
        res.timer = setTimeout(() => {
            if (s.waiter === res) {
                s.waiter = null;
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end('{"messages":[]}');
            }
        }, 20000);
        flush(s);
        return;
    }

    // ASCII video clips for the intro: only plain JSON file names from web/video
    const clip = url.pathname.match(/\/video\/([a-z0-9_-]+\.json)$/);
    if (clip) {
        fs.readFile(path.join(__dirname, 'video', clip[1]), (err, data) => {
            if (err) { res.writeHead(404); res.end('[]'); return; }
            res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
            res.end(data);
        });
        return;
    }

    // recorded sound effects (Pixabay, see web/sounds/CREDITS.md): only plain mp3 file names
    const sample = url.pathname.match(/\/sounds\/([a-z0-9_-]+\.mp3)$/);
    if (sample) {
        fs.readFile(path.join(__dirname, 'sounds', sample[1]), (err, data) => {
            if (err) { res.writeHead(404); res.end(); return; }
            res.writeHead(200, { 'Content-Type': 'audio/mpeg', 'Cache-Control': 'max-age=86400' });
            res.end(data);
        });
        return;
    }

    const name = url.pathname.endsWith('/') ? '/' : '/' + path.basename(url.pathname);
    const entry = STATIC[name];
    if (!entry) { res.writeHead(404); res.end('Not found'); return; }
    fs.readFile(entry[0], (err, data) => {
        if (err) { res.writeHead(500); res.end(); return; }
        res.writeHead(200, { 'Content-Type': entry[1], 'Cache-Control': 'no-store' });
        res.end(data);
    });
});

const wss = new WebSocket.Server({ server });

wss.on('connection', (ws, req) => {
    const send = (text) => { if (ws.readyState === WebSocket.OPEN) ws.send(text); };
    const game = spawnGame(clientIp(req),
        (messages) => send(messages.join('\n')),
        () => { send(END_MSG); ws.close(); });
    if (!game) {
        send('{"t":"full"}');
        ws.close();
        return;
    }
    ws.on('message', (d) => game.write(d.toString()));
    ws.on('close', () => game.kill());
});

server.listen(PORT, '0.0.0.0', () => {
    console.log(`Grand Tyr Moscow running at http://localhost:${PORT}`);
});
