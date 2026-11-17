# Juoksuhaudat – toteutussuunnitelma

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rintaman tasoille proseduraaliset juoksuhaudat, altistus, kiväärimiehet, droonien keinulauta, tykistö (2–3) ja Bradley-hyökkäys upseerin ja oman sulkutulen kanssa (3).

**Architecture:** Hautakerros `Level::trench` (400×200, `uint8_t`) generoidaan `src/trenches.cpp`:ssä. Rintamalogiikka (altistus, tykistö, hyökkäysvaiheet, sulkutuli) laajentaa `src/frontline.cpp`:tä. Uusi henkilö `Ukrainalainen` asuu `persons.cpp`:ssä, ja `Bradley` (`Vehicle`-aliluokka) `src/bradley.cpp`:ssä. Tulostin piirtää kerroksen `printer.cpp`:n maavärinä.

**Tech Stack:** C++14, oma testikehys `tests/unit_tests.cpp`, `tests/test_game.py`.

**Spec:** `docs/superpowers/specs/2026-11-17-juoksuhaudat-design.md`

## Global Constraints

- Selusta on lännessä (x ≤ `REAR_X` = 1), ukrainalaiset idässä.
- Pelaaja on venäläinen sotilas (`isRussianSoldier`).
- Merkkijonot ovat suomeksi UTF-8-hex-escapeina kuten muualla koodissa.
- Kaikki satunnaisuus tulee `std::rand`/`common::random`ista, jotta testit voivat kiinnittää siemenen.
- Kerros on tyhjä kaikilla muilla kuin sotatasoilla. Moskovan käytös ei muutu.

## Review Focus

1. Pelaaja kartan reunalla tai hautakerroksen ulkopuolella (x ≥ 400, y ≥ 200): `trenchAt` palauttaa `None` eikä lue taulukon ohi.
2. Upseeri kuolee ennen hyökkäystä (drooni tai tykistö): selusta aukeaa silti, eikä sulkutuli ammu.
3. Upseeri puuttuu kokonaan (spawni epäonnistui): taso 3 ei saa jumittua, vaan selusta aukeaa `WAR_SHIFT_TICKS`:n jälkeen.
4. Pelaaja kuolee kesken hyökkäyksen: `warTick`, Bradleyt ja kiväärimiehet eivät kaadu `playerDead`-tilassa.
5. Testit, jotka luovat `World`in ilman hautoja (`trench` tyhjä), eivät saa kaatua uusiin hakuihin.

---

### Task 1: Hautakerros ja generaattori

**Files:** Create `include/trenches.hpp`, `src/trenches.cpp`. Modify `include/level.hpp`, `src/pultsari.cpp` (`populateBuildings`, `populatePersons`, aloituspaikka). Test `tests/unit_tests.cpp`.

**Interfaces (produces):**
- `enum class Trench : uint8_t { None, Own, Enemy, Crater, Wire };`
- `Level::trench` (`std::vector<uint8_t>`, tyhjä = ei kerrosta), `Trench Level::trenchAt(const Coord&) const`, `bool Level::inOwnTrench(const Coord&) const`, `bool Level::inCover(const Coord&) const` (Own tai Enemy).
- `void generateTrenches(Level&)` täyttää kerroksen. `Coord nearestOwnTrench(const Level&, const Coord&)`. `int ownTrenchXAt(const Level&, uint32_t y)` (pääaseman itäreuna rivillä y, −1 jos ei).

- [ ] Testit: `trenches_main_line_runs_the_whole_height` (jokaisella rivillä 0–199 on Own-ruutu kohdassa x 6–22). `trenches_a_communication_trench_reaches_the_rear` (BFS Own-ruuduissa pelaajan aloituskohdan läheltä x=0:aan). `trenches_enemy_line_is_east` (Enemy-ruutuja vain kohdassa x 60–80). `trenches_lookup_off_the_map_is_none` (Review Focus 1, 5).
- [ ] Toteutus speksin mukaan (siksak, pesäkkeet, yhteyshaudat ≥2, joista yksi rivin `PLAYER_START_Y` ±6 sisällä, haarat, vihollislinja x≈70±5, kraatterit 20–40, piikkilanka `Wire` 2–4 pätkää kohdassa x 25–55).
- [ ] Sotatasolla: `generateTrenches` ennen henkilöitä. Ei `Seina`-muureja. Pelaaja `nearestOwnTrench`-kohtaan. Sotilaat ja skinheadit Own-ruutuihin.
- [ ] `make tests/unit_tests && ./tests/unit_tests`, kaikki läpi. Commit.

### Task 2: Piirto

**Files:** `src/printer.cpp`. Test `tests/unit_tests.cpp`.

- [ ] Testi `trench_cell_draws_sandbags_and_dark_ground`: `trenchCell(level, c, glyph, color)` palauttaa true Own-ruudulle, ja väri sisältää `48;5;`. Reunaruutu (naapuri None) saa `▒`. Kraatteri `o`, piikkilanka `#`.
- [ ] `bool trenchCell(const Level*, const Coord&, std::string& glyph, const char*& color)` nimettömään nimiavaruuteen `roadCell`in viereen ja kutsu samaan kohtaan. Henkilöiden ja esineiden alle hautakerroksen taustaväri kuten asfaltti. Näkymättömissä hauta näkyy himmeänä `48;5;234`-tyyppisenä.
- [ ] Testit läpi. Commit.

### Task 3: Altistus ja hautapaine

**Files:** `include/player.hpp`, `src/frontline.cpp`, `src/printerterm.cpp` + `printerweb`-tilarivi, `src/subtitles.cpp` (vain kutsut). Test.

**Interfaces:** `Player::exposure` (0–100), `Player::trenchPressure` (0–100), `Coord Player::pressureCell`. `void Level::exposureTick()`, jota kutsutaan `warTick`istä.
- [ ] Testit `exposure_rises_outside_and_falls_in_the_trench` (+4 ulkona, +2 kraatterissa, −2 haudassa, rajat 0–100) ja `trench_pressure_builds_when_standing_still_and_eases_when_moving` (+3/tick samassa 5×5-lohkossa, −2 lohkon vaihtuessa).
- [ ] Tekstitykset kynnyksillä 25/50/75 kerran per nousu. Tilarivillä `ALTISTUS` vain sotatasolla.
- [ ] Commit.

### Task 4: Ukrainalainen kiväärimies

**Files:** `include/persontype.hpp` (+`ukrainalainen`), `include/persons.hpp`, `src/persons.cpp`, `src/level.cpp` (`createPerson`), `src/printer.cpp` (väri, piilotus), `src/frontline.cpp` (`riflemenTick`). Test.

**Interfaces:** `class Ukrainalainen : public Person { enum class Mode { Lurk, Assault }; Mode mode; uint32_t flash; bool hiddenFromView() const; void fireAt(Person*, std::string&); }`. `bool Level::clearShot(const Coord& from, const Coord& to) const` (Bresenham, rakennusseinät katkaisevat, ≤ 70). `bool isUkrainian(const Person*)`.
- [ ] Testit: `riflemen_never_shoot_a_player_in_the_trench` (1000 tickiä, terveys ennallaan). `riflemen_shoot_a_player_in_the_open` (altistus 80, 300 tickiä, terveys laskenut, viesti "Luoti" tai "osuu"). `lurking_rifleman_is_hidden_until_he_fires`.
- [ ] Laukauksen todennäköisyys `2 + exposure/4` % ulkona kohteelle, osuma 60 % → 15 % etäisyyden mukaan (lineaarinen 0–70), vahinko 2–4. Kraatterissa osuma puolittuu. Suuliekki `*`-animaatio, ääni, ja ohilaukauksesta "Luoti viuhuu ohi!". Ammutaan myös venäläisiä NPC-sotilaita.
- [ ] `isRussianSoldier`: ukrainalainen ei ole. Sotatason `populatePersons`: 6 + 2×taso kiväärimiestä Enemy-ruutuihin.
- [ ] Sotilaat ja skinheadit liikkuvat sotatasolla vain hautaruutuihin (lukuun ottamatta lihamyrskyä).
- [ ] Commit.

### Task 5: Droonien keinulauta

**Files:** `src/fpv.cpp`, `include/fpv.hpp`. Test.

**Interfaces:** `uint32_t fpvPlayerShare(const Level&)` (prosentti). `uint32_t fpvMax(const Level&)`.
- [ ] Testit `fpv_player_share_is_high_in_the_trench_low_when_just_out` (haudassa 50 + paine/2, enintään 90; ulkona altistus 0 → 10, 100 → 90) ja `fpv_drones_come_from_the_east` (lähtö-x > saaliin x).
- [ ] Sotatasolla `FPV_MAX` 7. Uusi drooni `random(25 + exposure/4)`. Ukrainalaiset eivät ole kohteita. Tulokulma idästä (0 ± 0,8 rad).
- [ ] Vanha testi `fpv_drones_swarm_the_front_but_not_moscow` pysyy vihreänä (rajana `fpvMax`). Commit.

### Task 6: Tykistö (taso 2–3) ja lihamyrsky

**Files:** `src/frontline.cpp`, `src/events.cpp`, `include/events.hpp`, `include/level.hpp`. Test.

**Interfaces:** `void Level::shellBurst(const Coord& at, uint32_t radius, uint32_t maxDamage, const char* who)`. Haudassa oleva, joka ei ole suoraan osumaruudussa, saa vahinkoa vain säteellä 1. Osuma tekee ruudusta (ja naapureista) `Crater`in, paitsi Own-ruudusta, joka pysyy Own-ruutuna (hauta suojaa yhä). `Level::warStage`, `artilleryCountdown`, `void Level::artilleryTick()`.
- [ ] Testit: `no_artillery_on_the_first_front_level` (300 tickiä warStage 1, ei "TYKIST"-viestiä). `artillery_warns_before_it_lands` (varoitus tick t, iskut t+2). `shell_burst_spares_the_trench_beyond_one_cell`.
- [ ] `EnvironmentEvents`: sotahaaran `artilleryBarrage` pois. `meatAssault` spawnaa skinheadit Own-ruutuihin ja juoksuttaa itään (skinheadin tila `charging`).
- [ ] Commit.

### Task 7: Taso 3 – keskitys, Bradley-hyökkäys, upseeri, sulkutuli

**Files:** Create `include/bradley.hpp`, `src/bradley.cpp`. Modify `src/frontline.cpp`, `src/pultsari.cpp` (upseerin spawni), `include/level.hpp`. Test.

**Interfaces:** `enum class AssaultPhase { Quiet, Barrage, Assault };` `Level::assaultPhase`, `Person* Level::holdOfficer`. `class Bradley : public Vehicle { Coord stopAt; bool unloaded; uint32_t gunCooldown; uint32_t hp; void update() override; }` (3×2, `▓`, hiekka). `void Level::barrierFireTick()`. Konekivääripesäkkeet `Level::mgNests` (`std::vector<Coord>`).
- [ ] Testit: `third_level_barrage_then_assault` (tick 30 → Barrage, 55 → Assault, Bradleyt lisätty). `bradley_stops_short_of_the_trench_and_unloads` (pysähtyy 6–10 ruutua Own-itäreunasta, 4–6 Assault-tilan ukrainalaista). `rear_stays_shut_while_the_officer_lives` (warTicks 200, upseeri elossa → `rearOpen` false; upseeri kuolee → true seuraavalla tickillä). `own_machine_guns_fire_on_a_player_behind_the_officer` (pelaaja upseeri.x − 15, 200 tickiä → terveys laskee). `no_barrier_fire_once_the_officer_is_dead`. `third_level_without_officer_opens_after_shift` (Review Focus 3).
- [ ] Hyökkääjät: kohti lähintä näkyvää venäläistä (upseeri painotettuna), ja hautaruudut ovat sallittuja. AK-sarja kuten `Sotilas::fire`, kantama 8.
- [ ] Bradleyn konetykki: 3 tickin välein `shellBurst(r=1, max 5)` näkyvään kohteeseen ≤ 30. Kranaatti säteellä 1 → `wrecked`.
- [ ] Commit.

### Task 8: Pelitesti ja viimeistely

- [ ] `make test` kokonaan (yksikkö + `test_game.py`).
- [ ] Aja `./pultsari --front` ja tarkista, että haudat näkyvät ja peli ei kaadu. Tasot 2 ja 3 tarkistetaan väliaikaisella `--front`-tasoparametrilla, jos sellainen on, tai yksikkötestein.
- [ ] Commit ja push `pultsari2`:een.
