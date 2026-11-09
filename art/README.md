# Pultsari – mainoskuva ASCII-taiteena

| Tiedosto | Sisältö |
|---|---|
| `mainos-pullolla.jpg` | Uusi mainoskuva, johon pullo on liitetty vanhasta versiosta |
| `mainos-100.txt` / `.ans` | 100×75 merkkiä, mustavalkoinen / ANSI-värit |
| `mainos-200.txt` / `.ans` | 200×150 merkkiä, mustavalkoinen / ANSI-värit |
| `mainos-anim.html` | Värillinen 200×150-versio, jossa liekit, savu ja horisontin palot on animoitu. Tauko/jatko klikkaamalla |
| `mainos-anim.json` | Animaation data (48 ruudun silmukka, 12 fps) |

Katselu terminaalissa (truecolor, esim. iTerm2):

```bash
cat art/mainos-100.ans                 # still-kuva
python3 art/tools/play_anim.py         # animaatio, vaatii 200x150-ikkunan (pienennä fonttia)
```

## Uudelleengenerointi

`tools/`-skriptit eivät tarvitse lisäkirjastoja. JPEG muunnetaan BMP:ksi macOS:n `sips`-työkalulla.

```bash
sips -s format bmp art/mainos-pullolla.jpg --out /tmp/m.bmp
python3 art/tools/ascii.py /tmp/m.bmp 200 art/mainos-200 0.9 0.5
python3 art/tools/gen_anim.py art/mainos-200.ans art/mainos-anim.json 48
python3 art/tools/build_html.py art/mainos-anim.json art/mainos-anim.html
```

`ascii.py` kirjoittaa myös `.html`-esikatselun, jota ei tarvita repossa. `composite.py` on työkalu, jolla pullo liitettiin
(`composite.py vanha.bmp uusi.bmp ulos.bmp 245 1225 400 1555 5 -30 14`).
