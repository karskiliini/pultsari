
#include "player.hpp"
#include "input.hpp"
#include <iostream>

namespace PlayerNS {

Player::Player() : Person(PersonType::pelaaja)
{
}

bool Player::drink()
{
    if ((inventory.kalja > 0) && (inventory.lonkka > 0))
    {
        std::cout << "Kumpaakos horaat, kaljaa vai lonkkaa (1,2)?" << std::endl;
        auto type = InputNS::Input::getDrink();
        // drink
        switch(type)
        {
            case InputNS::kalja:
                --inventory.kalja;
                std::cout << "Glub glub glub glub ..... Burb !!!" << std::endl;
                promillet += 2;
                break;
            case InputNS::lonkka:
                --inventory.lonkka;
                std::cout << "Glub glub glub glub ..... Burb !!!" << std::endl;
                promillet += 4;
                break;
            default:
                std::cout << "Päätit olla juomatta mitään." << std::endl;
                return false;
        }
    }
    else if (inventory.lonkka > 0) {
        std::cout << "Glub glub .. gulb gulu... ooorbbbs.\n" << std::endl;
        --inventory.lonkka;
        promillet += 4;
    }
    else if (inventory.kalja > 0) {
        std::cout << "<Tsuuuhss> .. gluuuub gluub glub ... ooorrroyyh hh hh\n" << std::endl;
        --inventory.kalja;
        promillet += 3;
    } else {
        std::cout << "Ei oo ehtaa tavaraa.\n";
        return false;
    }

    return true;
}

bool Player::move(DirectionNS::Direction d)
{
    switch(d)
    {
        case DirectionNS::Direction::down:
            ++y;
            break;
        case DirectionNS::Direction::up:
            --y;
            break;
        case DirectionNS::Direction::right:
            ++x;
            break;
        case DirectionNS::Direction::left:
            --x;
            break;

    }
    std::cout << "player: x: " << x << "y: " << y << std::endl;
    return true;
}

}