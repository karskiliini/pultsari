const http = require('http');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const pty = require('node-pty');
const WebSocket = require('ws');

const PORT = process.env.PORT || 3000;
const ROOT = path.join(__dirname, '..');
const GAME = path.join(ROOT, 'pultsari');
const GAME_ARGS = ['--rt', '--los', '--anims', '--path'];

const STATIC = {
    '/': [path.join(__dirname, 'index.html'), 'text/html; charset=utf-8'],
    '/xterm.js': [path.join(ROOT, 'node_modules/@xterm/xterm/lib/xterm.js'), 'text/javascript'],
    '/xterm.css': [path.join(ROOT, 'node_modules/@xterm/xterm/css/xterm.css'), 'text/css'],
};

// node-pty's macOS prebuild ships spawn-helper without the execute bit when npm
// skips its install script; without it every spawn fails with "posix_spawnp failed"
function ensureSpawnHelperExecutable() {
    const base = path.dirname(require.resolve('node-pty/package.json'));
    const candidates = [
        path.join(base, 'prebuilds', `${process.platform}-${process.arch}`, 'spawn-helper'),
        path.join(base, 'build', 'Release', 'spawn-helper'),
    ];
    for (const file of candidates) {
        try { fs.chmodSync(file, 0o755); } catch (e) { /* not present on this platform */ }
    }
}
ensureSpawnHelperExecutable();

const MAX_GAMES = Number(process.env.MAX_GAMES || 20);
const MAX_GAMES_PER_IP = Number(process.env.MAX_GAMES_PER_IP || 3);
let running = 0;
const perIp = new Map();

function clientIp(req) {
    return req.socket.remoteAddress || 'unknown';
}

// returns null when the server or this client already runs too many games
function spawnGame(ip) {
    if (running >= MAX_GAMES || (perIp.get(ip) || 0) >= MAX_GAMES_PER_IP) return null;
    let game;
    try {
        game = pty.spawn(GAME, GAME_ARGS, {
            name: 'xterm-256color',
            cols: 100,
            rows: 35,
            cwd: ROOT,
            env: { ...process.env, TERM: 'xterm-256color', LANG: 'C.UTF-8' },
        });
    } catch (e) {
        console.error('Pelin käynnistys epäonnistui:', e.message);
        return null;
    }
    running++;
    perIp.set(ip, (perIp.get(ip) || 0) + 1);
    game.onExit(() => {
        running--;
        const n = (perIp.get(ip) || 1) - 1;
        if (n > 0) perIp.set(ip, n); else perIp.delete(ip);
    });
    return game;
}

const END_MSG = '\r\n\x1b[1;31m=== PELI PÄÄTTYI — päivitä sivu ===\x1b[0m\r\n';

// HTTP polling sessions for proxies that don't pass WebSockets.
const sessions = new Map();
const SESSION_IDLE_MS = 60000;

function newSession(ip) {
    const game = spawnGame(ip);
    if (!game) return null;
    const id = crypto.randomBytes(16).toString('hex');
    const s = { game, buf: '', waiter: null, done: false, seen: Date.now() };
    s.game.onData((d) => { s.buf += d; flush(s); });
    s.game.onExit(() => { s.buf += END_MSG; s.done = true; flush(s); });
    sessions.set(id, s);
    return id;
}

function flush(s) {
    if (!s.waiter || (!s.buf && !s.done)) return;
    const res = s.waiter;
    s.waiter = null;
    clearTimeout(res.timer);
    res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
    res.end(JSON.stringify({ data: s.buf, done: s.done }));
    s.buf = '';
}

setInterval(() => {
    const now = Date.now();
    for (const [id, s] of sessions) {
        if (now - s.seen > SESSION_IDLE_MS) {
            try { s.game.kill(); } catch (e) {}
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
            if (s.waiter === res) { s.waiter = null; res.writeHead(200, { 'Content-Type': 'application/json' }); res.end('{"data":""}'); }
        }, 20000);
        flush(s);
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
    const game = spawnGame(clientIp(req));
    if (!game) {
        ws.send('\r\n\x1b[1;31mPalvelin on täynnä. Yritä hetken päästä uudelleen.\x1b[0m\r\n');
        ws.close();
        return;
    }
    game.onData((d) => { if (ws.readyState === WebSocket.OPEN) ws.send(d); });
    game.onExit(() => {
        if (ws.readyState === WebSocket.OPEN) { ws.send(END_MSG); ws.close(); }
    });
    ws.on('message', (d) => game.write(d.toString()));
    ws.on('close', () => { try { game.kill(); } catch (e) {} });
});

server.listen(PORT, '0.0.0.0', () => {
    console.log(`PПLTSAЯI 2 running at http://localhost:${PORT}`);
});
