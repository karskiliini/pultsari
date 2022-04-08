
#include "pultsari.hpp"
#include "intro.hpp"
#include "input.hpp"
#include "building.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "item.hpp"
#include <iostream>
#include <exception>

using std::cout;
using std::endl;

void show_console_cursor(const bool show) {
#if defined(_WIN32)
    static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(handle, &cci);
    cci.bVisible = show; // show/hide cursor
    SetConsoleCursorInfo(handle, &cci);
#elif defined(__linux__)
    std::cout << (show ? "\033[?25h" : "\033[?25l"); // show/hide cursor
#endif // Windows/Linux
}

static bool handleInput(PlayerNS::Player& player, Level& level, Printer& printer)
{
    bool ret;
    auto input = InputNS::Input::getInput();
    switch(input)
    {
        case InputNS::up:
            ret = player.move(DirectionNS::up, level, printer);
            break;
        case InputNS::down:
            ret = player.move(DirectionNS::down, level, printer);
            break;
        case InputNS::left:
            ret = player.move(DirectionNS::left, level, printer);
            break;
        case InputNS::right:
            ret = player.move(DirectionNS::right, level, printer);
            break;
        case InputNS::drinking:
            ret = player.drink(printer, level);
            break;
        case InputNS::eating:
            ret = player.eat(printer, level);
            break;
        default:
        case InputNS::quit:
            throw std::out_of_range("quit!");
    }

    return ret;
}

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

static void populateItems(Level& level)
{
    level.addBonas();
    if (random(50))  level.addItem(EKalja);
    if (random(250)) level.addItem(EKetjut);
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
    // divari tai isku
    if (level.stage > 4) {
        if (random(50)) {
            b = new Divari();
        } else {
            b = new Isku();
        }
    }
    level.addBuilding(b);

    if (level.stage % 2) {
        b = new Alko();
    } else {
        b = new KRauta();
    }
    level.addBuilding(b);

    if ((level.stage > 6) && random(50)) {
        b = new Vankila();
        level.addBuilding(b);
        b = new PoliisiAsema();
        level.addBuilding(b);
    }

    if (level.stage > 8)
    {
        // asema
        b = new Asema();
        level.addBuilding(b);

        b = new Seina();
        level.addBuilding(b);
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

void mainloop()
{
    bool welcome = true;

    Printer printer;
    uint32_t stage = 9;
    PlayerNS::Player player;

    bool quit = false;

    while (!quit) {
        player.resetPosition();
        Level level(stage);

        //debug
        printer.player = &player;

        {
            NextLevel* next = new NextLevel(79,27, &level);
            level.addItem(next);
        }

        level.addPerson(player);

        populateBuildings(level);
        populateItems(level);

        if (welcome) {
            welcome = false;
            printer.showMessage("Tervetuloa Pultsariin!", level);
        }

        printer.print(level);

        bool nextLevel = false;
        while(!nextLevel && !quit) {
            printer.removeMessage();

            try {
                bool turn = handleInput(player, level, printer);
                if (turn) {
                    ++player.turn;
                }
            } catch(...) {
                quit = true;
                break;
            }

            if (player.inJail) {
                printer.setMessage("Putkassa istuskellessasi huomaat parin tunnin paasta miten          <LISää>\nPaasi alkaa selveta, ja tunnet miten kankkunen tulee. Game over !!!!!");
                printer.print(level);
                quit = true;
                break;
            } else if (player.health < 1) {
                quit = true;
                printer.setMessage("Terveytesi pettaa... kemahdat tantereeseen pitkaksesi .........");
                printer.print(level);
                break;
            }

            printer.print(level);
            if (checkExit(level))
            {
                printer.showMessage("Seuraavaan kenttaan...", level, false);
                printer.removeMessage();

                InputNS::Input::waitKey();

                // next level
                stage++;
                nextLevel = true;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    show_console_cursor(false);


    IntroNS::Intro i;
    i.show();

    try {
        mainloop();
    }
    catch(const std::out_of_range& e)
    {
        InputNS::Input::noraw();
        std::cout << e.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "unknown exception caught, exit!" << std::endl;
    }

    show_console_cursor(true);

    return 0;
}