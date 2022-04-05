
#include "pultsari.hpp"
#include "intro.hpp"
#include "input.hpp"
#include "level.hpp"
#include "player.hpp"
#include <iostream>
#include <exception>

void mainloop()
{
    Level level;
    PlayerNS::Player player;

    while(true) {

        auto input = InputNS::Input::getInput();
        switch(input)
        {
            case InputNS::up:
                std::cout << "up" << std::endl;
                player.move(DirectionNS::up);
                break;
            case InputNS::down:
                std::cout << "down" << std::endl;
                player.move(DirectionNS::down);
                break;
            case InputNS::left:
                std::cout << "left" << std::endl;
                player.move(DirectionNS::left);
                break;
            case InputNS::right:
                std::cout << "right" << std::endl;
                player.move(DirectionNS::right);
                break;
            case InputNS::drinking:
                std::cout << "drinking" << std::endl;
                if (!player.drink()) {
                    // didn't drink, continue turn
                    std::cout << "drinking failed, continue" << std::endl;
                    continue;
                }
                break;
            default:
            case InputNS::quit:
                throw std::out_of_range("quit!");
        }
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