
#include "player.hpp"
#include "input.hpp"
#include <iostream>

namespace PlayerNS {

Player::Player() : Person(PersonType::pelaaja)
{
    x = 10;
    y = 10;
}

bool Player::drink(Printer& printer)
{
    if ((inventory.kalja > 0) && (inventory.lonkka > 0))
    {
        printer.showMessage("Kumpaakos horaat, kaljaa vai lonkkaa (1,2)?");
        auto type = InputNS::Input::getDrink();
        switch(type)
        {
            case InputNS::kalja:
                --inventory.kalja;
                printer.showMessage("Glub glub glub glub ..... Burb !!!");
                promillet += 2;
                break;
            case InputNS::lonkka:
                --inventory.lonkka;
                printer.showMessage("Glub glub glub glub ..... Burb !!!");
                promillet += 4;
                break;
            default:
                printer.showMessage("Päätit olla juomatta mitään.");
                return false;
        }
    }
    else if (inventory.lonkka > 0) {
        printer.showMessage("Glub glub .. gulb gulu... ooorbbbs.\n");
        --inventory.lonkka;
        promillet += 4;
    }
    else if (inventory.kalja > 0) {
        printer.showMessage("<Tsuuuhss> .. gluuuub gluub glub ... ooorrroyyh hh hh\n");
        --inventory.kalja;
        promillet += 3;
    } else {
        printer.showMessage("Ei oo ehtaa tavaraa.");
        return false;
    }

    return true;
}

bool Player::move(DirectionNS::Direction d, Level& level, Printer& printer)
{
    bool ret = true;
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

    if ((y >= level.sizey) || (x >= level.sizex)) {
        printer.showMessage("Ei karata pelialueelta !!");
        ret = false;
    }

    x = (x == level.sizex) ? level.sizex-1 : x;
    x = (x > level.sizex) ? 0 : x;
    y = (y == level.sizey) ? level.sizey-1 : y;
    y = (y > level.sizey) ? 0 : y;

    return ret;
}

}