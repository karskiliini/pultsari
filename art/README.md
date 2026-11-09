# Pelin mainoskuva ASCII-taiteena

Kuva näkyy pelin alkuruudun taustalla. Selaimessa se on animoitu 200×150-versio
(`web/adart.js`), terminaalissa muotoon valituilla merkeillä tehty 100×75- tai
64×48-versio (`src/introart.cpp`). Terminaalissa käytetään suurinta versiota,
joka mahtuu ikkunaan.

| Tiedosto | Sisältö |
|---|---|
| `mainos-pullolla.jpg` | Mainoskuva, johon pullo on liitetty vanhemmasta versiosta |
| `mainos-100.txt` / `.ans` | 100×75, kirkkausramppi, mustavalkoinen / ANSI-värit |
| `mainos-200.txt` / `.ans` | 200×150, kirkkausramppi, mustavalkoinen / ANSI-värit |
| `mainos-shape-64` / `-100` `.txt` / `.ans` | Muotoon valitut merkit, etu- ja taustaväri (terminaali-intro) |
| `mainos-shape-64-anim.json` / `-100-anim.json` | Terminaali-intron liekki-, savu- ja palo-animaatio (48 kehystä, 12 fps) |
| `mainos-anim.html` | Animoitu 200×150-versio omana sivunaan (klikkaus: tauko / jatka) |
| `mainos-anim.json` | Animaation data: liekit, savu ja horisontin palot, 48 ruudun silmukka 12 fps |

Katselu truecolor-terminaalissa:

```bash
cat art/mainos-shape-100.ans
python3 art/tools/play_anim.py         # animaatio, vaatii 200x150-ikkunan
```

## Uudelleengenerointi

Skriptit eivät tarvitse lisäkirjastoja. JPEG muunnetaan BMP:ksi macOS:n `sips`-työkalulla.

```bash
sips -s format bmp art/mainos-pullolla.jpg --out /tmp/m.bmp

# selain: animoitu 200x150
python3 art/tools/ascii.py /tmp/m.bmp 200 art/mainos-200 0.9 0.5
python3 art/tools/gen_anim.py art/mainos-200.ans art/mainos-anim.json 48
python3 art/tools/build_html.py art/mainos-anim.json art/mainos-anim.html
python3 art/tools/build_webjs.py art/mainos-anim.json web/adart-data.js

# terminaali: muotoon valitut merkit
python3 art/tools/ascii_shape.py /tmp/m.bmp 100 art/mainos-shape-100.ans
python3 art/tools/ascii_shape.py /tmp/m.bmp 64 art/mainos-shape-64.ans
python3 art/tools/gen_shape_anim.py /tmp/m.bmp 100 art/mainos-shape-100-anim.json 48
python3 art/tools/gen_shape_anim.py /tmp/m.bmp 64 art/mainos-shape-64-anim.json 48
python3 art/tools/gen_cpp.py src/introart.cpp \
    art/mainos-shape-64.ans:art/mainos-shape-64-anim.json \
    art/mainos-shape-100.ans:art/mainos-shape-100-anim.json
```

`gen_shape_anim.py` maalaa liekin, savun ja palojen välkkeen kuvan pikselinäytteisiin
kehys kerrallaan ja sovittaa vain niiden koskemat ruudut uudelleen, joten animoidut
ruudut näyttävät samalta kuin muu kuva. Intro piirtää kehyksiä odotusten aikana eikä
koske ruutuihin, joiden päällä on tekstiä.

`ascii_shape.py` vertaa jokaisen merkin muotoa (`tools/glyphs4x8.json`, Menlo-fontista
mitattu musteen peitto 4×8-ruudukossa) kuvaan ja valitsee merkin sekä etu- ja
taustavärin pienimmän virheen mukaan. `composite.py` liitti pullon kuvaan
(`composite.py vanha.bmp uusi.bmp ulos.bmp 245 1225 400 1555 5 -30 14`).
