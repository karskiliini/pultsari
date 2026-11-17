# Valojuovat (tracerit) – lyhyt suunnitelma

Hyväksytty 2026-11-17 ("Hyväksy, toteuta").

## Tarkoitus
Kun kuka tahansa ampuu pelissä aseella, luoti näkyy pienenä, nopeasti lentävänä valojuovana:
viisi pientä pistettä / x-merkkiä värigradientilla (valkoinen kärki → keltainen → oranssi →
punainen → tummanpunainen häntä). Taistelu näyttää taistelulta, ja pelaaja näkee mistä ammutaan.

## Päätökset
- Yksi malli `Level::tracers` (alku, loppu, nopeus 60 ruutua/s, viive sarjan myöhemmille
  laukauksille). Osuma pysähtyy kohteeseen, ohilaukaus jatkaa 6–10 ruutua ohi.
- Jokainen ampumakohta kutsuu `addTracer`: `fireBullet` (pelaajan ase, rosvon sarja,
  harhaluodit), sotilaiden sarjat, veteraani, ryöstäjä, myyjä, ukrainalaisten kivääri ja AK,
  Bradleyn konetykki (raskaampi juova), sulkuosaston konekiväärit, ilmatorjunnan ja
  sotilaiden laukaukset droonia ja taivasta kohti.
- Terminaali: pistekirjoituspisteet (2×4 / ruutu) omilla 256-väreillään; vain näkyvissä
  ruuduissa.
- Selain: `{"t":"tracer"}`-tapahtuma, sivu piirtää viisi ¼-kokoista `x`:ää tarkkaan
  liukulukusijaintiin hehkulla, animoi itse (requestAnimationFrame), näkyvyysmaskin mukaan.
- Vain reaaliaikatilassa ja `--anims`-tilassa (selainpeli käyttää molempia). Vuoropohjaisessa
  tilassa juova jäisi roikkumaan vuorosta toiseen. Oma päätös.

## Ei tavoitteena
Juovat eivät vaikuta pelimekaniikkaan (osumat ratkaistaan kuten ennenkin).
