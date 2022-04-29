
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
#include <iostream>
#include <exception>

using std::cout;
using std::endl;
using common::random;

namespace common {
    bool animsEnabled;
}

static bool handleInput(PlayerNS::Player& player, Level* level, Printer& printer)
{
    bool ret;
    InputNS::InputType input;

    input = InputNS::Input::getInput();

    if (player.stats.promilles > 23 && common::random(10)) {
        if (input != InputNS::quit)
            input = InputNS::stagger;
    } else if (player.stats.promilles < 4 && common::random(20)) {
        printer.showMessage("Sua janottaa !!!", level);
    } else if (player.stats.promilles > 28 && common::random(5)) {
        printer.showMessage("Oksennat !!", level);
        player.damage(1);

        // quit will override even throwing up
        if (input != InputNS::quit)
            input = InputNS::throwup;
    }

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
                int dir = rand() % 4;
                switch(dir)
                {
                    default:
                    case 0:
                        d = DirectionNS::up;
                        break;
                    case 1:
                        d = DirectionNS::right;
                        break;
                    case 2:
                        d = DirectionNS::down;
                        break;
                    case 3:
                        d = DirectionNS::left;
                        break;
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

static void populateItems(Level& level)
{
    level.addBonas();
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

static void populateBuildings(Level& level)
{
    Building* b = nullptr;

    if (level.stage > 4) {
        if (random(50)) {
            b = new Divari();
        } else {
            b = new Isku();
        }
    }
    level.addBuilding(b);
    b = nullptr;

    if (level.stage % 2) {
        b = new Alko();
    } else {
        b = new KRauta();
    }
    level.addBuilding(b);
    b = nullptr;

    if ((level.stage > 6) && random(50))
    {
        b = new Vankila();
        level.addBuilding(b);
        b = new PoliisiAsema();
        level.addBuilding(b);
    }
    b = nullptr;

    if (level.stage > 8)
    {
        b = new Asema();
        level.addBuilding(b);

        b = new Seina();
        level.addBuilding(b);
    }
    b = nullptr;
}

static void populatePersons(Level& level)
{
    // at least one granny / level
    {
        bool once = true;
        if (once) {
            once = random(10);

            Coord freePos { 0, 0 };
            while ((freePos.x < 14) || (freePos.y < 5)) {
                freePos = level.freePosition();
            }

            level.createPerson(level.freePosition(), mummo);
        }
    }

    // at least one cop / level
    for (int i = 0; i < rand() % 9 + 1; ++i)
    {
        bool create = true;
        if (create) {
            create = random(60);
            level.createPerson(level.freePosition(), poliisi);
        }
    }

    // varas
    {
        if ((level.stage > 2) && random(60)) {
            Coord freePos { 0, 0 };
            while ((freePos.x < 14) || (freePos.y < 6)) {
                freePos = level.freePosition();
            }

            level.createPerson(freePos, varas);
        }
    }

    // ykä
    {
        if (random(50)) {
            Coord pos { common::PLAYER_START_X + 2, common::PLAYER_START_Y };
            if (!level.hit(pos))
            {
                level.createPerson(pos, yka);
            }
        }
    }

}

static bool checkExit(const Level& level)
{
    bool ok = true;
    for (const auto& i : level.items) {
        if (i->typeToChar() == "^")
        {
            ok = false;
            break;
        }
    }
    return ok;
}


void mainloop(Printer* printer, uint32_t stage, bool losEnabled, bool animsEnabled, bool pathFinding)
{
#ifdef ANIMATIONS_ENABLED
    common::animsEnabled = animsEnabled;
#endif
    bool welcome = true;

    PlayerNS::Player player;
    player.printer = printer;

    bool quit = false;

    while (!quit) {
        player.resetPosition();
        Level level(stage);
        level.pathFinding = pathFinding;

        VisionNS::Mask mask(&level);
#ifdef LOS_ENABLED
        if (losEnabled)
            mask.enable();
#endif
        printer->mask = &mask;

        level.printer = printer;

        printer->player = &player;

        {
            NextLevel* next = new NextLevel(Coord(79,27), &level);
            level.addItem(next);
        }

        level.addPerson(&player);
        populateBuildings(level);
        populatePersons(level);
        populateItems(level);

        if (welcome) {
            welcome = false;
            printer->showMessage("Tervetuloa Pultsariin!", &level, false);
        }

        mask.init();
        printer->print(&level);

        bool nextLevel = false;
        while(!nextLevel && !quit) {
            printer->removeMessage();

            try {
                bool turn = handleInput(player, &level, *printer);
                mask.init();
                level.cleanDead();
                level.cleanDiscardedItems();

                printer->print(&level);

                level.buildingTurn();
                level.npcTurn();
                level.cleanDead();
                level.cleanDiscardedItems();

                if (turn) {
                    player.updateTurn();
                }
            } catch(...) {
                quit = true;
                break;
            }

            if (player.inJail) {
                printer->setMessage("Putkassa istuskellessasi huomaat parin tunnin päästä miten\nPääsi alkaa selvetä, ja tunnet miten kankkunen tulee. Game over !!!!!");
                printer->print(&level);
                quit = true;
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
            if (checkExit(level))
            {
                printer->showMessage("Seuraavaan kenttaan...", &level, false);
                printer->removeMessage();

                InputNS::Input::waitKey();

                // next level
                stage++;
                nextLevel = true;
            }
        }
        printer->printScore(&player.scoreBoard);
    }
}

int init(int argc, char *argv[], Printer* printer)
{
    srand(time(NULL));
    printer->show_console_cursor(false);

    uint32_t stage = 1;
    bool animsEnabled = false;
    bool losEnabled = false;
    bool pathFinding = false;

    for (int i = 0; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--los") {
            // enable line of sight
            losEnabled = true;
        } else if (a == "--anims") {
            // enable animations
            animsEnabled = true;
        } else if ((a == "-h") || (a == "--help")) {
            printer->printHelp();
            InputNS::Input::waitKey();
            return 0;
        } else if (a == "--path") {
            // enable advanced path finding for npc characters
            pathFinding = true;
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

    IntroNS::Intro i(printer);
    i.show();

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
