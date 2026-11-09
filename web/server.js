const http = require('http');
const fs = require('fs');
const path = require('path');
const pty = require('node-pty');
const WebSocket = require('ws');

const PORT = process.env.PORT || 3000;

const server = http.createServer((req, res) => {
    let filePath;
    if (req.url === '/' || req.url === '/index.html') {
        filePath = path.join(__dirname, 'index.html');
    } else {
        filePath = path.join(__dirname, req.url);
    }

    const ext = path.extname(filePath);
    const types = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css' };
    const contentType = types[ext] || 'application/octet-stream';

    fs.readFile(filePath, (err, data) => {
        if (err) {
            res.writeHead(404);
            res.end('Not found');
            return;
        }
        res.writeHead(200, { 'Content-Type': contentType });
        res.end(data);
    });
});

const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    console.log('Player connected');

    const gamePath = path.join(__dirname, '..', 'pultsari');
    const shell = pty.spawn(gamePath, ['--rt', '--los', '--anims', '--path'], {
        name: 'xterm-256color',
        cols: 100,
        rows: 35,
        cwd: path.join(__dirname, '..'),
        env: { ...process.env, TERM: 'xterm-256color' }
    });

    shell.onData((data) => {
        try { ws.send(data); } catch (e) {}
    });

    shell.onExit(({ exitCode }) => {
        console.log('Game exited:', exitCode);
        try { ws.send('\r\n\x1b[1;33m=== PELI PÄÄTTYI ===\x1b[0m\r\n'); } catch (e) {}
        try { ws.close(); } catch (e) {}
    });

    ws.on('message', (data) => {
        shell.write(data.toString());
    });

    ws.on('close', () => {
        console.log('Player disconnected');
        shell.kill();
    });
});

server.listen(PORT, '0.0.0.0', () => {
    console.log(`PПLTSAЯI 2 running at http://localhost:${PORT}`);
});
