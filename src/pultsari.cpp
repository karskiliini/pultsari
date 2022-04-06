
#include "pultsari.hpp"
#include "intro.hpp"
#include "input.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include <iostream>
#include <exception>

bool handleInput(PlayerNS::Player& player, Level& level, Printer& printer)
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
            ret = player.drink(printer);
            break;
        default:
        case InputNS::quit:
            throw std::out_of_range("quit!");
    }

    return ret;
}

void mainloop()
{
    Printer printer;
    Level level;
    PlayerNS::Player player;
    bool added = level.addPerson(player);

    while(1) {
        printer.removeMessage();

        bool turn = handleInput(player, level, printer);

        if (turn) {
            // next turn
            ++level.turn;
        }
        printer.print(level);
    }
}


int main(int argc, char *argv[])
{

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

    return 0;
}