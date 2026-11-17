# Juoksuhaudat: suoja, kiväärit, droonit, tykistö ja Bradley-hyökkäys

Päivä: 2026-11-17 · Haara: `pultsari2`

## Miksi

Rintama (`--front`, `warStage` 1–3) on nyt tyhjä 400×200-kenttä, jossa on muuri tai kaksi,
skinheadeja ja omia sotilaita. Siinä ei ole paikkaa, joka antaisi suojaa, eikä mitään,
joka pakottaisi valintoihin. Käyttäjä haluaa, että rintamalla tuntuu rintamalta:
haudassa olet suojassa luodeilta mutta et droneilta. Kun nouset ylös, sinua ammutaan,
ja mitä kauemmin olet ylhäällä, sitä useampi drooni jahtaa sinua. Tasot pahenevat
yksi kerrallaan, ja viimeisellä tasolla pelaaja joutuu vetäytymään vyöryttävän
vihollisen edeltä, kunnes vuorovaihdon käsky tulee.

## Tavoitteet

- Juoksuhaudat ovat selvästi näkyviä, leveitä uria, ja ne generoidaan proseduraalisesti
  joka tasolle hieman eri tavalla: siksak, haarat, ampumapesäkkeet ja yhteyshaudat selustaan.
- **Haudassa**: kivääriluodit eivät osu, mutta FPV-droonit iskevät sinne silti.
- **Haudan ulkopuolella**: vihollisen kiväärimiehet ampuvat omista haudoistaan, ja
  altistusmittari kasvaa. Mitä korkeampi altistus, sitä useampi drooni valitsee pelaajan.
- Tasot:
  1. Droonit ja väijyvät kiväärimiehet.
  2. Kuten taso 1, ja lisäksi tykistö.
  3. Droonit ja tykistökeskitys. Kun keskitys loppuu, Bradley-vaunut ajavat
     hautaverkoston itäreunaan, ukrainalaiset jalkaväkimiehet nousevat kyydistä ja alkavat
     vyöryttää hautoja AK:lla, ja Bradleyn konetykki ampuu sirpaleammuksia. Pelaaja joutuu
     käytännössä vetäytymään hautoja pitkin länteen, kunnes vuorovaihdon käsky tulee.
- Altistus näkyy statusrivillä, ja tekstitykset kertovat sen nousemisesta.

## Ei-tavoitteet

- Rintaman ulkopuolinen peli (Moskova, Kreml) ei muutu.
- Ei uutta näppäintä esimerkiksi kyyristymiselle. Suoja tulee siitä, missä seisot.
- Ei omia Bradleyitä tai tankkeja, joilla pelaaja voisi ajaa.

## Anti-tavoitteet (epäonnistuminen, vaikka toimisi)

- Haudat eivät erotu kartalla. Pelaaja ei tiedä, onko hän suojassa vai ei.
- Kuolema tulee tyhjästä. Jokaisesta laukauksesta pitää tulla viesti tai ääni ja
  suuliekki, jotta pelaaja ymmärtää, miksi häntä ammutaan.
- Taso 3 on mahdoton: vetäytymisreitti länteen on aina olemassa (yhteyshauta).
- Suoja tekee haudasta täysin turvallisen. Droonit tulevat sinne silti.

## Rajoitteet

- C++, sekä terminaali- että web-tulostin (`printerterm.cpp`, `printerweb.cpp`, yhteinen `printer.cpp`).
- Reaaliaika: maailman tick noin 2,5/s (`NPC_INTERVAL = 8`, `FRAME_MS = 50`), droonit framekohtaisesti.
- Vuorovaihto ja selusta: `Level::warTick`, `WAR_SHIFT_TICKS = 120`, `REAR_X = 1` (vasen reuna).
- Pelaaja aloittaa kohdasta (6, 153) (`common::PLAYER_START_X/Y`).
- Yksikkötestit `tests/unit_tests.cpp` ja pelitestit (`test_game.py`) ajetaan ennen pushia.

## Toteutustavat

1. **Hautakerros kentän päällä (valittu).** `Level`illä on 400×200-taulukko
   (`std::vector<uint8_t> trench`, arvot: ei hautaa, oma hauta, vihollisen hauta, kraatteri).
   Generaattori ajetaan sotatasolle. `inTrench(c)` / `inEnemyTrench(c)` ratkaisevat suojan,
   ja tulostin piirtää kerroksen maan väriksi. Kevyt. Droonit, kiväärit, tykistö ja
   jalkaväen tekoäly käyttävät samaa kyselyä.
2. *Haudat rakennuksina* (kuten `Seina`): seinät rikkoisivat kävelyn, polunhaun ja
   `underOpenSky`-logiikan. Hylätty.
3. *Haudat maassa olevina esineinä*: tuhansia esineitä hidastaisi peliä ja sotkisi
   poiminnan. Hylätty.

Käyttäjä puhui lopputuloksesta, joten toteutustapa on Clauden valinta.

## Suunnittelu

### Kartta ja suunta

- **Selusta on lännessä (vasen reuna), vihollinen idässä.** FPV-droonit tulevat nyt
  lännestä (`fpvTick`, kulma π). Ne käännetään tulemaan idästä. *(Clauden valinta:
  nykyinen suunta on ristiriidassa selustan kanssa.)*
- Oma pääasema kulkee pohjois–etelä-suunnassa noin kohdassa x≈12, vihollisen linja
  noin kohdassa x≈70. Väliin jää ei-kenenkään-maa: kraattereita ja piikkilankaa
  (piikkilanka hidastaa ja raapii). Etäisyys mahtuu ruudulle. *(Clauden valinta)*
- Kenttä on 400 leveä, mutta toiminta keskittyy vasempaan reunaan (x 0–90). Kauempana
  idässä on vihollisen selustaa: sinne meneminen on itsemurha (kiväärituli ilman hautaa).

### Generointi (`src/trenches.cpp`, `include/trenches.hpp`)

- **Pääasema**: satunnaiskävely etelästä pohjoiseen koko kartan korkeudelta. x heilahtelee
  ±3 lohkoittain 4–8 ruudun välein (siksak, traverssit), ja hauta on 3 ruutua leveä.
- **Ampumapesäkkeet**: 1–2 ruudun ulokkeet itään 10–20 ruudun välein.
- **Yhteyshaudat**: 2–4 kappaletta pääasemasta länteen x=0:aan asti, 2 ruutua leveitä
  ja kiemurtelevia. Yksi kulkee aina pelaajan aloituskohdan läheltä, joten vetäytymisreitti
  on taattu.
- **Etulinjan haarat**: 1–3 umpikujaa itään ei-kenenkään-maahan.
- **Vihollisen linja**: sama algoritmi kohdassa x≈70 ± 5. Pesäkkeisiin sijoitetaan kiväärimiehet.
- **Kraatterit**: 20–40 satunnaista kraatteria ei-kenenkään-maalle. Kraatterissa on puolisuoja
  (kiväärin osuma puolittuu). *(Clauden valinta)*
- Siemenluku tulee `std::rand`ista kuten muussakin generoinnissa, joten testit voivat kiinnittää sen.
- Pelaajan aloituspaikka siirretään sotatasolla lähimpään oman haudan ruutuun. Omat sotilaat
  (`sotilas`) sijoitetaan oman haudan ruutuihin, skinheadit (lihamyrsky) myös. Betonimuurit
  poistetaan sotatasolta. *(Clauden valinta)*

### Piirto

- Oma hauta: tumma maa (`48;5;94`/`48;5;58`-sävyinen ruskea). Reunat ovat hiekkasäkkejä
  `▒` (vaalea khaki), ja pohjalla on lankkuja `=` harvakseltaan.
- Vihollisen hauta: samaa, mutta sumu tai etäisyys himmentää. Vihollinen näkyy vasta
  suuliekin hetkellä tai kun hän nousee ylös.
- Kraatteri: `o`/`O` tummanruskea. Piikkilanka: `#` harmaa / `x`.
- Kerros piirretään maan väriksi ennen hahmoja, kuten tiet (`printer.cpp`, tieväritys).

### Altistus (`Player::exposure`, 0–100)

- Haudan ulkopuolella: +4 / maailman tick, kraatterissa +2. Haudassa: −2 / tick.
- Kynnysten ylittyessä tekstitys: 25 "[Sinut on huomattu.]", 50 "[Kiikarit kiiltävät idässä.]",
  75 "[Droonien pilotit puhuvat sinusta.]".
- Statusrivillä mittari `ALTISTUS ▮▮▮▯▯` vain sotatasolla.

### Kiväärimiehet (taso 1+)

- Uusi henkilötyyppi `ukrainalainen` (merkki `u`, oliivinvihreä/keltainen). Väijyvässä tilassa
  hän pysyy vihollisen haudassa eikä näy, paitsi ampuessaan.
- Joka maailman tickillä jokainen väijyjä, jolla on näköyhteys pelaajaan alle 70 ruudun päähän,
  ampuu todennäköisyydellä `p = 2 % + altistus × 0,25 %` (ulkona; haudassa 0). Osuma-
  todennäköisyys putoaa etäisyyden mukana (60 % lähellä, 15 % kaukana), ja vahinko on 2–4.
  *(Luvut ovat Clauden valinta, ja ne viritetään pelitesteissä.)*
- Joka laukauksesta: suuliekki-animaatio `*` ampujan kohdalla, ääni ("PAM!" / "kaukainen
  laukaus") ja ohilaukauksesta "Luoti viuhuu ohi!"
- Ampujat ampuvat samoin omia sotilaita ja skinheadeja, jotka ovat haudan ulkopuolella
  (lihamyrsky kuolee ei-kenenkään-maalle).
- Pelaaja voi ampua heitä AK:lla. Haudassa olevaan vihollisen osuma puolittuu.

### Droonit

- `fpvTick`: pelaajan valintaosuus riippuu siitä, onko hän haudassa vai ulkona (alla).
  Uuden droonin tulotiheys kasvaa altistuksen mukana (`random(12)` →
  `random(12 + altistus / 4)`).
- **Haudoissa pörrää niin paljon drooneja, että siellä on vaikea olla** (käyttäjä: "On
  helpompaa nousta hetkeksi ylös juoksuhaudasta kuin tulla pommitetuksi.") Tästä syntyy
  keinulauta, joka ratkaisee, miksi haudasta noustaan:
  - Haudassa olevaan pelaajaan kohdistuu korkea perusosuus (50 %). Droonien tulotiheys
    hautaverkoston päällä on suuri (`FPV_MAX` sotatasolla 5 → 7, uusi drooni noin 2 s välein).
    Mitä pidempään pelaaja seisoo samassa hautalohkossa (5×5 alue), sitä varmemmin pilotit
    löytävät hänet: `hautapaine` +3/tick samassa lohkossa, ja liikkuminen haudassa nollaa
    sen hitaasti.
  - Ulos noussut pelaaja on aluksi droneille vaikeampi kohde (10 %), koska pilotit
    tähyilevät hautoja. Kun altistus kasvaa, osuus nousee (`10 % + altistus × 0,8 %`, enintään 90 %),
    ja samalla kiväärimiehet ampuvat.
  - Syöksyvä drooni lukittuu ruutuun, jossa pelaaja seisoi. Hyppy haudasta ylös (tai takaisin)
    väistää sen, mikä on se "hetki ylhäällä".
  - Tavoite: optimaalinen pelaaminen on rytmiä. Ollaan haudassa, kunnes drooni syöksyy tai
    hautapaine nousee, ja sitten noustaan hetkeksi ylös, kunnes luodit alkavat viuhua.

### Tykistö (taso 2+)

- `EnvironmentEvents::artilleryBarrage` kohdistetaan sotatasolla oman haudan läheisyyteen
  satunnaisen koko kartan sijaan. Ensimmäisenä tulee varoitus ("Ulvontaa idästä!" ja
  tekstitys) 2 tickiä ennen iskuja.
- Hauta suojaa: sirpaleiden säde 3 → 1, jos kohde on haudassa eikä suoraan osuman ruudussa.
  *(Clauden valinta)*
- Osumista jää kraatteri karttaan, ja oman haudan osumasta tulee kraatteri hautaan.

### Taso 3: keskitys ja hyökkäys

- Vaihe A (tickit 0–30): tavalliset droonit ja kiväärit.
- Vaihe B (30–55): **tykistökeskitys**. Iskuja tulee 2–4 joka tickillä oman linjan päälle,
  ja kiväärimiehet ovat hiljaa.
- Vaihe C (55→): **hyökkäys**. 2–3 Bradleytä (`Vehicle`-aliluokka, 3×2 ruutua, `▓` hiekanväri)
  ajaa idästä 6–10 ruudun päähän oman haudan itäreunasta ja pysähtyy. Kustakin nousee
  4–6 `ukrainalainen`-jalkaväkimiestä hyökkäystilassa:
  - Hyökkääjät liikkuvat kohti omaa hautaa, hyppäävät sinne ja etenevät haudassa pelaajaa
    ja omia sotilaita kohti (vyörytys). Ne ampuvat AK-sarjoja kuten `Sotilas::fire`.
  - Bradleyn konetykki ampuu 3 tickin välein sirpaleammuksen (räjähdys säteellä 1,
    `common::boomEvent`) näkyvissä olevaa kohdetta kohti, ensisijaisesti pelaajaa.
  - Kranaatti, joka osuu Bradleyhin, tuhoaa sen (`wrecked`). AK ei tuhoa sitä.
- **Vetäytyä saa vasta, kun upseeri on kaatunut taistelussa** (käyttäjä). Tasolla 3
  pelaajan haudassa on upseeri (`Sotilas`, `officer = true`, `S`), joka pitää asemaa
  oman haudan itäisimmässä osassa eikä peräänny.
  - Niin kauan kuin upseeri elää, selusta ei aukea (`WAR_SHIFT_TICKS` ei avaa sitä
    tasolla 3). Jos pelaaja on yli 12 ruutua upseerista länteen, upseeri huutaa
    radioon ("Ei askeltakaan taaksepäin!"), ja **omat konekiväärit ampuvat häntä
    kentän länsilaidalta** (käyttäjä). Kyse on sulkuosastosta: 2–3 konekivääripesäkettä
    (`KK`, punainen) kohdassa x=0–2, yhteyshautojen suilla ja niiden välissä. Ne ampuvat
    sarjan 2 tickin välein lähimmästä pesäkkeestä. Hauta ei suojaa tältä tulelta (se tulee
    takaa, haudan suuntaisesti). Osuma 40 %, vahinko 2–4 per sarja. Sama tuli kohdistuu
    myös perääntyviin omiin sotilaisiin.
  - Taso 3 jakaa näin pelaajan puristuksiin: edessä hyökkääjät, takana omat konekiväärit.
  - Kun upseeri kuolee (kenen tahansa käsissä), tulee radioviesti "Upseeri kaatui!
    Vetäytykää!", selusta aukeaa heti, ja pelaaja vetäytyy yhteyshautoja pitkin länteen
    hyökkääjien vyöryttäessä perässä. *(Clauden valinta: myös pelaajan tappama upseeri
    avaa selustan, mutta silloin muut sotilaat ampuvat pelaajaa.)*
  - Hyökkääjät ja Bradleyn konetykki pitävät upseeria yhtenä kohteistaan, joten hän kaatuu
    yleensä 20–40 tickin kuluessa hyökkäyksen alusta.

### Testit

- Generaattori: pääasema on yhtenäinen etelästä pohjoiseen. Jokin yhteyshauta yltää x=0:aan
  ja on yhtenäinen pääasemaan (BFS). Pelaajan aloituskohta on haudassa. Vihollisen linja
  on eri alueella.
- Altistus: kasvaa ulkona, laskee haudassa, rajat 0–100.
- Kivääri: haudassa olevaan pelaajaan ei koskaan ammuta, ulkona ammutaan (kiinnitetty siemen).
- Droonien kohdevalinta painottuu pelaajaan altistuksen kasvaessa (tilastotesti).
- Taso 3: keskitys alkaa ja loppuu, Bradleyt ilmestyvät ja jalkaväki nousee kyydistä.
- Pelitesti `--front`: peli käynnistyy, haudat piirtyvät, eikä peli kaadu 200 tickin aikana.

## Tarkastuksessa ratkaistut (Clauden valinnat)

- **Puolet:** pelaaja on venäläinen (`isRussianSoldier`). Ukrainalaiset (`ukrainalainen`,
  FPV:t, Bradleyt, tykistö) tulevat idästä, ja selusta on lännessä. `fpv.cpp`:n kommentti
  ja tulokulma käännetään.
- **Lihamyrsky (`meatAssault`):** säilyy, mutta skinheadit nousevat omasta haudasta ja
  rynnäköivät itään, suoraan kiväärimiesten tuleen. Nykyään ne tulevat oikeasta reunasta.
- **Tykistö:** uusi ajastin `frontline.cpp`:hen (`warArtilleryTick`), joka toimii vain tasoilla
  2–3. Satunnaistapahtuman `artilleryBarrage` sotahaara poistetaan, jotta tasolla 1 ei
  tule tykistöä. Vahinko lasketaan yhteisellä funktiolla `shellBurst(at, radius)`, jota
  myös Bradleyn sirpaleammus käyttää. `common::boomEvent` on pelkkä webin visuaali.
- **Näköyhteys:** Bresenham-säde ampujasta kohteeseen, jonka rakennusten seinät katkaisevat,
  ja enintään 70 ruutua. Haudassa oleva kohde ei näy kiväärimiehelle lainkaan.
- **Web:** `game.js` saa ruudut valmiina C++-tulostimelta, joten muutos riittää `printer.cpp`:hen.
- **Hyökkääjät haudassa:** jahtaavat lähintä näkyvää venäläistä ja liikkuvat polulla, joka
  suosii hautaruutuja. Koska kohteet vetäytyvät länteen, tuloksena on vyörytys länteen.
- **AK:** `issueRifle` tasolla 1, ja ase säilyy inventaariossa tasoille 2–3 (nykyinen toiminta).

## Rakentajalle jätetty

- Tarkat värikoodit ja glyyfit, kunhan haudat erottuvat selvästi sekä terminaalissa että webissä.
- Lukujen viritys pelitestien perusteella: osumatodennäköisyydet, altistuksen kasvu ja
  tykistön tiheys.
- Jalkaväen polunhaun yksityiskohdat haudoissa: olemassa oleva `pathFinding` maskilla,
  joka suosii hautaruutuja.
