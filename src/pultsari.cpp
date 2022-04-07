
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
            ret = player.drink(printer, level);
            break;
        default:
        case InputNS::quit:
            throw std::out_of_range("quit!");
    }

    return ret;
}

bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

void initLevel(Level& level)
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
    if (random(80))  level.addItem(ELonkka);
    if (random(90))  level.addItem(ERaha);
    if (random(19))  level.addItem(EPaska);
}

void mainloop()
{
    Printer printer;

    Level level(3);
    PlayerNS::Player player;

    //debug
    printer.player = &player;

    Alko alko;
    Divari divari;

    {
        Paska* p = new Paska(30,10);
        level.addItem(p);
    }

    level.addBuilding(alko);
    level.addBuilding(divari);
    level.addPerson(player);

    initLevel(level);

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