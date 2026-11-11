
#include "pultsari.hpp"
#include "common.hpp"
#include "intro.hpp"
#include "input.hpp"
#include "building.hpp"
#include "level.hpp"
#include "aagun.hpp"
#include "patrol.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "coord.hpp"
#include "item.hpp"
#include "mask.hpp"
#include "scoreboard.hpp"
#include "vehicle.hpp"
#include "events.hpp"
#include "gamestate.hpp"
#include "metro.hpp"
#include <iostream>
#include <exception>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <cstdlib>

using std::cout;
using std::endl;
using common::random;

namespace common {
    bool animsEnabled;
    bool realtimeEnabled;
}


static void populateItems(Level& level, const GameState& gs)
{
    if (gs.phase == GamePhase::War) {
        if (random(30)) level.addItem(EVodka);
        if (random(20)) level.addItem(EGranata);
        if (random(25)) level.addItem(EPistooli);
        if (random(40)) level.addItem(EKivi);
        if (random(60)) level.addItem(ELenkki);
        if (random(30)) level.addItem(EVeitsi);
        if (random(50)) level.addItem(ERaha);
        if (random(15)) level.addItem(EPaska);
        for (int i = 0; i < std::rand() % 2 + 1; ++i)
            level.addItem(EPensas);
        return;
    }

    for (int n = 0; n < 5; ++n) {
        if (random(50))  level.addItem(EKalja);
        if (random(25)) level.addItem(ELenkki);
        if (random(25)) level.addItem(EKetjut);
        if (random(10))  level.addItem(EVeitsi);
        if (random(55))  level.addItem(EKivi);
        if (random(15))  level.addItem(EBootsit);
        if (random(5))   level.addItem(EKala);
        if (random(10))  level.addItem(EOmppo);
        if (random(5))   level.addItem(EBanaani);
        if (random(45))  level.addItem(EPamppu);
        if (random(80))  level.addItem(EViesti);
        if (random(80))  level.addItem(ELonkka);
        if (random(90))  level.addItem(ERaha);
        if (random(19))  level.addItem(EPaska);
    }
    if (random(15))
        level.addItem(EPistooli);
    if (random(20)) level.addItem(EVodka);
    for (int i = 0; i < std::rand() % 8 + 4; ++i)
        level.addItem(EPensas);
    // a bin or two beside each building, the ground around them full of litter
    for (auto b : level.buildings) {
        if (b->type == ESeina || b->type == EKremlin) continue;
        for (int k = 0; k < 1 + std::rand() % 2; ++k) {
            const bool right = random(50);
            Coord bin { right ? b->wallRight + 1 : (b->wallLeft > 0 ? b->wallLeft - 1 : 0),
                        b->wallTop + std::rand() % (b->wallBot - b->wallTop + 1) };
            const Road* h = nullptr;
            const Road* v = nullptr;
            level.roadsAt(bin, h, v);
            if (h || v || level.hit(bin)) continue;
            level.addItem(new Roskis(bin));
            bool ratPlaced = !random(30);
            for (int t = 0; t < 4 + std::rand() % 5; ++t) {
                Coord c { bin.x + std::rand() % 5 - 2, bin.y + std::rand() % 5 - 2 };
                level.roadsAt(c, h, v);
                if (h || v || level.hit(c)) continue;
                if (!ratPlaced) {
                    Rotta* rat = new Rotta(c);
                    rat->home = bin;
                    rat->pathFinding = level.pathFinding;
                    rat->printer = level.printer;
                    if (level.addPerson(rat)) { ratPlaced = true; continue; }
                    delete rat;
                }
                level.addItem(new Roska(c));
            }
        }
    }
    level.scatterNeedles();
    if (random(20))
        level.addItem(EPropusk);
    {
        ViinaKellari* vk = new ViinaKellari(Coord{315, 23}, &level);
        level.addItem(vk);
    }
}

// Moscow's streets: they run between the buildings and give the city its shape
static void populateRoads(Level& level)
{
    level.roads = {
        { true, 46, "NOVYI ARBAT" },
        { true, 112, "TVERSKAJA UL." },
        { true, 150, "LENINSKI PR." },
        { false, 15, "LENINGRADSKOJE SH." },
        { false, 135, "SADOVOJE KOLTSO" },
        { false, 235, "KUTUZOVSKI PR." },
        { false, 340, "PROSPEKT MIRA" },
    };
}

// is this patch of ground free for a new building: no building, road or map edge within it?
static bool rectFree(const Level& level, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
    if (x0 < 2 || y0 < 2 || x1 >= common::SIZEX - 2 || y1 >= common::SIZEY - 2) return false;
    for (uint32_t y = y0; y <= y1; ++y) {
        for (uint32_t x = x0; x <= x1; ++x) {
            const Coord c { x, y };
            if (level.hitBuilding(c)) return false;
            const Road* h = nullptr;
            const Road* v = nullptr;
            level.roadsAt(c, h, v);
            if (h || v) return false;
        }
    }
    return true;
}

// the queue in a shop: drunks and babushkas waiting their turn, none of them patient
// (level.hit() counts the whole shop, so place by walls; the sign row stays clear)
static void spawnQueue(Level& level, Building* b, int count)
{
    uint32_t x = b->wallLeft + 3;
    for (int k = 0; k < count && x < b->wallRight; ++k, x += 3) {
        const Coord at { x, b->wallTop + (k % 2 ? 1u : 3u) };
        if (level.hitWall(at) || level.hitPerson(at)) continue;
        Person* p = (k % 3 == 1) ? (Person*)new Mummo(at) : (Person*)new Juoppo(at);
        p->pathFinding = level.pathFinding;
        p->printer = level.printer;
        if (!level.addPerson(p)) delete p;
    }
}

static void populateBuildings(Level& level, const GameState& gs)
{
    if (gs.phase == GamePhase::War) {
        Building* b = new Seina();
        b->setOffset(30, 80);
        level.addBuilding(b);
        if (random(50)) {
            b = new Seina();
            b->setOffset(100, 60);
            level.addBuilding(b);
        }
        return;
    }

    Building* b = nullptr;

    // Alko - west side
    b = new Alko();
    b->setOffset(30, 85);
    level.addBuilding(b);
    spawnQueue(level, b, 5);

    // KRauta - west side
    b = new KRauta();
    b->setOffset(80, 55);
    level.addBuilding(b);

    // Isku - south area
    b = new Isku();
    b->setOffset(60, 70);
    level.addBuilding(b);

    // Divari - east area
    b = new Divari();
    b->setOffset(150, 70);
    level.addBuilding(b);

    // Vankila + PoliisiAsema - mid-east
    b = new Vankila();
    b->setOffset(120, 40);
    level.addBuilding(b);

    b = new PoliisiAsema();
    b->setOffset(120, 60);
    level.addBuilding(b);

    // Asema - far east
    b = new Asema();
    b->setOffset(200, 70);
    level.addBuilding(b);

    // Seina obstacles scattered
    b = new Seina();
    b->setOffset(180, 50);
    level.addBuilding(b);

    // Sberbank on the north side of Tverskaja
    b = new Pankki();
    b->setOffset(100, 102);
    level.addBuilding(b);

    // Russian shops north of Tverskaja: open, rude and always queued up, and the wrong sort
    // of people hang around outside
    {
        struct Shop { const char* name; Kauppa::Kind kind; uint32_t x; };
        const Shop shops[] = {
            { "\xD0\x9F\xD0\xA0\xD0\x9E\xD0\x94\xD0\xA3\xD0\x9A\xD0\xA2\xD0\xAB", Kauppa::Kind::Produkty, 45 },
            { "\xD0\x9A\xD0\x98\xD0\x9E\xD0\xA1\xD0\x9A", Kauppa::Kind::Kioski, 160 },
            { "\xD0\x9C\xD0\x90\xD0\x93\xD0\x90\xD0\x97\xD0\x98\xD0\x9D", Kauppa::Kind::Produkty, 270 },
            // ОРУЖИЕ: the gun shop, between the first Produkty and Sberbank
            { "\xD0\x9E\xD0\xA0\xD0\xA3\xD0\x96\xD0\x98\xD0\x95", Kauppa::Kind::Oruzhie, 74 },
        };
        const PersonType crooks[] = { varas, jengi, diileri, skinhead, ryostaja };
        for (const auto& s : shops) {
            b = new Kauppa(s.name, s.kind);
            b->setOffset(s.x, 104);
            level.addBuilding(b);
            spawnQueue(level, b, 3 + std::rand() % 3);
            const Coord door = b->getDoor();
            const int loiterers = 1 + std::rand() % 2;
            for (int k = 0, tries = 0; k < loiterers && tries < 30; ++tries) {
                const Coord at { door.x - 6 + std::rand() % 13, door.y + 1 + std::rand() % 2 };
                if (level.createPerson(at, crooks[std::rand() % 5])) ++k;
            }
        }
    }

    // Tverskaja, the shopping street: both sides lined with Western shops, all shut since 2022
    {
        struct Shop { const char* name; const char* sign; };
        const Shop brands[] = {
            { "McDONALD'S", "Ikkunassa lukee nyt 'Vkusno i totska'. Logo on sama, ilman M:\xC3\xA4\xC3\xA4." },
            { "STARBUCKS", "Kyltiss\xC3\xA4 lukee 'Stars Coffee'. Sama nainen, eri kruunu." },
            { "H&M", "Mallinuket seisovat alasti. Ne ovat odottaneet vuodesta 2022." },
            { "ZARA", "Kyltiss\xC3\xA4 lukee nyt 'Maag'. Samat vaatteet, eri lappu." },
            { "APPLE STORE", "Ikkunassa lukee 'reStore'. Hyllyt ovat tyhj\xC3\xA4t, omenalogo on maalattu punaiseksi." },
            { "NIKE", "Ikkunassa on kiinalaisia lenkkareita. Swooshit ovat v\xC3\xA4\xC3\xA4rin p\xC3\xA4in." },
            { "UNIQLO", "P\xC3\xB6ly\xC3\xA4. Paljon p\xC3\xB6ly\xC3\xA4." },
            { "LEVI'S", "Farkkuja ei ole. On 'Lev'is'-farkkuja Kirgisiasta." },
            { "ADIDAS", "Kolme raitaa on teipattu yli. Nyt niit\xC3\xA4 on nelj\xC3\xA4." },
            { "KFC", "Kyltiss\xC3\xA4 lukee 'Rostic's'. Eversti on vaihdettu viiksekk\xC3\xA4\xC3\xA4n mieheen." },
            { "IKEA", "Ikkunassa on yksi Billy-hylly. Ohjeet puuttuvat. Kuten kaikki muukin." },
            { "LEGO", "Kyltiss\xC3\xA4 lukee 'Mir Kubikov'. Palikat ovat vinossa." },
            { "MANGO", "Ikkunassa on lappu: 'Vuokrataan. Ei l\xC3\xA4nsimaisille.'" },
            { "SEPHORA", "Hajuvesipullot ovat tyhji\xC3\xA4. Joku on juonut ne." },
            { "SAMSUNG", "N\xC3\xA4yt\xC3\xB6ss\xC3\xA4 py\xC3\xB6rii yh\xC3\xA4 sama mainos vuodelta 2021." },
            { "PUMA", "Puma on vaihdettu kissaksi. Kissa nukkuu ikkunalla." },
            { "BOSS", "Pomo l\xC3\xA4hti. Puvut j\xC3\xA4iv\xC3\xA4t, ja ne myyd\xC3\xA4\xC3\xA4n Dubaissa." },
            { "L'OR\xC3\x89" "AL", "'Koska olet sen arvoinen.' Ei ole." },
            { "ROLEX", "Kelloissa on aika 02:24. Ne ovat pys\xC3\xA4htyneet." },
            { "VOLVO", "N\xC3\xA4yttelyauto on kiinalainen. Logo on liimattu." },
            { "PEPSI", "Pullossa lukee 'Cool Cola'. Maku on 'cool'." },
            { "AUCHAN", "Ovessa lukee: 'Lähdemme. Tai emme. Emme tiedä.'" },
        };
        const size_t nBrands = sizeof(brands) / sizeof(brands[0]);
        size_t next = 0;
        // north side faces south onto the street, south side faces north
        for (int side = 0; side < 2; ++side) {
            const uint32_t y = side == 0 ? 117 : 104;
            for (uint32_t x = 18; x + 15 < common::SIZEX && next < nBrands * 2; x += 17) {
                if (!rectFree(level, x - 1, y - 1, x + 15, y + 5)) continue;
                const auto& br = brands[next++ % nBrands];
                b = new SuljettuLiike(br.name, br.sign, side == 1);
                b->setOffset(x, y);
                level.addBuilding(b);
            }
        }
    }

    // oil refineries in the south: drone magnets
    b = new Jalostamo();
    b->setOffset(40, 160);
    level.addBuilding(b);
    b = new Jalostamo();
    b->setOffset(350, 160);
    level.addBuilding(b);

    // Kremlin - far northeast (the goal)
    b = new Kremlin();
    b->setOffset(250, 20);
    level.addBuilding(b);

    // Hypermarkets - scattered across the map
    {
        Hypermarket* hm = new Hypermarket();
        hm->setOffset(100, 30);
        level.addBuilding(hm);

        hm = new Hypermarket();
        hm->setOffset(200, 100);
        level.addBuilding(hm);

        hm = new Hypermarket();
        hm->setOffset(300, 60);
        level.addBuilding(hm);
    }

    // metro stations dotted around the city, one line through them all
    {
        struct Station { const char* name; uint32_t x, y; };
        const Station stations[] = {
            { "Tverskaja", 62, 96 }, { "Ohotnyi Rjad", 232, 40 }, { "Park Kultury", 190, 140 },
            { "Komsomolskaja", 330, 96 }, { "Kievskaja", 20, 140 },
        };
        for (const auto& st : stations) {
            // the nearest free spot to where the station should be
            bool placed = false;
            for (int r = 0; r < 30 && !placed; ++r) {
                for (int k = 0; k < 16 && !placed; ++k) {
                    const int x = (int)st.x + (k % 4 - 2) * r, y = (int)st.y + (k / 4 - 2) * r / 2;
                    if (x < 3 || y < 3) continue;
                    if (!rectFree(level, x - 1, y - 1, x + 11, y + 4)) continue;
                    b = new Metro(st.name);
                    b->setOffset(x, y);
                    level.addBuilding(b);
                    placed = true;
                }
            }
        }
    }

    // petrol stations by the big roads: a kiosk, two pumps, and people complaining about prices
    {
        const Coord wanted[] = { { 100, 142 }, { 300, 38 } };
        for (const auto& want : wanted) {
            bool placed = false;
            for (int r = 0; r < 30 && !placed; ++r) {
                for (int k = 0; k < 16 && !placed; ++k) {
                    const int x = (int)want.x + (k % 4 - 2) * r, y = (int)want.y + (k / 4 - 2) * r / 2;
                    if (x < 3 || y < 3) continue;
                    if (!rectFree(level, x - 1, y - 1, x + 13, y + 9)) continue;
                    Bensa* station = new Bensa();
                    station->outOfFuel = random(50);
                    b = station;
                    b->setOffset(x, y);
                    level.addBuilding(b);
                    for (int px : { 3, 9 }) {
                        Building* pump = new Pumppu();
                        pump->setOffset(x + px, y + 7);
                        level.addBuilding(pump);
                    }
                    // customers waiting, and grumbling
                    for (int c = 0, tries = 0; c < 3 && tries < 30; ++tries) {
                        const Coord at { (uint32_t)(x + std::rand() % 13), (uint32_t)(y + 5 + std::rand() % 4) };
                        if (level.createPerson(at, common::random(60) ? kansalainen : juoppo)) ++c;
                    }
                    placed = true;
                }
            }
        }
    }

    // and the empty blocks fill up with grey flats, leaving alleys between them
    {
        const char* names[] = {
            "\xD0\xA5\xD0\xA0\xD0\xA3\xD0\xA9\xD0\x81\xD0\x92\xD0\x9A\xD0\x90",   // ХРУЩЁВКА
            "\xD0\x9F\xD0\x90\xD0\x9D\xD0\x95\xD0\x9B\xD0\xAC\xD0\x9A\xD0\x90",   // ПАНЕЛЬКА
            "\xD0\x9E\xD0\x91\xD0\xA9\xD0\x95\xD0\x96\xD0\x98\xD0\xA2\xD0\x98\xD0\x95", // ОБЩЕЖИТИЕ
            "", "", "",
        };
        const Coord start { common::PLAYER_START_X, common::PLAYER_START_Y };
        for (uint32_t y = 6; y < common::SIZEY - 10; y += 4) {
            for (uint32_t x = 4; x < common::SIZEX - 20; x += 5) {
                const uint32_t w = 12 + std::rand() % 14, h = 5 + std::rand() % 4;
                if (Coord{ x + w / 2, y + h / 2 }.distance(start) < 14) continue;
                if (!rectFree(level, x - 2, y - 2, x + w + 1, y + h + 1)) continue;
                if (!random(65)) continue;
                b = new Kerrostalo(names[std::rand() % 6], w, h);
                b->setOffset(x, y);
                level.addBuilding(b);
            }
        }
    }
}

// some veterans come with a state-funded personal assistant, permanently drunk
static void spawnVeteran(Level& level)
{
    Veteraani* v = dynamic_cast<Veteraani*>(level.createPerson(level.freePosition(), veteraani));
    if (!v || !random(v->blind ? 70 : 40)) return;

    const int offsets[][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    for (const auto& o : offsets) {
        Coord c { (uint32_t)((int)v->coord.x + o[0]), (uint32_t)((int)v->coord.y + o[1]) };
        if (level.hit(c)) continue;
        Avustaja* a = new Avustaja(c);
        a->pathFinding = level.pathFinding;
        a->printer = level.printer;
        if (level.addPerson(a)) { v->assistant = a; return; }
        delete a;
    }
}

static void populatePersons(Level& level, const GameState& gs)
{
    if (gs.phase == GamePhase::War) {
        // war levels: skinheads as enemy soldiers, vankis as POWs
        for (int i = 0; i < 3 + (int)gs.warStage * 2; ++i) {
            level.createPerson(level.freePosition(), skinhead);
        }
        if (random(40)) level.createPerson(level.freePosition(), vanki);
        for (int i = 0; i < (int)gs.warStage; ++i) {
            if (random(50)) spawnVeteran(level);
        }
        // comrades in the trenches around the player: they shoot at the FPV drones, and the drones hunt them
        for (int i = 0; i < 4 + (int)gs.warStage * 2; ++i) {
            Coord c = level.freePosition();
            for (int tries = 0; tries < 30; ++tries) {
                const Coord near { common::PLAYER_START_X + std::rand() % 41 - 20, common::PLAYER_START_Y + std::rand() % 17 - 8 };
                if (near.x < common::SIZEX && near.y < common::SIZEY && !level.hit(near)) { c = near; break; }
            }
            level.createPerson(c, sotilas);
        }
        return;
    }

    if (gs.phase == GamePhase::Kremlin) {
        // Kremlin: FSB guards, cops and soldiers
        for (int i = 0; i < 3; ++i)
            level.createPerson(level.freePosition(), fsb);
        for (int i = 0; i < 2; ++i)
            level.createPerson(level.freePosition(), poliisi);
        for (int i = 0; i < 2; ++i)
            level.createPerson(level.freePosition(), sotilas);
        level.createPerson(level.freePosition(), ilmatorjunta);
        return;
    }

    // Moscow phases - density scales with Kremlin proximity
    float kremlinProximity = (float)gs.moscowStage / 8.0f;

    // JUOPOT - lots of drunks, fewer near Kremlin
    {
        int drunkCount = (int)(5 * (1.0f - kremlinProximity * 0.7f)) + std::rand()%3;
        for (int i = 0; i < drunkCount; ++i) {
            level.createPerson(level.freePosition(), juoppo);
        }
    }

    // mummelit
    {
        int mummoCount = 1 + std::rand()%3;
        for (int i = 0; i < mummoCount; ++i) {
            level.createPerson(level.freePosition(), mummo);
        }
    }

    // MILIISIT - on every pavement and in the alleys, more towards the Kremlin
    Patrol::populateCops(level, gs.moscowStage);

    // varas
    if (random(60)) {
        level.createPerson(level.freePosition(), varas);
    }
    if (random(30)) {
        level.createPerson(level.freePosition(), varas);
    }

    // ykä
    if (random(50)) {
        Coord pos { common::PLAYER_START_X + 2, common::PLAYER_START_Y };
        if (!level.hit(pos))
            level.createPerson(pos, yka);
    }

    // jengi
    {
        int count = 1 + std::rand() % 3;
        for (int i = 0; i < count; ++i) {
            if (random(40))
                level.createPerson(level.freePosition(), jengi);
        }
    }

    // ryöstäjä
    if (random(30))
        level.createPerson(level.freePosition(), ryostaja);
    if (random(20))
        level.createPerson(level.freePosition(), ryostaja);

    // diileri
    if (random(40))
        level.createPerson(level.freePosition(), diileri);
    if (random(30))
        level.createPerson(level.freePosition(), diileri);

    // ordinary Muscovites, each with their own name, job and errands
    for (int i = 0; i < 25; ++i) {
        Kansalainen* k = new Kansalainen(level.freePosition());
        k->pathFinding = level.pathFinding;
        k->printer = level.printer;
        if (!level.addPerson(k)) delete k;
    }

    // drunk housewives with their foul-mouthed kids
    for (int i = 0; i < 2 + std::rand() % 2; ++i) {
        Kotiaiti* mum = dynamic_cast<Kotiaiti*>(level.createPerson(level.freePosition(), kotiaiti));
        if (!mum) continue;
        Coord c { mum->coord.x + 1, mum->coord.y };
        if (level.hit(c)) continue;
        Lapsi* kid = new Lapsi(c);
        kid->pathFinding = level.pathFinding;
        kid->printer = level.printer;
        kid->mother = mum;
        if (!level.addPerson(kid)) delete kid;
    }

    // SMO veteraanit
    if (random(35))
        spawnVeteran(level);
    if (random(25))
        spawnVeteran(level);

    // FSB - more near Kremlin
    {
        int fsbCount = (int)(kremlinProximity * 3);
        for (int i = 0; i < fsbCount; ++i) {
            if (random(20 + (int)(kremlinProximity * 40)))
                level.createPerson(level.freePosition(), fsb);
        }
    }

    // foreign recruits, far from home: a few lost Africans, a few North Koreans in step
    if (gs.phase != GamePhase::War) {
        for (int i = 0; i < 2 + std::rand() % 3; ++i) level.createPerson(level.freePosition(), rekryytti);
        for (int i = 0; i < 2 + std::rand() % 2; ++i) level.createPerson(level.freePosition(), korealainen);
    }

    // Sotilaat - appear closer to Kremlin
    if (kremlinProximity > 0.5f) {
        int solCount = (int)((kremlinProximity - 0.5f) * 4);
        for (int i = 0; i < solCount; ++i) {
            if (random(30 + (int)(kremlinProximity * 30)))
                level.createPerson(level.freePosition(), sotilas);
        }
    }

    // IT-yksiköt (ilmatorjunta) - near Kremlin
    if (kremlinProximity > 0.6f && random(40)) {
        level.createPerson(level.freePosition(), ilmatorjunta);
    }

    // Vartijat (guards) at hypermarkets
    for (auto b : level.buildings) {
        if (b->type == EHypermarket) {
            Coord doorPos = b->getDoor();
            Coord guardPos = doorPos;
            guardPos.y += 1;
            if (!level.hit(guardPos)) {
                Person* guard = level.createPerson(guardPos, vartija);
                if (guard) {
                    Vartija* v = dynamic_cast<Vartija*>(guard);
                    if (v) v->homePos = doorPos;
                }
            }
            // second guard
            Coord guard2Pos = doorPos;
            guard2Pos.x += 2;
            guard2Pos.y += 1;
            if (!level.hit(guard2Pos)) {
                Person* guard = level.createPerson(guard2Pos, vartija);
                if (guard) {
                    Vartija* v = dynamic_cast<Vartija*>(guard);
                    if (v) v->homePos = doorPos;
                }
            }
        }
    }

    // Putin spawns inside the Kremlin + comically long conference table
    for (auto b : level.buildings) {
        if (b->type == EKremlin) {
            // level.hit() counts the whole Kremlin rectangle, so place things by walls only
            auto free = [&](const Coord& c) {
                return !level.hitWall(c) && !level.hitPerson(c) && !level.hitItem(c);
            };

            // Putin's famous comically long conference table: a dashed outline
            // with an empty, polished surface in the middle
            const uint32_t top = b->wallTop + 5;
            const uint32_t bot = top + 3;
            const uint32_t left = b->wallLeft + 5;
            const uint32_t right = b->wallRight - 5;
            for (uint32_t y = top; y <= bot; ++y) {
                for (uint32_t x = left; x <= right; ++x) {
                    const bool edgeRow = (y == top || y == bot);
                    const bool edgeCol = (x == left || x == right);
                    if (!edgeRow && !edgeCol) continue;
                    const char* glyph = edgeRow ? "\xE2\x95\x8C" : "\xE2\x95\x8E";
                    if (edgeRow && edgeCol)
                        glyph = y == top ? (x == left ? "\xE2\x95\xAD" : "\xE2\x95\xAE")
                                         : (x == left ? "\xE2\x95\xB0" : "\xE2\x95\xAF");
                    Coord tc { x, y };
                    if (free(tc)) level.addItem(new PutinPoyta(tc, glyph));
                }
            }

            const KremlinArea area { { b->wallLeft + 1, b->wallTop + 1 }, { b->wallRight - 1, b->wallBot - 1 } };
            auto spawn = [&](Person* p) {
                p->pathFinding = level.pathFinding;
                p->printer = level.printer;
                if (!free(p->coord) || !level.addPerson(p)) delete p;
            };
            spawn(new Putin(Coord{ left - 2, top + 1 }));

            // the generals' war map: dashed table with a map on it
            const uint32_t mapTop = b->wallBot - 6;
            const uint32_t mapLeft = b->wallLeft + 6;
            const char* mapRow[] = {
                "~", "\xE2\x86\x92", "\xE2\x96\xB2", "~", "\xE2\x9C\x95", "\xE2\x86\x90", "\xE2\x96\xB2", "~", "\xE2\x86\x92",
            };
            const uint32_t mapW = sizeof(mapRow) / sizeof(mapRow[0]);
            for (uint32_t x = mapLeft; x <= mapLeft + mapW + 1; ++x) {
                for (uint32_t y = mapTop; y <= mapTop + 2; ++y) {
                    Coord mc { x, y };
                    if (!free(mc)) continue;
                    const bool edgeCol = (x == mapLeft || x == mapLeft + mapW + 1);
                    if (y == mapTop + 1 && !edgeCol)
                        level.addItem(new Kartta(mc, mapRow[x - mapLeft - 1]));
                    else
                        level.addItem(new PutinPoyta(mc, edgeCol && y == mapTop + 1 ? "\xE2\x95\x8E" : "\xE2\x95\x8C", false));
                }
            }

            spawn(new Kenraali(Coord{ mapLeft + 3, mapTop + 3 }, shoigu, area));
            spawn(new Kenraali(Coord{ mapLeft + 7, mapTop + 3 }, gerasimov, area));
            spawn(new Medvedev(Coord{ b->wallRight - 8, b->wallBot - 5 }, area));

            // Kremlin guards carry radios: two outside the door, two inside, one by Putin
            const Coord door = b->getDoor();
            const Coord guardPosts[] = {
                { door.x - 2, door.y + 1 }, { door.x + 2, door.y + 1 },
                { door.x - 2, door.y - 1 }, { door.x + 2, door.y - 1 },
                { left - 2, top + 3 },
            };
            for (const auto& post : guardPosts) {
                Vartija* guard = new Vartija(post);
                guard->hasRadio = true;
                spawn(guard);
            }
            break;
        }
    }

}

static void populateVehicles(Level& level)
{
    if (level.isWarLevel) return;

    {
        // two or three cars per lane, both directions
        for (const auto& r : level.roads) {
            for (int lane = 0; lane < 2; ++lane) {
                const uint32_t pos = r.pos + (lane ? 2 : 0);
                const int dir = lane ? 1 : -1;
                for (int k = 0; k < 2 + std::rand() % 2; ++k) {
                    const uint32_t along = std::rand() % (r.horizontal ? common::SIZEX : common::SIZEY);
                    const Coord c = r.horizontal ? Coord{ along, pos } : Coord{ pos, along };
                    Car* car = new Car(c, r.horizontal, dir);
                    bool free = true;
                    for (const auto& cell : car->cells()) free = free && !level.hit(cell) && !level.getVehicleAt(cell);
                    if (free) level.addVehicle(car); else delete car;
                }
            }
        }
    }

    // a petrol station that has run dry: a queue of cars, kilometres long, parked down the road
    for (auto b : level.buildings) {
        const Bensa* station = dynamic_cast<const Bensa*>(b);
        if (!station || !station->outOfFuel) continue;
        const Road* road = nullptr;
        uint32_t best = 1000;
        for (const auto& r : level.roads) {
            if (!r.horizontal) continue;
            const uint32_t d = r.pos > b->wallBot ? r.pos - b->wallBot : b->wallTop - r.pos;
            if (d < best) { best = d; road = &r; }
        }
        if (!road || best > 20) continue;
        const uint32_t lane = road->pos + (road->pos > b->wallBot ? 0 : 2);
        const int dir = road->pos > b->wallBot ? -1 : 1;
        // the queue runs back from the station, against the direction of travel
        int x = (int)(b->wallLeft + b->wallRight) / 2;
        for (int k = 0, tries = 0; k < 18 + std::rand() % 12 && tries < 60; ++tries) {
            x -= dir * 4;
            if (x < 2 || x > (int)common::SIZEX - 6) break;
            Car* car = new Car(Coord{ (uint32_t)x, lane }, true, dir);
            car->script = Car::Script::Parked;
            bool free = true;
            for (const auto& cell : car->cells()) free = free && !level.hit(cell) && !level.getVehicleAt(cell);
            if (free) { level.addVehicle(car); ++k; } else delete car;
        }
    }

    // cars parked by the kerb, nobody in them; now and then the keys are left in the ignition
    {
        int parked = 0;
        for (int tries = 0; !level.roads.empty() && tries < 200 && parked < 6 + (int)(std::rand() % 4); ++tries) {
            const Road& r = level.roads[std::rand() % level.roads.size()];
            const bool before = std::rand() % 2;
            const uint32_t across = before ? r.pos - 1 : r.pos + 3;
            const uint32_t along = 6 + std::rand() % ((r.horizontal ? common::SIZEX : common::SIZEY) - 12);
            const Coord at = r.horizontal ? Coord{ along, across } : Coord{ across, along };
            // the car faces the way traffic goes on that side, parked a little crooked
            const int dir = before ? -1 : 1;
            Car* car = new Car(at, r.horizontal, dir);
            car->level = &level;
            car->driverInside = false;
            car->engineOn = false;
            car->keysIn = common::random(35);
            car->makeFree();
            car->heading = (car->heading + (std::rand() % 3) - 1 + Car::HEADINGS) % Car::HEADINGS;
            car->headingF = car->heading * 2.0f * 3.14159265f / Car::HEADINGS;
            car->drive(0);
            bool free = true;
            for (const auto& cell : car->cells()) {
                const Road* h = nullptr;
                const Road* v = nullptr;
                level.roadsAt(cell, h, v);
                free = free && !level.hit(cell) && !level.getVehicleAt(cell) && !h && !v;
            }
            if (free && !car->cells().empty()) { level.addVehicle(car); ++parked; } else delete car;
        }
    }

    // Putin's overlong limousine on Novyi Arbat, a police car ahead and behind
    {
        const uint32_t lane = 46;
        const uint32_t x = 200;
        Car* front = new Car(Coord{ x - 8, lane }, true, -1, Car::Kind::Police);
        Car* limo = new Car(Coord{ x, lane }, true, -1, Car::Kind::Limousine);
        Car* back = new Car(Coord{ x + 13, lane }, true, -1, Car::Kind::Police);
        for (Car* car : { front, limo, back }) {
            bool free = true;
            for (const auto& cell : car->cells()) free = free && !level.getVehicleAt(cell);
            if (free) level.addVehicle(car); else delete car;
        }
    }

    {
        constexpr uint32_t BUS_ROUTE_Y = 112; // top lane of Tverskaja
        constexpr uint32_t BUS_STOP_X = 70;

        Bus* bus = new Bus(BUS_ROUTE_Y, BUS_STOP_X);
        level.addVehicle(bus);

        BusStop* stop = new BusStop(Coord{BUS_STOP_X, BUS_ROUTE_Y - 1});
        level.addItem(stop);
    }
}

// thrown by Z: the game starts over with a fresh drunk
struct RestartGame {};
static bool restartedByZ = false;
// --car: a car with the keys in waits right next to where you wake up (for trying out driving)
static bool startByCar = false;
// --front: start at the front, for trying out the war levels
static bool startAtFront = false;

static void parkCarByPlayer(Level& level, const Coord& at)
{
    for (int tries = 0; tries < 400; ++tries) {
        const int dx = 3 + tries % 12, dy = (tries / 12) % 2 ? -(1 + tries / 24) : 1 + tries / 24;
        const Coord c { (uint32_t)((int)at.x + dx), (uint32_t)std::max(2, (int)at.y + dy) };
        Car* car = new Car(c, true, 1);
        car->level = &level;
        car->driverInside = false;
        car->engineOn = false;
        car->keysIn = true;
        car->makeFree();
        bool free = !car->cells().empty();
        for (const auto& cell : car->cells()) free = free && !level.hit(cell) && !level.getVehicleAt(cell);
        if (free) { level.addVehicle(car); return; }
        delete car;
    }
}

static bool processInput(InputNS::InputType input, PlayerNS::Player& player, Level* level, Printer& printer)
{
    bool ret;
    switch(input)
    {
        case InputNS::up:
            ret = player.move(DirectionNS::up, level);
            break;
        case InputNS::down:
            ret = player.move(DirectionNS::down, level);
            break;
        case InputNS::left:
            ret = player.move(DirectionNS::left, level);
            break;
        case InputNS::right:
            ret = player.move(DirectionNS::right, level);
            break;
        case InputNS::drinking:
            ret = player.drink(level);
            break;
        case InputNS::eating:
            ret = player.eat(level);
            break;
        case InputNS::throwing:
            ret = player.throwItem();
            break;
        case InputNS::stagger:
            ret = player.stagger(level);
            break;
        case InputNS::throwup:
            {
                DirectionNS::Direction d;
                int dir = std::rand() % 4;
                switch(dir)
                {
                    default:
                    case 0: d = DirectionNS::up; break;
                    case 1: d = DirectionNS::right; break;
                    case 2: d = DirectionNS::down; break;
                    case 3: d = DirectionNS::left; break;
                }
                ret = player.puke(d, level);
            }
            break;
        default:
        case InputNS::quit:
            ret = false;
            throw RestartGame();
    }
    return ret;
}

static InputNS::InputType applyDrunkEffects(InputNS::InputType input, PlayerNS::Player& player, Level* level, Printer& printer)
{
    if (player.stats.promilles > 23 && common::random(10)) {
        if (input != InputNS::quit)
            input = InputNS::stagger;
    } else if (player.stats.promilles < 4 && common::random(20)) {
        printer.showMessage("Sua janottaa !!!", level);
    } else if (player.stats.promilles > 28 && common::random(5)) {
        printer.showMessage("Oksennat !!", level);
        HurtBy who(level, "oma oksennuksesi");
        player.damage(1);
        if (input != InputNS::quit)
            input = InputNS::throwup;
    }
    return input;
}

// After death the world carries on as if nothing happened. The body stays where
// it fell; in the browser the camera slowly zooms out until the whole city is
// a speck, and then the intro plays again. Any key starts a new life at once.
// the death certificate says something else: officially there is no epidemic
static const char* AIDS_DEATH = "Olet kuollut. Tappajasi: AIDS. Kuolintodistukseen kirjattiin keuhkokuume, koska virallisesti Ven\xC3\xA4j\xC3\xA4ll\xC3\xA4 ei ole HIV-epidemiaa.";

// pushed into a black Aurus for walking down the road: the front, or for a veteran, the end
static const char* CONSCRIPTED_TO_WAR = "FSB pakkov\xC3\xA4rv\xC3\xA4si sinut. Mustan Auruksen takakontissa on ahdasta, ja matka rintamalle kest\xC3\xA4\xC3\xA4 kolme p\xC3\xA4iv\xC3\xA4\xC3\xA4.";
static const char* CONSCRIPTED_DEATH = "SURUVIESTI: FSB pakkov\xC3\xA4rv\xC3\xA4si sinut toistamiseen rintamalle. Toiselta kerralta ei palata.";

static bool conscriptionGoesToWar(const GameState& gs)
{
    return gs.phase == GamePhase::Moscow && !gs.hasBeenToWar;
}

static void mourn(Level& level, VisionNS::Mask& mask, PlayerNS::Player& player, Printer* printer, const std::string& cause)
{
    constexpr uint32_t FRAME_MS = 50;
    constexpr uint32_t WORLD_TICK = 8;
    constexpr uint32_t ZOOM_MS = 22000;
    const bool web = common::webMode();

    level.playerDead = true;
    if (Car* car = player.driving) {
        // the car rolls on with a dead drunk at the wheel
        car->playerAtWheel = false;
        car->driverInside = true;
        car->driverDead = true;
        car->throttlePos = 0;
        player.driving = nullptr;
    }
    mask.enabled = false; // the camera rises above the rooftops
    level.audio.clear();
    printer->removeMessage();

    // you are dead, this is who did it, and a few words of comfort
    static const char* comfort[] = {
        "Ei se mit\xC3\xA4\xC3\xA4n. Ven\xC3\xA4j\xC3\xA4ll\xC3\xA4 syntyy joka p\xC3\xA4iv\xC3\xA4 uusia juoppoja.",
        "\xC3\x84l\xC3\xA4 sure. Pullo j\xC3\xA4i kesken, mutta joku muu juo sen loppuun.",
        "Lep\xC3\xA4\xC3\xA4 rauhassa. Krapula ei tule en\xC3\xA4\xC3\xA4 koskaan.",
        "Sinua kaivataan. Ainakin pullonpalautusluukulla.",
        "Et ollut ensimm\xC3\xA4inen etk\xC3\xA4 j\xC3\xA4\xC3\xA4 viimeiseksi. Vova kiitt\xC3\xA4\xC3\xA4 palveluksestasi.",
        "\xC3\x84iti olisi silti ylpe\xC3\xA4. Ehk\xC3\xA4.",
        "Taivaassa on Alko, ja se on aina auki.",
    };
    const std::string words = comfort[std::rand() % (sizeof(comfort) / sizeof(comfort[0]))];
    const std::string card = cause + " " + words;
    if (web) {
        common::webSend("{\"t\":\"death\",\"cause\":" + common::jsonString(cause) +
                        ",\"comfort\":" + common::jsonString(words) +
                        ",\"mummot\":" + std::to_string(player.scoreBoard.mummot) +
                        ",\"kaikki\":" + std::to_string(player.scoreBoard.kaikki) + "}");
    } else {
        printer->setMessage(card + " Paina mit\xC3\xA4 tahansa n\xC3\xA4pp\xC3\xA4int\xC3\xA4.");
    }

    while (InputNS::Input::getRawCharNonBlocking() != 0) { }

    const float maxZoom = (float)common::SIZEX / common::VIEWPORT_W;
    bool zoomedOut = false;
    // big frames take time to build, so the zoom follows the wall clock, not frame counts
    const auto start = std::chrono::steady_clock::now();
    for (uint32_t frame = 0;; ++frame) {
        const uint32_t t = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        bool key = false;
        for (char k; (k = InputNS::Input::getRawCharNonBlocking()) != 0;) key = true;
        if (key && t > 800) break;

        level.npcFrame(FRAME_MS);
        level.vehicleFrame(FRAME_MS);
        if (frame % WORLD_TICK == 0) {
            level.audio.clear();
            level.buildingTurn();
            level.spreadNews();
            level.updateVehicles();
            level.skyTick();
            level.robberyTick();
            if (level.events) level.events->update();
        }
        level.cleanDead();
        level.cleanDiscardedItems();
        level.stepAnimations();
        printer->msgTtl = 0;
        printer->removeMessage();

        if (web) {
            // the same zoom rate all the way: the view grows by a constant factor per second
            const float progress = std::min(1.0f, (float)t / ZOOM_MS);
            const float zoom = std::pow(maxZoom, progress);
            printer->viewW = std::min<uint32_t>(common::SIZEX, (uint32_t)(common::VIEWPORT_W * zoom));
            printer->viewH = std::min<uint32_t>(common::SIZEY, (uint32_t)(common::VIEWPORT_H * zoom));
            const int cx = (int)player.coord.x - (int)printer->viewW / 2;
            const int cy = (int)player.coord.y - (int)printer->viewH / 2;
            level.cameraX = std::max(0, std::min(cx, (int)(common::SIZEX - printer->viewW)));
            level.cameraY = std::max(0, std::min(cy, (int)(common::SIZEY - printer->viewH)));
            // big views are heavy to send: fewer frames the further out we are
            const uint32_t every = 1 + printer->viewW * printer->viewH / 5000;
            if (frame % every == 0) printer->print(&level);
            if (t > ZOOM_MS + 2000) { zoomedOut = true; break; }
        } else {
            printer->setMessage(card + " Paina mit\xC3\xA4 tahansa n\xC3\xA4pp\xC3\xA4int\xC3\xA4.");
            printer->print(&level);
        }
        common::frameSleep(FRAME_MS);
    }

    printer->viewW = common::VIEWPORT_W;
    printer->viewH = common::VIEWPORT_H;
    if (zoomedOut) {
        // back to the intro; the page sends a key when it is over
        common::webSend("{\"t\":\"restart\"}");
        while (InputNS::Input::getRawCharNonBlocking() == 0) common::sleep(20);
    }
}

// Through the metro's doors: the trip is a mini-game (metro.hpp), and how it went
// decides where, and whether, the player comes out. True when the player died on the way.
static bool rideMetro(Level& level, VisionNS::Mask& mask, PlayerNS::Player& player, Printer* printer,
                      GameState& gameState, bool& nextLevel)
{
    Metro* from = player.metroFrom;
    Metro* to = player.metroTrip;
    player.metroFrom = player.metroTrip = nullptr;
    if (!from || !to) return false;
    Metro* after = to->nextOnLine();
    if (!after) after = to;

    const auto r = MetroNS::runMetroMinigame(&player, &level, from->station, to->station, after->station, player.metroDodged);
    switch (r.outcome) {
        case MetroNS::Outcome::Arrived:  player.coord = to->getSpawn(); break;
        case MetroNS::Outcome::Overshot: player.coord = after->getSpawn(); break;
        case MetroNS::Outcome::Arrested: player.inJail = true; break;   // the court sends you on
        case MetroNS::Outcome::Recruited:
            if (gameState.phase == GamePhase::Moscow || gameState.phase == GamePhase::ReturnMoscow) {
                printer->setMessage("FSB:n mies saattaa sinut suoraan kutsuntatoimistoon. Rintamalle!");
                printer->print(&level);
                common::sleep(2000);
                gameState.sendToWar();
                nextLevel = true;
                return false;
            }
            player.inJail = true;
            break;
        case MetroNS::Outcome::Dead:
            player.stats.health = 0;
            mourn(level, mask, player, printer, "SURUVIESTI: " + r.cause);
            return true;
        default: break;
    }
    level.updateCamera();
    mask.init();
    printer->showMessage(r.summary, &level, false);
    return false;
}

bool mainloop(Printer* printer, uint32_t stage, bool losEnabled, bool animsEnabled, bool pathFinding)
{
    bool restart = false;
#ifdef ANIMATIONS_ENABLED
    common::animsEnabled = animsEnabled;
#endif
    bool welcome = true;

    PlayerNS::Player player;
    player.printer = printer;

    GameState gameState;
    gameState.moscowStage = stage;
    if (startAtFront) gameState.sendToWar();

    bool quit = false;
    bool skipWaitKey = false;

    while (!quit && gameState.phase != GamePhase::Victory) {
        player.resetPosition();
        stage = gameState.getEffectiveStage();
        player.returnedFromWar = gameState.hasBeenToWar;
        Level level(stage);
        level.pathFinding = pathFinding;

        VisionNS::Mask mask(&level);
        // the field of vision turns gradually, per frame in real time, per turn otherwise
        mask.maxTurn = common::realtimeEnabled ? VisionNS::TURN_PER_FRAME : VisionNS::TURN_PER_STEP;
#ifdef LOS_ENABLED
        if (losEnabled)
            mask.enable();
#endif
        printer->mask = &mask;

        level.printer = printer;
        level.audio.setLevel(&level);

        printer->player = &player;
        printer->levelName = gameState.getLevelName();

        if (gameState.phase == GamePhase::War) {
            level.isWarLevel = true;
        }
        if (gameState.getLandmark() == MoscowLandmark::Bolshoi) {
            level.isBolshoi = true;
        }
        if (gameState.hasKremlinPass) {
            player.hasKremlinPass = true;
        }

        level.addPerson(&player);
        if (gameState.phase != GamePhase::War) populateRoads(level);
        populateBuildings(level, gameState);
        populatePersons(level, gameState);
        populateItems(level, gameState);
        populateVehicles(level);
        if (startByCar && !gameState.hasBeenToWar) parkCarByPlayer(level, player.coord);
        populateAAGuns(level, gameState.moscowStage);
        player.manning = nullptr;   // the last level's gun is gone with it

        // fires burn everywhere; random disasters only from the third level and at the front
        level.events = new EnvironmentEvents(&level, gameState.phase != GamePhase::War && level.stage <= 2);

        if (welcome) {
            welcome = false;
            std::string welcomeMsg = restartedByZ ? "Uusi pultsari her\xC3\xA4\xC3\xA4 penkilt\xC3\xA4. Edellisest\xC3\xA4 ei kukaan kysy."
                : "Tervetuloa Grand Tyr Moscow'hun! \xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80";
            restartedByZ = false;
            printer->showMessage(welcomeMsg, &level, false);
        } else {
            printer->showMessage(gameState.getLevelName(), &level, false);
        }

        level.updateCamera();
        mask.init();
        printer->print(&level);

        bool nextLevel = false;

        if (common::realtimeEnabled) {
            InputNS::Input::raw();

            uint32_t npcTick = 0;
            InputNS::InputType queuedMove = InputNS::inputpending;
            uint32_t msSinceMove = 1000;
            constexpr uint32_t NPC_INTERVAL = 8; // NPCs act 2.5 times a second
            constexpr uint32_t FRAME_MS = 50;

            enum class Mode { Normal, Inventory, Aim };
            Mode mode = Mode::Normal;
            int invSelection = 0;
            bool throwMenu = false; // the inventory was opened with F, to throw something
            constexpr int INV_ITEMS = 17; // rows in overlayItems()

            // aim state
            float aimAngle = 0.0f;
            float aimNow = 0.0f;        // aimAngle plus the wobble, where the weapon really points
            uint32_t aimClockMs = 0;
            // the pistol: holstered, in hand (lowered, laser on the ground) or aimed
            bool gunDrawn = false;
            bool rifleRaised = false;   // the weapon in hand is the assault rifle
            uint32_t msSinceShot = 1000;
            float facing = 0.0f; // direction of the last step, for raising the gun
            uint32_t aimItemNum = 0;
            uint32_t aimTick = 0;
            bool aimIsShooting = false;

            // inventory item-to-throw mapping
            // maps inventory overlay row to decrementInventory() index
            // the aim angle snapped to one of the four directions
            // the longer you hold a cocked pistol, the likelier it goes off by itself
            uint32_t aimHeldMs = 0;
            float aimRisk = 0.0f;

            // driving: a terminal sends no key-up, so a held key is a key that keeps repeating.
            // Shift (the clutch) shows as capital letters; the browser sends 0x0E/0x0F for Shift down/up.
            uint32_t clutchHoldMs = 0;
            bool clutchLatched = false;
            uint32_t brakeMs = 0;
            uint32_t brakeRecentMs = 0;  // S pressed just now: Space shifts down instead of up
            player.driving = nullptr;
            // the keys that drive the car; anything else falls through to the usual keys
            auto driveKey = [&](char c) -> bool {
                Car* car = player.driving;
                if (!car) return false;
                // the terminal's key repeat starts only after a pause: hold a little longer than that
                const bool upper = c >= 'A' && c <= 'Z';
                if (upper) clutchHoldMs = 450;
                switch (c) {
                    case 'w': case 'i': case 'W': case 'I': ++car->controls.throttle; return true;
                    case 's': case 'k': case 'S': case 'K': brakeMs = 400; brakeRecentMs = 450; return true;
                    case 'a': case 'j': case 'A': case 'J': --car->controls.steer; return true;
                    case 'd': case 'l': case 'D': case 'L': ++car->controls.steer; return true;
                    case ' ': car->controls.shift = brakeRecentMs > 0 ? -1 : 1; return true;
                    case 0x0e: clutchLatched = true; return true;
                    case 0x0f: clutchLatched = false; return true;
                    case 'e': case 'E': leaveCar(&player, &level); return true;
                    default: return upper;
                }
            };

            // overlay row -> Player::decrementInventory / Item::createItem index
            // rows: kalja jaguar vodka makkara vobla omppo banaani ketjut veitsi kivi pamppu verkkarit kranaatti
            auto invIndexToThrowNum = [](int idx) -> uint32_t {
                constexpr uint32_t map[] = { 1, 2, 13, 3, 9, 10, 11, 4, 5, 6, 7, 8, 14 };
                if (idx >= 0 && idx < (int)(sizeof(map) / sizeof(map[0]))) return map[idx];
                return 0;
            };
            // how many of a throwable the player carries (decrementInventory numbering)
            auto throwableCount = [&](uint32_t num) -> uint32_t {
                const auto& inv = player.inventory;
                switch (num) {
                    case 1: return inv.kalja;    case 2: return inv.lonkka;  case 3: return inv.lenkki;
                    case 4: return inv.ketjut;   case 5: return inv.veitset; case 6: return inv.kivet;
                    case 7: return inv.pamput;   case 8: return inv.bootsit; case 9: return inv.kalat;
                    case 10: return inv.omppo;   case 11: return inv.bansku; case 13: return inv.vodka;
                    case 14: return inv.granata; default: return 0;
                }
            };
            // the item leaves the inventory only when it is actually thrown, so cancelling keeps it
            auto startThrowAim = [&](uint32_t throwNum) {
                if (throwNum > 0 && throwableCount(throwNum) > 0) {
                    aimItemNum = throwNum;
                    aimAngle = -M_PI / 2.0f;
                    aimTick = 0;
                    mode = Mode::Aim;
                    printer->showMessage("A/D t\xC3\xA4ht\xC3\xA4\xC3\xA4, v\xC3\xA4lily\xC3\xB6nti heitt\xC3\xA4\xC3\xA4, Esc peruu", &level, false);
                } else {
                    printer->showMessage("Ei tavaraa heitett\xC3\xA4v\xC3\xA4ksi.", &level, false);
                }
            };

            auto feedHunger = [&](uint32_t amount) {
                player.stats.hunger += amount;
                if (player.stats.hunger > 100) player.stats.hunger = 100;
            };

            auto useInventoryItem = [&](int idx) {
                auto& inv = player.inventory;
                switch(idx) {
                    case 0: // kalja
                        if (inv.kalja > 0) {
                            --inv.kalja;
                            ++inv.pullot;
                            printer->showMessage("Glub glub glub ..... Burb !!! Tyhj\xC3\xA4 pullo talteen.", &level, false);
                            common::sound("glug");
                            player.updatepromilles(2);
                            feedHunger(2);
                        } else printer->showMessage("Ei pivoa.", &level, false);
                        break;
                    case 1: // lonkka (Jaguar)
                        if (inv.lonkka > 0) {
                            --inv.lonkka;
                            printer->showMessage("Jaguar! Kofeiinia ja viinaa. Syd\xC3\xA4n hakkaa.", &level, false);
                            common::sound("glug");
                            player.updatepromilles(4);
                            feedHunger(1);
                        } else printer->showMessage("Ei Jaguaria.", &level, false);
                        break;
                    case 2: // vodka
                        if (inv.vodka > 0) {
                            const bool piss = !inv.pissVodka.empty() &&
                                (uint32_t)(std::rand() % inv.vodka) < inv.pissVodka.size();
                            --inv.vodka;
                            ++inv.pullot; // whatever was in it, the bottle is worth 20 kopecks
                            if (piss) {
                                const std::string seller = inv.pissVodka.back();
                                inv.pissVodka.pop_back();
                                printer->showMessage(seller + " myi sinulle n\xC3\xA4k\xC3\xB6j\xC3\xA4\xC3\xA4n kusta pullossa, hyi! Oksennat.", &level, false);
                                level.audio.addSound(player.coord, SoundType::Vomit,
                                    "BL\xC3\x84\xC3\x84\xC3\x84RGH! Oksennat kaaressa!", "Joku oksentaa", "...ykin\xC3\xA4\xC3\xA4...", 15);
                                HurtBy who(&level, seller + " (kusipullo)");
                                player.damage(2);
                                level.addItem(new Oksennus(player.coord));
                            } else {
                                printer->showMessage("Na zdorovje! Vodka polttaa kurkkua.", &level, false);
                            common::sound("glug");
                                player.updatepromilles(6);
                                feedHunger(1);
                            }
                        } else printer->showMessage("Ei vodkaa.", &level, false);
                        break;
                    case 3: // lenkki
                        if (inv.lenkki > 0) {
                            --inv.lenkki; player.powerup(1);
                            printer->showMessage("Mamamamakkaraa.", &level, false);
                            feedHunger(25);
                        } else printer->showMessage("Ei makkaraa.", &level, false);
                        break;
                    case 4: // kala
                        if (inv.kalat > 0) {
                            --inv.kalat;
                            if (random(70)) { player.damage(2); printer->showMessage("Syomasi kala sisalsi matoja...", &level, false); }
                            else { player.powerup(4); printer->showMessage("Kaloreja kalasta.", &level, false); feedHunger(30); }
                        } else printer->showMessage("Ei kalaa.", &level, false);
                        break;
                    case 5: // omppo
                        if (inv.omppo > 0) {
                            --inv.omppo;
                            if (random(50)) { player.damage(1); printer->showMessage("Antonovka oli pilaantunut.", &level, false); }
                            else { player.powerup(2); printer->showMessage("orb.", &level, false); feedHunger(15); }
                        } else printer->showMessage("Ei antonovkaa.", &level, false);
                        break;
                    case 6: // bansku
                        if (inv.bansku > 0) {
                            --inv.bansku;
                            if (random(50)) { printer->showMessage("Banaani ei vaikuttanut.", &level, false); feedHunger(5); }
                            else { player.powerup(1); printer->showMessage("Babababananiii", &level, false); feedHunger(10); }
                        } else printer->showMessage("Ei banaania.", &level, false);
                        break;
                    case 12: // kranaatti can only be thrown
                        startThrowAim(14);
                        break;
                    case 15: // rynkky: raise the assault rifle; hold Space for full auto
                        if (inv.rynkky > 0 && inv.ammukset > 0) {
                            gunDrawn = true;
                            rifleRaised = true;
                            aimIsShooting = true;
                            aimAngle = facing;
                            aimTick = 0;
                            mode = Mode::Aim;
                            printer->showMessage("Rynkky olalle. Pid\xC3\xA4 v\xC3\xA4lily\xC3\xB6nti\xC3\xA4 pohjassa: sarjatulta!", &level, false);
                        } else if (inv.rynkky == 0) {
                            printer->showMessage("Ei rynn\xC3\xA4kk\xC3\xB6kiv\xC3\xA4\xC3\xA4ri\xC3\xA4.", &level, false);
                        } else {
                            printer->showMessage("Ei ammuksia.", &level, false);
                        }
                        break;
                    case 13: // pistooli: raise it towards the last step and aim
                        if (inv.pistooli > 0 && inv.ammukset > 0) {
                            gunDrawn = true;
                            rifleRaised = false;
                            aimIsShooting = true;
                            aimAngle = facing;
                            aimTick = 0;
                            mode = Mode::Aim;
                            printer->showMessage("T\xC3\xA4ht\xC3\xA4\xC3\xA4t. A/D k\xC3\xA4\xC3\xA4nn\xC3\xA4, v\xC3\xA4lily\xC3\xB6nti ampuu, Esc laskee aseen.", &level, false);
                        } else if (inv.pistooli == 0) {
                            printer->showMessage("Ei pistoolia.", &level, false);
                        } else {
                            printer->showMessage("Ei ammuksia.", &level, false);
                        }
                        break;
                    default:
                        printer->showMessage("Tätä ei voi käyttää.", &level, false);
                        break;
                }
            };

            while (!nextLevel && !quit) {
                printer->removeMessage();

                try {
                    // drain every key that arrived since the last frame: held-down keys
                    // repeat faster than frames, and a backlog would keep the player
                    // walking long after the key is released
                    std::string keys;
                    for (char k; (k = InputNS::Input::getRawCharNonBlocking()) != 0;) keys += k;
                    if (keys.empty()) keys += '\0';

                    // the aim never holds still: two waves on top of where you point, wider the drunker you are
                    aimClockMs += FRAME_MS;
                    {
                        const float amp = 0.10f + player.stats.promilles / 80.0f;
                        aimNow = aimAngle + amp * (0.65f * sinf(aimClockMs / 260.0f) + 0.35f * sinf(aimClockMs / 113.0f + 1.3f));
                    }

                    for (const char c : keys) {
                    if (c == 'z') throw RestartGame();
                    // behind an anti-aircraft gun every key works the gun
                    if (player.manning) {
                        if (c != 0) gunnerKey(c, player, level, *printer, msSinceShot);
                        mode = Mode::Normal;
                        continue;
                    }

                    switch (mode) {
                    case Mode::Normal: {
                        if (c != 0 && player.driving && driveKey(c)) break;
                        if (c != 0) {
                            switch(c) {
                                case 'w': case 'i': queuedMove = InputNS::up; break;
                                case 's': case 'k': queuedMove = InputNS::down; break;
                                case 'a': case 'j': queuedMove = InputNS::left; break;
                                case 'd': case 'l': queuedMove = InputNS::right; break;
                                case 'q': case '\t': mode = Mode::Inventory; throwMenu = false; invSelection = 0; break;
                                case 'f': {
                                    // F asks what to throw: the menu opens on the first throwable thing carried
                                    int first = -1;
                                    for (int r = 0; r < INV_ITEMS && first < 0; ++r)
                                        if (throwableCount(invIndexToThrowNum(r)) > 0) first = r;
                                    if (first < 0) {
                                        printer->showMessage("Ei mit\xC3\xA4\xC3\xA4n heitett\xC3\xA4v\xC3\xA4\xC3\xA4.", &level, false);
                                    } else {
                                        mode = Mode::Inventory;
                                        throwMenu = true;
                                        invSelection = first;
                                    }
                                    break;
                                }
                                case 27: // Esc puts a drawn pistol back in the belt
                                    if (gunDrawn) {
                                        gunDrawn = false;
                                        printer->showMessage("Laitat aseen vy\xC3\xB6lle.", &level, false);
                                    }
                                    break;
                                case ' ': // Space raises the best gun to aim; Space again fires
                                    if (player.inventory.rynkky > 0) useInventoryItem(15);
                                    else if (player.inventory.pistooli > 0) useInventoryItem(13);
                                    else printer->showMessage("Sinulla ei ole asetta.", &level, false);
                                    break;
                                default: break;
                            }
                        }

                        if (player.driving) { queuedMove = InputNS::inputpending; break; }
                        // the drunker you are, the longer each step takes; an early key waits its turn
                        InputNS::InputType input = InputNS::inputpending;
                        const uint32_t stepMs = 80 + player.stats.promilles * 5;
                        if (queuedMove != InputNS::inputpending && msSinceMove >= stepMs) {
                            input = queuedMove;
                            queuedMove = InputNS::inputpending;
                            msSinceMove = 0;
                        }

                        if (input != InputNS::inputpending) {
                            input = applyDrunkEffects(input, player, &level, *printer);
                            switch (input) {
                                case InputNS::up:    facing = -M_PI / 2.0f; break;
                                case InputNS::down:  facing = M_PI / 2.0f; break;
                                case InputNS::left:  facing = M_PI; break;
                                case InputNS::right: facing = 0.0f; break;
                                default: break;
                            }
                            processInput(input, player, &level, *printer);
                            InputNS::Input::raw();
                            level.cleanDead();
                            level.cleanDiscardedItems();
                        }
                        break;
                    }

                    case Mode::Inventory: {
                        if (c != 0) {
                            switch(c) {
                                case 'w': case 'i':
                                    invSelection = (invSelection - 1 + INV_ITEMS) % INV_ITEMS;
                                    break;
                                case 's': case 'k':
                                    invSelection = (invSelection + 1) % INV_ITEMS;
                                    break;
                                case 'e': case '\r':
                                    if (throwMenu && c == '\r') {
                                        startThrowAim(invIndexToThrowNum(invSelection));
                                        break;
                                    }
                                    useInventoryItem(invSelection); // eat, drink or aim the pistol
                                    if (mode == Mode::Inventory) mode = Mode::Normal;
                                    break;
                                case 'f': // throw -> aim mode
                                    startThrowAim(invIndexToThrowNum(invSelection));
                                    break;
                                case 'q': case '\t': case 27: // Q, Tab or ESC closes
                                    mode = Mode::Normal;
                                    break;
                            }
                        }
                        break;
                    }

                    case Mode::Aim: {

                        if (c != 0) {
                            switch(c) {
                                case 'a': case 'j':
                                    aimAngle -= 0.15f;
                                    break;
                                case 'd': case 'l':
                                    aimAngle += 0.15f;
                                    break;
                                case ' ': // fire/throw/shoot
                                    if (aimIsShooting && rifleRaised) {
                                        // held Space repeats: full auto, about eight rounds a second, wildly
                                        if (msSinceShot < 120) break;
                                        msSinceShot = 0;
                                        if (player.inventory.ammukset == 0) {
                                            printer->showMessage("*KLIK* Lipas tyhj\xC3\xA4!", &level, false);
                                            break;
                                        }
                                        --player.inventory.ammukset;
                                        const float spread = ((std::rand() % 100) - 50) / 110.0f * (1.0f + player.stats.promilles / 30.0f);
                                        Person* hit = level.fireBullet(&player, aimNow + spread, 18, std::rand() % 3 + 3, 85);
                                        level.audio.addSound(player.coord, SoundType::Gunshot,
                                            "R\xC3\x84K\xC3\x84K\xC3\x84K\xC3\x84! Rynn\xC3\xA4kk\xC3\xB6kiv\xC3\xA4\xC3\xA4ri laulaa!", "Sarjatulta!", "...sarjatulta kaukaa...", 35);
                                        level.alertCops(&player, 35);
                                        if (hit) printer->showMessage("Osuma: " + level.nameOf(hit) + "!", &level, false);
                                        break; // the rifle stays up
                                    }
                                    if (aimIsShooting) {
                                        player.shootAt(aimNow);
                                        aimIsShooting = false;
                                    } else {
                                        if (player.decrementInventory(aimItemNum))
                                            player.executeThrowAt(aimItemNum, aimNow);
                                    }
                                    mode = Mode::Normal;
                                    break;
                                case 27: case '\t': // ESC or Tab cancels; a raised pistol is lowered, still in hand
                                    if (aimIsShooting) printer->showMessage("Lasket aseen. Se on yh\xC3\xA4 k\xC3\xA4""dess\xC3\xA4 (Esc laittaa vy\xC3\xB6lle).", &level, false);
                                    aimIsShooting = false;
                                    mode = Mode::Normal;
                                    break;
                            }
                        }
                        ++aimTick;
                        break;
                    }
                    }
                    }

                    if (player.inventory.pistooli == 0) gunDrawn = false;
                    // a pistol in hand, pointing at the ground, can always go off; now and
                    // then it is pointing at your own foot
                    float lowered = facing;
                    if (gunDrawn && mode != Mode::Aim) {
                        static uint32_t swayMs = 0;
                        swayMs += FRAME_MS;
                        lowered = facing + sinf(swayMs / 420.0f) * (0.35f + player.stats.promilles / 60.0f);
                        const float p = 0.0006f * (1.0f + player.stats.promilles / 15.0f);
                        if (player.inventory.ammukset > 0 && (float)(std::rand() % 100000) < p * 100000.0f) {
                            --player.inventory.ammukset;
                            common::sound("pistol", 1.0f);
                            level.audio.addSound(player.coord, SoundType::Gunshot,
                                "PAM! Pistooli laukesi!", "Pistoolinlaukaus", "...pamaus kaukaa...", 30);
                            level.alertCops(&player, 30);
                            if (random(35)) {
                                const bool knee = random(25);
                                common::hitEvent(player.coord, knee ? 8 : 4);
                                HurtBy who(&level, "oma pistoolisi");
                                player.damage(knee ? std::rand() % 6 + 6 : std::rand() % 4 + 2);
                                printer->showMessage(knee ? "PAM! Ase laukesi ja ammuit itse\xC3\xA4si polveen! Kipu on sanoinkuvaamaton."
                                                          : "PAM! Ase laukesi ja ammuit itse\xC3\xA4si jalkaan! AUTS!", &level, false);
                            } else {
                                printer->showMessage("PAM! Ase laukesi maahan. Korvat soivat.", &level, false);
                            }
                        }
                    }

                    msSinceShot += FRAME_MS;
                    if (mode == Mode::Aim && aimIsShooting) {
                        aimHeldMs += FRAME_MS;
                        // about ten seconds to the top sober, much quicker drunk
                        const float fullMs = 10000.0f / (1.0f + player.stats.promilles / 15.0f);
                        aimRisk = std::min(1.0f, aimHeldMs / fullMs);
                        if ((float)(std::rand() % 10000) < aimRisk * aimRisk * 0.05f * 10000.0f) {
                            player.shootAt(aimNow);
                            printer->showMessage("PAM! Ase laukesi vahingossa!", &level, false);
                            aimIsShooting = false;
                            mode = Mode::Normal;
                        }
                    } else {
                        aimHeldMs = 0;
                        aimRisk = 0.0f;
                    }

                    if (player.metroTrip) {
                        if (rideMetro(level, mask, player, printer, gameState, nextLevel)) {
                            quit = restart = true;
                            break;
                        }
                        InputNS::Input::raw();
                        if (nextLevel) break;
                    }

                    msSinceMove += FRAME_MS;
                    if (Car* car = player.driving) {
                        car->controls.brake = brakeMs > 0;
                        car->controls.clutch = clutchLatched || clutchHoldMs > 0;
                    }
                    clutchHoldMs = clutchHoldMs > FRAME_MS ? clutchHoldMs - FRAME_MS : 0;
                    brakeMs = brakeMs > FRAME_MS ? brakeMs - FRAME_MS : 0;
                    brakeRecentMs = brakeRecentMs > FRAME_MS ? brakeRecentMs - FRAME_MS : 0;
                    level.npcFrame(FRAME_MS);
                    level.vehicleFrame(FRAME_MS);
                    updateDriverGaze(&player, FRAME_MS);
                    level.cleanDead();

                    ++npcTick;
                    if (npcTick >= NPC_INTERVAL) {
                        npcTick = 0;
                        level.buildingTurn();
                        level.spreadNews();
                        level.updateVehicles();
                        level.skyTick();
                        level.robberyTick();
                        level.fsbTick();
                        if (level.events) level.events->update();

                        // ambient audio sources
                        for (auto& v : level.vehicles) {
                            Bus* bus = dynamic_cast<Bus*>(v);
                            if (bus) {
                                level.audio.addSound(bus->coord, SoundType::Engine,
                                    "Bussin moottori jyrisee", "Bussin \xC3\xA4\xC3\xA4nt\xC3\xA4", "...moottoria kaukaa...", 20);
                            }
                        }
                        for (auto& b : level.buildings) {
                            if (b->type == EHypermarket) {
                                Hypermarket* hm = dynamic_cast<Hypermarket*>(b);
                                if (hm && hm->onFire) {
                                    Coord fc { (b->wallLeft + b->wallRight) / 2, (b->wallTop + b->wallBot) / 2 };
                                    level.audio.addSound(fc, SoundType::Fire,
                                        "Tulipalo raivoaa! R\xC3\xA4iskyy ja paukkuu!", "Liekkien \xC3\xA4\xC3\xA4ni\xC3\xA4", "...palon huminaa...", 30);
                                }
                            }
                        }

                        level.cleanDead();
                        level.cleanDiscardedItems();
                        player.updateTurn();

                        // check Putin death -> celebration
                        if (level.putinDead && !level.celebrating) {
                            bool insideKremlin = false;
                            for (auto b : level.buildings) {
                                if (b->type == EKremlin) {
                                    if (player.coord.x > b->wallLeft && player.coord.x < b->wallRight &&
                                        player.coord.y > b->wallTop && player.coord.y < b->wallBot) {
                                        insideKremlin = true;
                                    }
                                    break;
                                }
                            }
                            if (!insideKremlin) {
                                level.celebrating = true;
                                player.killedPutin = true;
                                printer->showMessage("Kansa tajuaa mitä tapahtui... VOVA ON KUOLLUT! Juhla alkaa!", &level, false);
                            }
                        }

                        // check if player is at bus stop and bus is there
                        for (auto& v : level.vehicles) {
                            Bus* bus = dynamic_cast<Bus*>(v);
                            if (bus && bus->atStop) {
                                Coord stopCoord { bus->stopX, bus->routeY - 1 };
                                if (player.coord == stopCoord) {
                                    printer->showMessage("Bussi saapui! Nouse kyytiin? (K=kyllä)", &level, false);
                                    printer->print(&level);
                                    InputNS::Input::noraw();

                                    auto result = runBusMinigame(&player, &level);
                                    if (result.levelChange != 0 &&
                                        (gameState.phase == GamePhase::Moscow || gameState.phase == GamePhase::ReturnMoscow)) {
                                        int newStage = (int)gameState.moscowStage + result.levelChange;
                                        newStage = std::max(1, std::min(8, newStage));
                                        gameState.moscowStage = (uint32_t)newStage;
                                        nextLevel = true;
                                    }

                                    InputNS::Input::raw();
                                    break;
                                }
                            }
                        }
                    }

                    level.stepAnimations();
                    level.updateCamera();
                    mask.init();
                    printer->print(&level);
                    // sounds stay on screen for a full NPC interval and are cleared after the
                    // last frame before the next tick, so the player's own shots are never lost
                    if (npcTick == NPC_INTERVAL - 1) level.audio.clear();

                    if (player.manning) {
                        // the gun's sight reaches far, and the shells left show on top
                        const Coord g = player.manning->centre();
                        printer->aimRisk = -1.0f;
                        printer->aimLength = AAGun::RANGE;
                        printer->gunAmmo = (int)player.manning->ammo;
                        printer->printAimLine(&level, g.x, g.y, player.manning->angle);
                        printer->gunAmmo = -1;
                    } else if (mode == Mode::Inventory) {
                        printer->printInventoryOverlay(&player.inventory, invSelection, throwMenu);
                    } else if (mode == Mode::Aim) {
                        printer->aimRisk = aimIsShooting ? aimRisk : -1.0f;
                        printer->aimLength = 20;
                        printer->printAimLine(&level, player.coord.x, player.coord.y, aimNow);
                    } else if (gunDrawn) {
                        // laser from the lowered pistol, a few steps ahead on the ground
                        printer->aimRisk = -1.0f;
                        printer->aimLength = 3;
                        printer->printAimLine(&level, player.coord.x, player.coord.y, lowered);
                    }

                } catch (const RestartGame&) {
                    throw;
                } catch(...) {
                    quit = true;
                    skipWaitKey = true;
                    break;
                }

                if (player.wonGame) {
                    gameState.phase = GamePhase::Victory;
                    nextLevel = true;
                    break;
                }

                if (player.conscripted) {
                    player.conscripted = false;
                    if (conscriptionGoesToWar(gameState)) {
                        printer->setMessage(CONSCRIPTED_TO_WAR);
                        printer->print(&level);
                        common::sleep(3000);
                        gameState.sendToWar();
                        nextLevel = true;
                    } else {
                        mourn(level, mask, player, printer, CONSCRIPTED_DEATH);
                        quit = restart = true;
                    }
                    break;
                }
                if (player.inJail) {
                    player.inJail = false;
                    if (gameState.phase == GamePhase::Moscow || gameState.phase == GamePhase::ReturnMoscow) {
                        printer->setMessage("Politsijat vievät sinut oikeuteen...\nTuomari: 'Rintamalle!'");
                        printer->print(&level);
                        common::sleep(2000);
                        gameState.sendToWar();
                        nextLevel = true;
                    } else {
                        mourn(level, mask, player, printer, "Olet kuollut. Tappajasi: sotaoikeus, SIZOn sellissä.");
                        quit = restart = true;
                    }
                    break;
                } else if (player.stats.health < 1) {
                    mourn(level, mask, player, printer, player.diedOfAids() ? std::string(AIDS_DEATH) : "Olet kuollut. Tappajasi: " +
                          (player.lastHurtBy.empty() ? std::string("Moskova itse") : player.lastHurtBy) + ".");
                    quit = restart = true;
                    break;
                } else if (player.stats.promilles < 1) {
                    player.stats.health = 0;
                    mourn(level, mask, player, printer, "Olet kuollut. Tappajasi: krapula.");
                    quit = restart = true;
                    break;
                } else if (player.stats.promilles > 34 && common::random(10)) {
                    player.stats.health = 0;
                    mourn(level, mask, player, printer, "Olet kuollut. Tappajasi: vodka. Joit itsesi hengilt\xC3\xA4.");
                    quit = restart = true;
                    break;
                }

                common::frameSleep(FRAME_MS);
            }

            InputNS::Input::noraw();

        } else {
            // original turn-based loop
            while(!nextLevel && !quit) {
                printer->removeMessage();

                try {
                    InputNS::InputType input = InputNS::Input::getInput();
                    input = applyDrunkEffects(input, player, &level, *printer);
                    bool turn = processInput(input, player, &level, *printer);
                    if (player.metroTrip) {
                        const bool died = rideMetro(level, mask, player, printer, gameState, nextLevel);
                        InputNS::Input::noraw();
                        if (died) { quit = restart = true; break; }
                        if (nextLevel) break;
                    }
                    level.updateCamera();
                    mask.init();
                    level.cleanDead();
                    level.cleanDiscardedItems();
                    level.stepAnimations();

                    printer->print(&level);

                    level.audio.clear();
                    level.buildingTurn();
                    level.npcTurn();
                    level.updateVehicles();
                    level.skyTick();
                    level.robberyTick();
                    level.fsbTick();
                    if (level.events) level.events->update();

                    // ambient audio sources
                    for (auto& v : level.vehicles) {
                        Bus* bus = dynamic_cast<Bus*>(v);
                        if (bus) {
                            level.audio.addSound(bus->coord, SoundType::Engine,
                                "Bussin moottori jyrisee", "Bussin \xC3\xA4\xC3\xA4nt\xC3\xA4", "...moottoria kaukaa...", 20);
                        }
                    }
                    for (auto& b : level.buildings) {
                        if (b->type == EHypermarket) {
                            Hypermarket* hm = dynamic_cast<Hypermarket*>(b);
                            if (hm && hm->onFire) {
                                Coord fc { (b->wallLeft + b->wallRight) / 2, (b->wallTop + b->wallBot) / 2 };
                                level.audio.addSound(fc, SoundType::Fire,
                                    "Tulipalo raivoaa! R\xC3\xA4iskyy ja paukkuu!", "Liekkien \xC3\xA4\xC3\xA4ni\xC3\xA4", "...palon huminaa...", 30);
                            }
                        }
                    }

                    level.cleanDead();
                    level.cleanDiscardedItems();

                    // check Putin death -> celebration
                    if (level.putinDead && !level.celebrating) {
                        bool insideKremlin = false;
                        for (auto b : level.buildings) {
                            if (b->type == EKremlin) {
                                if (player.coord.x > b->wallLeft && player.coord.x < b->wallRight &&
                                    player.coord.y > b->wallTop && player.coord.y < b->wallBot) {
                                    insideKremlin = true;
                                }
                                break;
                            }
                        }
                        if (!insideKremlin) {
                            level.celebrating = true;
                            player.killedPutin = true;
                            printer->showMessage("Kansa tajuaa mitä tapahtui... VOVA ON KUOLLUT! Juhla alkaa!", &level, false);
                        }
                    }

                    if (turn) {
                        player.updateTurn();
                    }
                } catch (const RestartGame&) {
                    throw;
                } catch(...) {
                    quit = true;
                    skipWaitKey = true;
                    break;
                }

                if (player.wonGame) {
                    gameState.phase = GamePhase::Victory;
                    nextLevel = true;
                    break;
                }

                if (player.conscripted) {
                    player.conscripted = false;
                    printer->setMessage(conscriptionGoesToWar(gameState) ? CONSCRIPTED_TO_WAR : CONSCRIPTED_DEATH);
                    printer->print(&level);
                    if (conscriptionGoesToWar(gameState)) {
                        InputNS::Input::waitKey();
                        gameState.sendToWar();
                        nextLevel = true;
                    } else {
                        quit = true;
                    }
                    break;
                }
                if (player.inJail) {
                    player.inJail = false;
                    if (gameState.phase == GamePhase::Moscow || gameState.phase == GamePhase::ReturnMoscow) {
                        printer->setMessage("Politsijat vievät sinut oikeuteen...\nTuomari: 'Rintamalle!'");
                        printer->print(&level);
                        InputNS::Input::waitKey();
                        gameState.sendToWar();
                        nextLevel = true;
                    } else {
                        printer->setMessage("SIZOssa... Game over!");
                        printer->print(&level);
                        quit = true;
                    }
                    break;
                } else if (player.stats.health < 1) {
                    quit = true;
                    printer->setMessage(player.diedOfAids() ? AIDS_DEATH : "Terveytesi pettää... kemahdat tantereeseen pitkäksesi .........");
                    printer->print(&level);
                    break;
                } else if (player.stats.promilles < 1) {
                    quit = true;
                    printer->setMessage("Yhktäkkii tajuat krapuliuksen tulevan. Et kestä enää...");
                    printer->print(&level);
                    break;
                } else if (player.stats.promilles > 34 && common::random(10)) {
                    quit = true;
                    printer->setMessage("HIC ! .... Kompastut omiin jalkoihisi...huomaat ettet elä ennee...");
                    printer->print(&level);
                    break;
                }

                printer->print(&level);
            }
        }
    }

    if (gameState.phase == GamePhase::Victory) {
        printer->clear();
        printer->printChar("GRAND TYR MOSCOW - \xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80 p\xC3\xA4\xC3\xA4ttyi!\n\n");
        printer->printChar("Teljet oven takanasi...\n");
        printer->printChar("Viinirivistöt loistavat edessäsi kuin paratiisi.\n\n");
        common::sleep(2000);

        const char* bottles[] = {
            "Glub glub... Château Pétrus 1945... *GULP*",
            "Romanée-Conti... *GLUG GLUG GLUG*",
            "Massandra Collection... *BURP*",
            "Dom Pérignon... *hic*... suoraan pullosta...",
            "Vodka Beluga... *GULP*... kotiinpaluu!",
            "Screaming Eagle... *glub*... ei huono...",
        };
        for (int i = 0; i < 6; ++i) {
            printer->printChar(bottles[i]);
            printer->printChar("\n");
            common::sleep(1500);
        }
        printer->printChar("\n");
        common::sleep(1000);
        printer->printChar("Normaalit ihmiset etsiv\xC3\xA4t nirvanaa...\n");
        common::sleep(1500);
        printer->printChar("Ven\xC3\xA4j\xC3\xA4ll\xC3\xA4 pyrit\xC3\xA4\xC3\xA4n DELIRIUM TREMENSIIN.\n");
        printer->printChar("  (\xD0\x91\xD0\x95\xD0\x9B\xD0\x90\xD0\xAF \xD0\x93\xD0\x9E\xD0\xA0\xD0\xAF\xD0\xA7\xD0\x9A\xD0\x90)\n\n");
        common::sleep(2000);
        printer->printChar("Ja sin\xC3\xA4 saavutit sen. Vovan viinakellarissa.\n");
        printer->printChar("Loputtomasti vodkaa ja aikaa. Ven\xC3\xA4l\xC3\xA4inen unelma.\n\n");
        common::sleep(2000);
    }

    if (restart) return true;
    if (!skipWaitKey) InputNS::Input::waitKey();
    printer->endGame();
    printer->printScore(&player.scoreBoard);
    InputNS::Input::waitKey();
    return false;
}

int init(int argc, char *argv[], Printer* printer)
{
    srand(time(NULL));
    printer->show_console_cursor(false);

    uint32_t stage = 1;
    bool animsEnabled = false;
    bool losEnabled = false;
    bool pathFinding = false;
    bool realtime = false;

    for (int i = 0; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--los") {
            losEnabled = true;
        } else if (a == "--anims") {
            animsEnabled = true;
        } else if ((a == "-h") || (a == "--help")) {
            printer->printHelp();
            InputNS::Input::waitKey();
            return 0;
        } else if (a == "--car") {
            startByCar = true;
        } else if (a == "--front") {
            startAtFront = true;
        } else if (a == "--path") {
            pathFinding = true;
        } else if ((a == "--realtime") || (a == "--rt")) {
            realtime = true;
        } else if ((a == "--all") || (a == "-a")) {
            losEnabled = true;
            animsEnabled = true;
            pathFinding = true;
        } else if (a == "--level") {
            ++i;
            if (i == argc) {
                printer->printErr("ERROR: --level requires a parameter!");
                return -1;
            }
            uint32_t tmp = std::atoi(argv[i]);
            if ((tmp > 100) || (tmp == 0)) {
                printer->printErr("ERROR: --level parameter out of range");
                return -1;
            } else {
                stage = tmp;
            }
        } else if (i > 0) {
            printer->printErr("ERROR: unknown parameter " + a);
            return -1;
        }
    }

    common::realtimeEnabled = realtime;

    // the browser client plays its own intro
    if (!std::getenv("PULTSARI_WEB")) {
        IntroNS::Intro i(printer);
        i.show();
    }

    try {
        // every death, and Z, is followed by a new drunk
        for (;;) {
            try {
                if (!mainloop(printer, stage, losEnabled, animsEnabled, pathFinding)) break;
            } catch (const RestartGame&) {
                restartedByZ = true;
            }
        }
    }
    catch(const std::out_of_range& e)
    {
        // reset terminal
        InputNS::Input::noraw();
        std::cout << e.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "unknown exception caught, exit!" << std::endl;
    }

    printer->show_console_cursor(true);

    return 0;
}
