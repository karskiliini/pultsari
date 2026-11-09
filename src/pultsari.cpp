
#include "pultsari.hpp"
#include "common.hpp"
#include "intro.hpp"
#include "input.hpp"
#include "building.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "coord.hpp"
#include "item.hpp"
#include "mask.hpp"
#include "scoreboard.hpp"
#include "vehicle.hpp"
#include "events.hpp"
#include "gamestate.hpp"
#include <iostream>
#include <exception>
#include <cmath>
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
    for (int i = 0; i < std::rand() % 4 + 2; ++i)
        level.addItem(ERoskis);
    if (random(20))
        level.addItem(EPropusk);
    {
        ViinaKellari* vk = new ViinaKellari(Coord{315, 23}, &level);
        level.addItem(vk);
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

    // Alko - near player start area
    b = new Alko();
    b->setOffset(30, 85);
    level.addBuilding(b);

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

    // MILIISIT - more near Kremlin
    {
        int copCount = (int)(2 + kremlinProximity * 6) + std::rand()%3;
        for (int i = 0; i < copCount; ++i) {
            if (random(60 + (int)(kremlinProximity * 30))) {
                level.createPerson(level.freePosition(), poliisi);
            }
        }
    }

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
        constexpr uint32_t BUS_ROUTE_Y = 110;
        constexpr uint32_t BUS_STOP_X = 70;

        Bus* bus = new Bus(BUS_ROUTE_Y, BUS_STOP_X);
        level.addVehicle(bus);

        BusStop* stop = new BusStop(Coord{BUS_STOP_X, BUS_ROUTE_Y - 1});
        level.addItem(stop);
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
            throw std::out_of_range("quit!");
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
        player.damage(1);
        if (input != InputNS::quit)
            input = InputNS::throwup;
    }
    return input;
}

void mainloop(Printer* printer, uint32_t stage, bool losEnabled, bool animsEnabled, bool pathFinding)
{
#ifdef ANIMATIONS_ENABLED
    common::animsEnabled = animsEnabled;
#endif
    bool welcome = true;

    PlayerNS::Player player;
    player.printer = printer;

    GameState gameState;
    gameState.moscowStage = stage;

    bool quit = false;
    bool skipWaitKey = false;

    while (!quit && gameState.phase != GamePhase::Victory) {
        player.resetPosition();
        stage = gameState.getEffectiveStage();
        player.returnedFromWar = gameState.hasBeenToWar;
        Level level(stage);
        level.pathFinding = pathFinding;

        VisionNS::Mask mask(&level);
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
        populateBuildings(level, gameState);
        populatePersons(level, gameState);
        populateItems(level, gameState);
        populateVehicles(level);

        if (gameState.phase == GamePhase::War || level.stage > 2) {
            level.events = new EnvironmentEvents(&level);
        }

        if (welcome) {
            welcome = false;
            std::string welcomeMsg = "Tervetuloa P\xD0\x9F""LTSARI 2:een! \xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80";
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
            constexpr int INV_ITEMS = 15;

            // aim state
            float aimAngle = 0.0f;
            uint32_t aimItemNum = 0;
            uint32_t aimTick = 0;
            bool aimIsShooting = false;

            // inventory item-to-throw mapping
            // maps inventory overlay row to decrementInventory() index
            // overlay row -> Player::decrementInventory / Item::createItem index
            // rows: kalja jaguar vodka makkara vobla omppo banaani ketjut veitsi kivi pamppu verkkarit kranaatti
            auto invIndexToThrowNum = [](int idx) -> uint32_t {
                constexpr uint32_t map[] = { 1, 2, 13, 3, 9, 10, 11, 4, 5, 6, 7, 8, 14 };
                if (idx >= 0 && idx < (int)(sizeof(map) / sizeof(map[0]))) return map[idx];
                return 0;
            };
            auto startThrowAim = [&](uint32_t throwNum) {
                if (throwNum > 0 && player.decrementInventory(throwNum)) {
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
                            printer->showMessage("Glub glub glub ..... Burb !!!", &level, false);
                            common::sound("glug");
                            player.updatepromilles(2);
                            feedHunger(2);
                        } else printer->showMessage("Ei kaljaa.", &level, false);
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
                            if (piss) {
                                const std::string seller = inv.pissVodka.back();
                                inv.pissVodka.pop_back();
                                printer->showMessage(seller + " myi sinulle n\xC3\xA4k\xC3\xB6j\xC3\xA4\xC3\xA4n kusta pullossa, hyi! Oksennat.", &level, false);
                                level.audio.addSound(player.coord, SoundType::Vomit,
                                    "BL\xC3\x84\xC3\x84\xC3\x84RGH! Oksennat kaaressa!", "Joku oksentaa", "...ykin\xC3\xA4\xC3\xA4...", 15);
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
                        } else printer->showMessage("Ei lenkkiä.", &level, false);
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
                            if (random(50)) { player.damage(1); printer->showMessage("Omppo oli pilaantunut.", &level, false); }
                            else { player.powerup(2); printer->showMessage("orb.", &level, false); feedHunger(15); }
                        } else printer->showMessage("Ei omppoa.", &level, false);
                        break;
                    case 6: // bansku
                        if (inv.bansku > 0) {
                            --inv.bansku;
                            if (random(50)) { printer->showMessage("Bansku ei vaikuttanut.", &level, false); feedHunger(5); }
                            else { player.powerup(1); printer->showMessage("Babababananiii", &level, false); feedHunger(10); }
                        } else printer->showMessage("Ei banaania.", &level, false);
                        break;
                    case 12: // kranaatti can only be thrown
                        startThrowAim(14);
                        break;
                    case 13: // pistooli - go to aim mode for shooting
                        if (inv.pistooli > 0 && inv.ammukset > 0) {
                            aimIsShooting = true;
                            aimAngle = -M_PI / 2.0f;
                            aimTick = 0;
                            mode = Mode::Aim;
                            printer->showMessage("A/D = tähtää, SPACE = ammu, ESC = peru", &level, false);
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
                    char c = InputNS::Input::getRawCharNonBlocking();
                    if (c == 'z') throw std::out_of_range("quit!");

                    switch (mode) {
                    case Mode::Normal: {
                        if (c != 0) {
                            switch(c) {
                                case 'w': case 'i': queuedMove = InputNS::up; break;
                                case 's': case 'k': queuedMove = InputNS::down; break;
                                case 'a': case 'j': queuedMove = InputNS::left; break;
                                case 'd': case 'l': queuedMove = InputNS::right; break;
                                case '\t': mode = Mode::Inventory; invSelection = 0; break;
                                default: break;
                            }
                        }

                        // the drunker you are, the longer each step takes; an early key waits its turn
                        InputNS::InputType input = InputNS::inputpending;
                        const uint32_t stepMs = 90 + player.stats.promilles * 8;
                        if (queuedMove != InputNS::inputpending && msSinceMove >= stepMs) {
                            input = queuedMove;
                            queuedMove = InputNS::inputpending;
                            msSinceMove = 0;
                        }

                        if (input != InputNS::inputpending) {
                            input = applyDrunkEffects(input, player, &level, *printer);
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
                                case 'q': // use (eat/drink)
                                    useInventoryItem(invSelection);
                                    mode = Mode::Normal;
                                    break;
                                case 'e': // throw -> aim mode
                                    startThrowAim(invIndexToThrowNum(invSelection));
                                    break;
                                case '\t': case 27: // tab or ESC closes
                                    mode = Mode::Normal;
                                    break;
                            }
                        }
                        break;
                    }

                    case Mode::Aim: {
                        float sway = sinf((float)aimTick * 0.15f) * (float)player.stats.promilles / 100.0f;
                        aimAngle += sway * 0.05f;

                        if (c != 0) {
                            switch(c) {
                                case 'a': case 'j':
                                    aimAngle -= 0.15f;
                                    break;
                                case 'd': case 'l':
                                    aimAngle += 0.15f;
                                    break;
                                case ' ': { // fire/throw/shoot
                                    float dx = cosf(aimAngle);
                                    float dy = sinf(aimAngle);
                                    int idx = (fabsf(dx) > fabsf(dy))
                                        ? (dx > 0 ? 0 : 2)
                                        : (dy > 0 ? 1 : 3);
                                    DirectionNS::Direction dirs[] = {
                                        DirectionNS::right, DirectionNS::down,
                                        DirectionNS::left, DirectionNS::up
                                    };
                                    if (aimIsShooting) {
                                        player.shoot(dirs[idx]);
                                        aimIsShooting = false;
                                    } else {
                                        player.executeThrow(aimItemNum, dirs[idx]);
                                    }
                                    mode = Mode::Normal;
                                    break;
                                }
                                case 27: case '\t': // ESC or Tab cancels
                                    mode = Mode::Normal;
                                    break;
                            }
                        }
                        ++aimTick;
                        break;
                    }
                    }

                    msSinceMove += FRAME_MS;
                    level.npcFrame(FRAME_MS);
                    level.cleanDead();

                    ++npcTick;
                    if (npcTick >= NPC_INTERVAL) {
                        npcTick = 0;
                        level.buildingTurn();
                        level.spreadNews();
                        level.updateVehicles();
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
                                printer->showMessage("Kansa tajuaa mitä tapahtui... PUTIN ON KUOLLUT! Juhla alkaa!", &level, false);
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

                    if (mode == Mode::Inventory) {
                        printer->printInventoryOverlay(&player.inventory, invSelection);
                    } else if (mode == Mode::Aim) {
                        printer->printAimLine(&level, player.coord.x, player.coord.y, aimAngle);
                    }

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

                if (player.inJail) {
                    player.inJail = false;
                    if (gameState.phase == GamePhase::Moscow || gameState.phase == GamePhase::ReturnMoscow) {
                        printer->setMessage("Poliisit vievät sinut oikeuteen...\nTuomari: 'Rintamalle!'");
                        printer->print(&level);
                        common::sleep(2000);
                        gameState.sendToWar();
                        nextLevel = true;
                    } else {
                        printer->setMessage("Putkassa istuskellessasi huomaat parin tunnin päästä miten\nPääsi alkaa selvetä... Game over!");
                        printer->print(&level);
                        quit = true;
                    }
                    break;
                } else if (player.stats.health < 1) {
                    quit = true;
                    printer->setMessage("Terveytesi pettää... kemahdat tantereeseen pitkäksesi .........");
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

                common::sleep(FRAME_MS);
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
                            printer->showMessage("Kansa tajuaa mitä tapahtui... PUTIN ON KUOLLUT! Juhla alkaa!", &level, false);
                        }
                    }

                    if (turn) {
                        player.updateTurn();
                    }
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

                if (player.inJail) {
                    player.inJail = false;
                    if (gameState.phase == GamePhase::Moscow || gameState.phase == GamePhase::ReturnMoscow) {
                        printer->setMessage("Poliisit vievät sinut oikeuteen...\nTuomari: 'Rintamalle!'");
                        printer->print(&level);
                        InputNS::Input::waitKey();
                        gameState.sendToWar();
                        nextLevel = true;
                    } else {
                        printer->setMessage("Putkassa... Game over!");
                        printer->print(&level);
                        quit = true;
                    }
                    break;
                } else if (player.stats.health < 1) {
                    quit = true;
                    printer->setMessage("Terveytesi pettää... kemahdat tantereeseen pitkäksesi .........");
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
        printer->printChar("P\xD0\x9F""LTSARI 2 - \xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80 p\xC3\xA4\xC3\xA4ttyi!\n\n");
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
        printer->printChar("Ja sin\xC3\xA4 saavutit sen. Putinin viinakellarissa.\n");
        printer->printChar("Loputtomasti vodkaa ja aikaa. Ven\xC3\xA4l\xC3\xA4inen unelma.\n\n");
        common::sleep(2000);
    }

    if (!skipWaitKey) InputNS::Input::waitKey();
    printer->endGame();
    printer->printScore(&player.scoreBoard);
    InputNS::Input::waitKey();
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
        mainloop(printer, stage, losEnabled, animsEnabled, pathFinding);
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
