#!/usr/bin/env python3
"""Drives the real game in browser mode (JSON frames over a pipe) and checks
frame rate, the visibility mask, restart with Z and that random play does not crash.
Run from the repository root: python3 tests/test_game.py"""
import json, os, random, subprocess, sys, threading, time

BIN = os.path.join(os.path.dirname(__file__), '..', 'pultsari')
failures = []


class Game:
    def __init__(self):
        env = dict(os.environ, PULTSARI_WEB='1')
        self.p = subprocess.Popen([BIN, '--rt', '--los', '--anims', '--path'], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
        self.buf = b''
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        while True:
            chunk = self.p.stdout.read1(65536) if hasattr(self.p.stdout, 'read1') else self.p.stdout.read(4096)
            if not chunk:
                return
            self.buf += chunk

    def keys(self, s, delay=0.15):
        for ch in s:
            self.p.stdin.write(ch.encode()); self.p.stdin.flush(); time.sleep(delay)

    def events(self):
        out = []
        for part in self.buf.decode('utf-8', 'replace').split('\x1e'):
            part = part.strip()
            if part.startswith('{'):
                try: out.append(json.loads(part))
                except ValueError: pass
        return out

    def frames(self):
        return [e for e in self.events() if e.get('t') == 'frame']

    def close(self):
        self.p.kill(); self.p.wait()


def check(name, ok, detail=''):
    print(('ok   ' if ok else 'FAIL ') + name + (f'  ({detail})' if detail and not ok else ''))
    if not ok: failures.append(name)


def test_frame_rate():
    g = Game()
    time.sleep(1); g.keys(' ', 0)
    n0 = len(g.frames()); time.sleep(10); n1 = len(g.frames())
    g.close()
    fps = (n1 - n0) / 10
    check('frame_rate_is_twenty', 18.5 <= fps <= 21, f'{fps:.1f} fps')


def test_visibility_mask_is_sent():
    g = Game()
    time.sleep(1); g.keys(' ', 0); time.sleep(2)
    fr = g.frames()
    g.close()
    last = fr[-1] if fr else {}
    vis = last.get('vis', [])
    ok = len(vis) == len(last.get('rows', [])) and vis and all(len(r) == (last['cols'] + 3) // 4 for r in vis)
    # some cells are seen and some are not
    flat = ''.join(vis)
    check('visibility_mask_is_sent', bool(ok) and flat.strip('0') != '' and flat.strip('f') != '')


def test_z_restarts():
    g = Game()
    time.sleep(1); g.keys(' ', 0); time.sleep(1.5)
    g.keys('ddd'); g.keys('z', 0); time.sleep(2)
    alive = g.p.poll() is None
    msgs = [f.get('msg', '') for f in g.frames()]
    g.close()
    check('z_restarts_the_game', alive and any('Uusi pultsari' in m for m in msgs))


def test_city_is_built_up():
    g = Game()
    time.sleep(1); g.keys(' ', 0); time.sleep(3)
    maps = [e for e in g.events() if e.get('t') == 'map']
    g.close()
    kinds = [b[5] for b in maps[-1]['b']] if maps else []
    PRISON, CLOSED_SHOP, SHOP, METRO, FLATS = 3, 10, 13, 14, 15
    ok = (kinds.count(PRISON) >= 1 and kinds.count(METRO) >= 4 and kinds.count(CLOSED_SHOP) >= 20
          and kinds.count(SHOP) >= 3 and kinds.count(FLATS) >= 30)
    check('city_is_built_up', ok, f'prison {kinds.count(PRISON)} metro {kinds.count(METRO)} closed {kinds.count(CLOSED_SHOP)} '
                                  f'shops {kinds.count(SHOP)} flats {kinds.count(FLATS)}')


def test_player_starts_on_leninski():
    g = Game()
    time.sleep(1); g.keys(' ', 0); time.sleep(1)
    fr = g.frames()
    g.close()
    first = fr[0] if fr else {}
    # the bottom-left corner of the city, by Leninski prospekt
    check('player_starts_on_leninski', first.get('pos') == [6, 153] and first.get('lvl') == 'LENINSKI PR.',
          f"pos {first.get('pos')} lvl {first.get('lvl')}")


def test_random_play_does_not_crash():
    g = Game()
    time.sleep(1); g.keys(' ', 0)
    random.seed(7)
    end = time.time() + 25
    while time.time() < end and g.p.poll() is None:
        g.keys(random.choice('wasdwasdwasdqef \x1b12'), 0.06)
    code = g.p.poll()
    text = g.buf.decode('utf-8', 'replace')
    g.close()
    check('random_play_does_not_crash', code is None and 'Segmentation' not in text and 'exception' not in text,
          f'exit {code}')


if __name__ == '__main__':
    if not os.path.exists(BIN):
        sys.exit('build the game first: make')
    test_frame_rate()
    test_visibility_mask_is_sent()
    test_z_restarts()
    test_city_is_built_up()
    test_player_starts_on_leninski()
    test_random_play_does_not_crash()
    print(f'{6 - len(failures)}/6 game tests passed')
    sys.exit(1 if failures else 0)
