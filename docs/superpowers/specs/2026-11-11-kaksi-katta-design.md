# Kaksi kättä: inventaario, votkamittari ja käsikuvat

Päivä: 2026-11-11 · Haara: `pultsari2`

## Miksi

Votkapullo on pelin tärkein aarre. Krapula tappaa jo nyt, kun promillet putoavat
alle 0,1 (`pultsari.cpp`, "Tappajasi: krapula"), joten pullossa jäljellä oleva votka
on käytännössä elämänmittari ja tärkeämpi kuin voima. Sen pitää näkyä.

Samalla inventaario muuttuu fyysiseksi: pelaajalla on mukana vain se, mikä mahtuu
kahteen käteen. Silloin syntyy juopon arkisia valintoja. Pidätkö pulloa vai
pistoolia? Juotko pullon tyhjäksi, jotta saat AK:n käteen?

Kuvat ovat juonen keskiössä, ja tyylireferenssinä on alkukuva (`art/mainos-pullolla.jpg`):
repaleinen ruskeanvihreä takki, likaiset luiset kädet, toisessa kädessä pullo ja
toisessa pistooli.

## Tavoitteet

- Kartan vasemmalla puolella näkyy vasen käsivarsi, oikealla oikea, ASCII-grafiikkana.
- Kädessä näkyy, mitä siinä on: votkapullo nestepinnan kanssa, pistooli tai tyhjä koura.
- Inventaario = kaksi kättä + yksi taskupaikka + vyö (vain pistoolille) + selkä (vain AK:lle).
- Votkapullossa on 6 huikkaa. `q` juo kädessä olevasta pullosta 1/6, ja nestepinta laskee kuvassa.
- Olutpullo juodaan kerralla tyhjäksi.
- Peli antaa vinkkejä ja hauskoja kommentteja.

## Ei-tavoitteet

- Inventaariota ei laajenneta. Ei reppuja, ei kasseja, ei useita taskuja.
  Käyttäjä: "Ei muuteta inventorya liian laajaksi."
- Raha ja kopeekat eivät ole tavaroita, vaan ne pysyvät tilastoina kuten nyt (`stats.money`, `stats.kopecks`).

## Anti-tavoitteet (epäonnistuminen, vaikka toimisi)

- Votkamittari ei näy selvästi tai jää hukkaan kuvan sisään.
- Inventaarion hallinta muuttuu näpräämiseksi (monia valikoita, monta näppäintä).
- Kuvat eivät näytä samalta repaleiselta juopolta (hihat eri paria, epäselvä ase).

## Paikat

| Paikka | Mitä mahtuu | Huom. |
|---|---|---|
| Vasen käsi | 1 tavara | |
| Oikea käsi | 1 tavara | |
| Tasku | 1 pieni tavara | Esim. lipas. Kun lipas on taskussa, muuta ei mahdu. |
| Vyö | pistooli | Vain jos pelaaja on löytänyt vyön sotilaalta. |
| Selkä | AK-47 (`rynkky`) | Aina käytettävissä. |

Yksi tavara tarkoittaa yhtä kappaletta: yksi pullo, yksi kivi, yksi makkara.

### Tavaroiden luokat

- **Kädessä kannettavat:** olut (`kalja`), Jaguar (`lonkka`), votka, tyhjä pullo,
  makkara, vobla, omena, banaani, ketjut, veitsi, kivi, pamppu, kranaatti, pistooli, AK
  (AK kädessä vie molemmat kädet).
- **Taskuun mahtuvat (pienet):** lipas, kranaatti, propusk, veitsi.
  *(Lista on builderin oletus; käyttäjä nimesi vain lippaan.)*
- **Verkkarit (`bootsit`)** ovat vaate, eivät tavara. Builder päättää, puetaanko ne heti
  vai pidetäänkö kädessä.

## Votkapullo

- Kädessä olevalla votkapullolla on täyttöaste 0–6.
- `q`: jos jommassakummassa kädessä on votkapullo, jossa on nestettä, juodaan siitä yksi
  huikka. Täyttöaste laskee yhdellä, ja promillet nousevat +1 (0,1 ‰). Kuusi huikkaa antaa
  saman +0,6 ‰ kuin koko pullo nykyään. Lisäksi tulee nykyinen pieni nälkävaikutus.
- Kun pullo tyhjenee, se pysyy kädessä **tyhjänä pullona**. Sen voi heittää tai palauttaa
  hypermarketin luukulle 20 kopeekalla, kuten nykyiset `pullot`.
- Kusipullo (`pissVodka`) paljastuu ensimmäisellä huikalla, ja seuraukset ovat samat kuin nyt.
  Pullo pysyy kädessä tyhjänä. *(Builderin oletus.)*
- Jos votkaa ei ole kädessä, `q` juo kädessä olevan oluen tai Jaguarin kerralla tyhjäksi.
  Tyhjä pullo jää käteen.
- Juoman valinta, jos molemmissa käsissä on juotavaa: votka ensin, sitten vasen käsi.
  *(Builderin oletus.)*

## Pistooli, vyö ja AK

- Pistooli tarvitsee yhden vapaan käden. Jos kädessä on pullo, pullo pitää ensin heittää
  pois tai juoda ja heittää, ja vasta sitten pistoolin saa käteen.
- **Vyö** saadaan sotilaalta (ryöstämällä/löytämällä kaatuneelta, esim. `player.cpp`:n
  ruumiinryöstö, jossa nyt arvotaan RYNKKY). Ilman vyötä pistooli on vain kädessä
  kannettava tavara. Vyön kanssa pistooli kulkee vyöllä ja vapauttaa kädet.
- **AK-47** kulkee selässä. Sillä voi ampua vasta, kun **molemmat kädet ovat tyhjät**.
- **Ammukset (käyttäjän päätös):** ladattu ase sisältää omat panoksensa (Makarov 8, AK 30).
  Taskuun mahtuu yksi varalipas, joka on yksi lataus kumpaan tahansa aseeseen.
  *(Lippaan ja aseen yhteensopivuus on builderin valinta; yksinkertaisin tapa on, että lipas
  täyttää aseen.)* Löydetyt ja ostetut ammukset ovat lippaita. Kun lipas on käytetty,
  tasku vapautuu. Nykyinen `ammukset`-laskuri poistuu ja korvataan aseen panoksilla.

## Kaatuminen ja tasku

- Kun pelaaja kaatuu (humalassa "Jalat pettävät..." `Player::updateCrawling`, kaatuminen
  bussin lattialle `vehicle.cpp`), taskusta voi pudota tavara tietyllä todennäköisyydellä.
  *(Todennäköisyys on builderin valinta, esim. 25 %.)*
- Pudonnut tavara näkyy heti maassa, mutta pelaaja saa siitä viestin vasta hetken päästä
  (muutaman vuoron viiveellä), esimerkiksi "Hetkinen... tasku tuntuu kevyeltä!"

## Poimiminen, ostaminen, varkaat

- Tavaran päälle kävellessä se poimitaan vapaaseen käteen (tai taskuun, jos se on pieni,
  kädet ovat täynnä ja tasku on tyhjä). Jos kädet ovat täynnä, tavara jää maahan ja peli
  sanoo: "Kädet ovat täynnä. Juo tai heitä jotain pois."
- Kaupat ja myyjät: ostos vaatii vapaan paikan. Muuten myyjä kieltäytyy ja rahat jäävät.
- Pullovarkaat ja muut, jotka nyt vievät tavaraa `stealable()`-listan kautta, vievät sen kädestä.

## Varastaminen kaupasta (käyttäjän)

- Kaupasta voi varastaa. Varastettu tavara menee käteen, joten se vaatii vapaan paikan.
- Myyjä on fanaattinen: kun hän huomaa varkauden, hän tulee kimppuun ja jahtaa pelaajaa
  kadulla. Jos hänellä on ase, edes nitoja, hän käyttää sitä säälittä. Joskus hänellä on
  veitsi, harvoin jopa pistooli.
- *(Builderin oletukset: kauppavalikossa `v` + numero varastaa. Myyjä huomaa varkauden
  70 %:n todennäköisyydellä. Myyjän ase arvotaan: nitoja 75 %, veitsi 20 %, pistooli 5 %.
  Kiinni jääneen myyjä ei enää myy samassa kaupassa sillä tasolla.)*

## Ohjaus

Nykyiset näppäimet säilyvät: `q` juo, `e` syö, `t`/välilyönti heittää, Tab avaa inventaarion.
Inventaarioruutu (`printInventoryOverlay`, web `invmenu`) näyttää vain paikat (vasen,
oikea, tasku, vyö, selkä), joten listasta tulee lyhyt. Heittäminen ja käyttö valitaan
siitä. Ase otetaan käteen nykyisellä tavalla (pistoolin nosto, rynkyn tähtäys), ja jos
kädet ovat varattuina, peli vinkkaa mitä tehdä.

## Kuvat

- Jokainen kuva on 14 merkkiä leveä ja 22 riviä korkea, pelkkää ASCIIta.
  Käsivarsi roikkuu paneelin yläreunasta, hiha on ylhäällä ja koura alaosassa, kuten julisteessa.
- Variantit:
  - vasen: tyhjä, votkapullo (täyttöaste 0–6 piirretään nesteriveinä), muu tavara
  - oikea: tyhjä, pistooli, muu tavara
  - AK kädessä: molemmat paneelit näyttävät rynkyn (builder piirtää)
- Grokin luonnokset: `docs/superpowers/specs/2026-11-11-kaksi-katta-art.md`.
  Pullokuvassa nesteet ovat riveillä 16–21 ja sarakkeissa 3–8: ylin nesterivi `~`, muut `#`,
  ja tyhjenevät rivit muuttuvat välilyönneiksi ylhäältä alas.
  Pistoolikuva piirretään uudelleen, ja hihat yhtenäistetään samaksi repaleiseksi takiksi.
- Muut tavarat (kivi, makkara, olut…) voivat aluksi näkyä yleisellä nyrkkikuvalla ja nimellä
  kuvan alla. Omat kuvat ovat builderin harkinnassa.

### Hihat repeytyvät vaurion mukaan (käyttäjän)

Mitä enemmän pelaaja ottaa vauriota, sitä repaleisemmiksi hihat muuttuvat. Lähes kuolleena
hihat ovat käytännössä revitty pois, ja kädet ovat verillä. Kuva kootaan kahdesta osasta:
yläosan hiha valitaan voiman mukaan ja alaosan koura sen mukaan, mitä kädessä on.
Vaiheet (`INITIAL_HP` = 10):

| Voima | Hiha |
|---|---|
| ≥ 8 | repaleinen takki (julisteen tyyli) |
| 5–7 | lisää reikiä, roikkuvia riekaleita |
| 2–4 | hiha revitty kyynärpäästä, paljas käsivarsi |
| 0–1 | hiha lähes poissa, käsivarressa ja kourassa verta (webissä punaisella) |

### Uusi takki = lisää voimaa (käyttäjän)

Uuden takin löytäminen on sama kuin voiman lisääminen. Takki (`T` kartalla) puetaan heti, kun sen
päälle kävelee, eikä se vie käsipaikkaa. Voima nousee täyteen (`INITIAL_HP`, taudin
`maxHealth`-katto rajoittaa), ja hihat näkyvät taas ehjimmässä vaiheessa.
*(Builderin oletus: takkeja ilmestyy tasolle harvakseltaan kuten muitakin tavaroita, ja
joskus kaatuneelta sotilaalta. Viesti esim. "Uusi takki! Vain vähän verta kauluksessa.")*

**Takin riisuminen ruumiilta (käyttäjän):** kaatuneen (voima 0) ihmisen päälle
kävellessä voi riisua tämän takin, jolloin voima nousee täyteen. Se on paheksuttavaa:
jokainen, joka näkee teon (`Person::canSeeTarget`, kuten `Level::witnessPutinDeath`),
hyökkää heti kimppuun ja huutaa esim. "Ruumiinryöstäjä!" tai "Paskiainen!".
*(Hyökkäys käyttää samaa tilaa kuin muutkin vihaiset ohikulkijat, esim. `alerted`; builder
valitsee. Riisuminen kysytään, jotta sitä ei tee vahingossa: "Riisutko takin? (k/e)")*
Yhdeltä ruumiilta saa vain yhden takin.

**Käytetty takki kaupasta (käyttäjän):** Divarista löytyy käytetty, heikkokuntoinen takki.
Se antaa vähemmän voimaa (esim. +3, enintään `INITIAL_HP`), ja hihat jäävät
repaleisemmiksi kuin uudella takilla. *(Hinta ja tarkka voima ovat builderin valinta.)*

### Missä kuvat näkyvät

- **Web (pääpeli, portti 3000):** vasen paneeli `#side` (nyt ТОВАРЫ-lista), johon tulee vasen käsi
  ja paikkalista (tasku, vyö, selkä). Kartan oikealle puolelle tulee uusi paneeli
  oikealle kädelle. Kapeassa ikkunassa (`max-width: 1000px`) paneelit piilotetaan kuten nyt.
- **Terminaali (`printerterm.cpp`) ja ncurses:** paikat näytetään tekstinä. Kuvat lisätään,
  jos tila riittää. *(Builderin harkinta.)*

## Vinkit ja kommentit

Kommentit arvotaan listoista, joten sama rivi ei toistu joka kerta.

- **Olut:** "Olutpullo meni kierteellä alas." (käyttäjän) sekä esim. "Baltika katosi kuin
  rupla lomakassasta."
- **Votkan huikat täyttöasteen mukaan:** 6→5 "Ensimmäinen on aina pyhä. Na zdorovje!",
  5→4 "Kurkku polttaa. Se on hyvä merkki.", 4→3 "Puolikas jäljellä. Optimisti sanoisi
  puoliksi täysi.", 3→2 "Pullo kevenee, mieli raskautuu.", 2→1 "Viimeinen huikka
  odottaa.", 1→0 "Tyhjä. Pullo katsoo sinua syyttävästi."
- **Vinkit:** "Kädet ovat täynnä. Juo tai heitä jotain pois." · "AK roikkuu selässä,
  mutta kädet ovat varattuina. Juo pullo tyhjäksi ja heitä se pois." · "Pistooli on
  vyöllä, mutta kädessä on pullo."
- **Ohikulkijat (käyttäjän):**
  - Kun pelaaja on humalassa ja tuijottaa ohikulkijaa (katsemekaniikka `gaze`), tämä voi
    sanoa ohimennen: "Sehän katsoo kuin ryssä rahaa."
  - Kun humalainen pelaaja hoipertelee: "Sehän pyörii kuin ryssä Anttilassa."

## Tarkentavat päätökset (Clauden valintoja, käyttäjä voi muuttaa)

- **Propusk** on paperi sisätaskussa: se ei vie paikkaa eikä putoa kaatuessa, koska se
  avaa Kremlin eikä saa jumittaa peliä.
- **Kädet ja kuvat:** juomat menevät mieluiten vasempaan käteen ja aseet oikeaan, muut
  tavarat vapaaseen käteen. Jos pullo päätyy oikeaan käteen tai pistooli vasempaan, kuva
  peilataan (merkkien vaihto `/`↔`\`, `(`↔`)` jne.). Aloituksessa olut on vasemmassa kädessä
  ja makkara oikeassa.
- **Vyö** = pistooli kulkee mukana ilman että se vie kättä. Ampuminen vaatii aina pistoolin
  ottamisen vapaaseen käteen. Tähtäystilassa (`gunDrawn`) pistooli on kädessä ja näkyy
  kuvassa, ja Esc palauttaa sen vyölle. Ilman vyötä pistoolia kannetaan kädessä.
- **AK:** tähtäys ja ammunta täyttävät molemmat kädet, kunnes ase lasketaan takaisin
  selkään. Toinen AK ei mahdu, joten se jää maahan.
- **Ryöstösaalis, joka ei mahdu:** se jää maahan ruumiin viereen, ja pelaaja voi vaihtaa
  tavaroita heittämällä ja poimimalla.
- **Heitto ja syönti:** `t` heittää kädestä. Jos molemmissa käsissä on jotain, valitaan
  käsi (vasen/oikea), muuten heitetään suoraan. `e` syö kädessä olevan ruoan.
  Heittäminen on samalla tapa pudottaa tavara.
- **Tyhjät pullot:** jokainen vie käden. Palautuksessa luukulle menevät kerralla kaikki
  kädessä olevat tyhjät pullot.
- **Webin kuvadata:** kuvat ovat yhdessä paikassa C++:ssa. Palvelin lähettää framessa
  valmiiksi piirretyt rivit vain silloin, kun ne muuttuvat, joten webiin ei tule kopiota.
- **Uudet tavarat** ovat oletuksena kädessä kannettavia.

## Jätetään builderille

- Datamalli (miten kädet/tasku tallennetaan, esim. enum + täyttöaste).
- Nykyisten `++inv.X`-kohtien läpikäynti (kaupat, löydöt, ryöstöt) poimintasäännön mukaisiksi.
- Aloitusvarustus. Nykyinen on kalja 1 ja makkara 1, joten olut menee vasempaan käteen ja makkara oikeaan.
- Testit: yksikkötestit (`test/unit_tests.cpp`) huikille, poiminnalle täysillä käsillä,
  AK:n ehdolle ja taskun putoamiselle.
