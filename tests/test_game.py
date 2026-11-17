#!/usr/bin/env python3
"""Drives the real game in browser mode (JSON frames over a pipe) and checks
frame rate, the visibility mask, restart with Z and that random play does not crash.
Run from the repository root: python3 tests/test_game.py"""
import json, os, random, subprocess, sys, threading, time

BIN = os.path.join(os.path.dirname(__file__), '..', 'pultsari')
failures = []


class Game:
    def __init__(self, extra=()):
        env = dict(os.environ, PULTSARI_WEB='1')
        self.p = subprocess.Popen([BIN, '--rt', '--los', '--anims', '--path', *extra], stdin=subprocess.PIPE,
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


checked = []


def check(name, ok, detail=''):
    print(('ok   ' if ok else 'FAIL ') + name + (f'  ({detail})' if detail and not ok else ''))
    checked.append(name)
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


def test_front_starts_with_the_ak_up_and_held_space_fires_a_burst():
    g = Game(['--front'])
    time.sleep(1); g.keys(' ', 0); time.sleep(1.5)
    up = [f for f in g.frames() if f.get('ak')]
    before = len(g.frames())
    # Space held: the page sends it every 60 ms while the rifle is up
    g.keys(' ' * 25, 0.06); time.sleep(0.5)
    after = g.frames()[before:]
    g.close()
    rounds = [f.get('hands', [''])[0] for f in after]
    shots = sum(1 for f in after for s in f.get('snd', []) if 'Rynn' in s[0] or 'Sarjatulta' in s[0])
    check('front_starts_with_the_ak_up_and_held_space_fires_a_burst', bool(up) and shots >= 3,
          f'ak frames {len(up)} shot frames {shots} hands {rounds[-1:] }')


def test_q_drinks_in_realtime():
    g = Game()
    time.sleep(1); g.keys(' ', 0); time.sleep(1.5)
    g.keys('q'); time.sleep(1)
    msgs = [f.get('msg', '') for f in g.frames()]
    menus = [e for e in g.events() if e.get('t') == 'invmenu']
    g.close()
    # a beggar may have taken the starting beer: then Player::drink says there is nothing to drink
    drank = ('Olutpullo meni', 'Baltika katosi', 'Ei oo ehtaa tavaraa')
    check('q_drinks_in_realtime', any(any(d in m for d in drank) for m in msgs) and not menus, f'menus {len(menus)}')


def test_arms_come_with_the_first_frame_and_only_on_change():
    g = Game()
    time.sleep(1); g.keys(' ', 0); time.sleep(1.5)
    fr = g.frames()
    g.close()
    first = fr[0] if fr else {}
    arms = first.get('arms') or []
    shaped = len(arms) == 2 and all(len(a) == 22 and all(len(r) == 14 for r in a) for a in arms)
    sent = sum(1 for f in fr if 'arms' in f)
    slots = all('slots' in f and 'inv' not in f and len(f.get('hands', [])) == 2 for f in fr)
    check('arms_come_with_the_first_frame_and_only_on_change', shaped and slots and len(fr) > 10 and sent <= len(fr) // 2,
          f'frames {len(fr)} with arms {sent} shaped {shaped} slots {slots}')


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


def test_front_trenches_and_the_assault():
    # the third front level: trenches drawn, the barrage and the Bradleys come, nothing crashes
    g = Game(['--front=3'])
    time.sleep(1); g.keys(' ', 0)
    random.seed(11)
    end = time.time() + 30
    while time.time() < end and g.p.poll() is None:
        g.keys(random.choice('wwssad '), 0.1)
    code = g.p.poll()
    frames = g.frames()
    text = g.buf.decode('utf-8', 'replace')
    g.close()
    rows = ''.join(run[0] for f in frames for row in f.get('rows', []) for run in row)
    msgs = ' '.join(f.get('msg', '') for f in frames)
    tracers = [e for e in g.events() if e.get('t') == 'tracer']
    check('front_trenches_are_drawn', '\u2592' in rows)
    check('front_aa_gun_stands_by_the_line', '\u0416' in rows)
    check('front_shots_send_tracers_to_the_page',
          len(tracers) > 0 and all(isinstance(e['x1'], (int, float)) for e in tracers), f'{len(tracers)} tracers')
    check('front_level_three_brings_the_barrage', 'TYKISTÖKESKITYS' in msgs or 'Bradley' in msgs, msgs[-200:])
    check('front_play_does_not_crash', code is None and 'Segmentation' not in text, f'exit {code}')


def test_captions_and_the_hands_intro():
    g = Game()
    time.sleep(1); g.keys(' ', 0); time.sleep(2)
    first = g.frames()
    # the hands are explained in captions at the bottom of the screen from the start
    caption = any('kätesi' in f.get('sub', '') for f in first)
    g.keys('h', 0); time.sleep(1.5)
    during = g.frames()[len(first):]
    opened = any(f.get('scene') and 'KÄDET' in f.get('lvl', '') for f in during)
    g.keys('h', 0); time.sleep(1)
    last = g.frames()[-1] if g.frames() else {}
    g.close()
    check('captions_explain_the_hands_at_the_start', caption)
    check('h_opens_the_hands_intro_and_closes_it', opened and not last.get('scene'),
          f'opened={opened} scene_after={last.get("scene")}')


if __name__ == '__main__':
    if not os.path.exists(BIN):
        sys.exit('build the game first: make')
    test_frame_rate()
    test_visibility_mask_is_sent()
    test_z_restarts()
    test_city_is_built_up()
    test_player_starts_on_leninski()
    test_q_drinks_in_realtime()
    test_front_starts_with_the_ak_up_and_held_space_fires_a_burst()
    test_arms_come_with_the_first_frame_and_only_on_change()
    test_captions_and_the_hands_intro()
    test_random_play_does_not_crash()
    test_front_trenches_and_the_assault()
    print(f'{len(checked) - len(failures)}/{len(checked)} game tests passed')
    sys.exit(1 if failures else 0)
