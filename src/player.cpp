
#include "player.hpp"
#include "input.hpp"
#include <iostream>
#include <string>

namespace PlayerNS {

using std::string;
using InputNS::Input;

Player::Player() : Person(PersonType::pelaaja)
{
}

void Player::resetPosition()
{
    x = 10;
    y = 10;
}

bool Player::drink(Printer& printer, Level& level)
{
    if ((inventory.kalja > 0) && (inventory.lonkka > 0))
    {
        printer.showMessage("Kumpaakos horaat, kaljaa vai lonkkaa (1,2)?", level);
        auto type = InputNS::Input::getDrink();
        switch(type)
        {
            case InputNS::kalja:
                --inventory.kalja;
                printer.showMessage("Glub glub glub glub ..... Burb !!!", level);
                promilles += 2;
                break;
            case InputNS::lonkka:
                --inventory.lonkka;
                printer.showMessage("Glub glub glub glub ..... Burb !!!", level);
                promilles += 4;
                break;
            default:
                printer.showMessage("Päätit olla juomatta mitään.", level);
                return false;
        }
    }
    else if (inventory.lonkka > 0) {
        printer.showMessage("Glub glub .. gulb gulu... ooorbbbs.\n", level);
        --inventory.lonkka;
        promilles += 4;
    }
    else if (inventory.kalja > 0) {
        printer.showMessage("<Tsuuuhss> .. gluuuub gluub glub ... ooorrroyyh hh hh\n", level);
        --inventory.kalja;
        promilles += 3;
    } else {
        printer.showMessage("Ei oo ehtaa tavaraa.", level);
        return false;
    }

    return true;
}

bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

bool Player::eat(Printer& printer, Level& level)
{
    if ((inventory.kalat == 0) && (inventory.omppo == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        printer.showMessage("Ei syötävää.", level);
    } else if ((inventory.kalat == 0) && (inventory.omppo == 0) && (inventory.lenkki == 0)) {
        --inventory.bansku;
        if (random(50)) {
            printer.showMessage("Ei vaikutusta.", level);
        } else {
            health += 1;
            printer.showMessage("Bansku teki teraa !!!", level);
        }
    } else if ((inventory.kalat == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        --inventory.omppo;
        if (random(50)) {
            health -= 1;
            printer.showMessage("Omppo oli pilaantunut,oksennat.", level);
        } else {
            health += 2;
            printer.showMessage("Omena oli herkullinen !!!", level);
        }
    } else if ((inventory.omppo == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        --inventory.kalat;
        if (random(70)) {
            health -= 2;
            printer.showMessage("Syomasi kala sisalsi matoja...", level);
        } else {
            health += 4;
            printer.showMessage("Kala antaa sinulle uutta puhtia !!!", level);
        }
    } else if ((inventory.kalat == 0) && (inventory.omppo == 0) && (inventory.bansku == 0)) {
        health += 1;
        --inventory.lenkki;
        printer.showMessage("Oispa edes kossua kaveriksi.", level);
    } else {
        printer.showMessage("Mitas...1=makkara 2=kala 3=omppo 4=bansku ???", level);
        auto type = InputNS::Input::getFood();

        switch(type) {
            case InputNS::foodlenkki:
                if (inventory.lenkki == 0) {
                    printer.showMessage("Ei ole niitä.", level);
                } else {
                    --inventory.lenkki;
                    printer.showMessage("Mamamamakkaraa.", level);
                }
                break;
            case InputNS::foodkala:
                if (inventory.kalat == 0) {
                    printer.showMessage("Kalat loppu.", level);
                } else {
                    --inventory.kalat;
                    if (random(70)) {
                        health -= 2;
                        printer.showMessage("Syomasi kala sisalsi matoja...", level);
                    } else {
                        health += 4;
                        printer.showMessage("Kaloreja kalasta.", level);
                    }
                }
                break;
            case InputNS::foodomppo:
                if (inventory.omppo == 0) {
                    printer.showMessage("EE OO OMMPPOJA.", level);
                } else {
                    --inventory.omppo;
                    if (random(50)) {
                        health -= 1;
                        printer.showMessage("Joku oli kaytellyt keski keppiaan omppoosi.", level);
                    } else {
                        health += 2;
                        printer.showMessage("orb.", level);
                    }
                }
                break;
            case InputNS::foodbansku:
                if (inventory.bansku == 0) {
                    printer.showMessage("Bailu ilman banskuu?", level);
                } else {
                    --inventory.bansku;
                    if (random(50)) {
                        printer.showMessage("Bansku ei vaikuttanut.", level);
                    } else {
                        health += 1;
                        printer.showMessage("Babababananiii", level);
                    }
                }
                break;
            default:
                printer.showMessage("Päätit olla syömättä mitään.", level);
                return false;

        }
    }
    return true;
}

Building* checkDoor(Level& level, uint32_t x, uint32_t y)
{
    for (auto& b : level.buildings) {
        if (b->hitDoor(x, y)) {
            return b;
        }
    }
    return nullptr;
}

Building* checkWalls(Level& level, uint32_t x, uint32_t y)
{
    for (auto& b : level.buildings) {
        if (b->hitWall(x, y)) {
            return b;
        }
    }
    return nullptr;
}

Item* checkItems(Level& level, uint32_t x, uint32_t y)
{
    for (auto& i : level.items) {
        if ((i->x == x) && (i->y == y)) {
            return i;
        }
    }
    return nullptr;
}

bool Player::move(DirectionNS::Direction d, Level& level, Printer& printer)
{
    // spend turn
    bool ret = true;
    // prevent movement
    bool blocked = false;


    uint32_t checkx = x;
    uint32_t checky = y;

    switch(d)
    {
        case DirectionNS::Direction::down:
            ++checky;
            break;
        case DirectionNS::Direction::up:
            --checky;
            break;
        case DirectionNS::Direction::right:
            ++checkx;
            break;
        case DirectionNS::Direction::left:
            --checkx;
            break;
    }

    string msg;
    Building* b = checkDoor(level, checkx, checky);
    if (b) {
        // step into building
        blocked = true;

        bool entered = b->getEnterMsg(this, msg);
        printer.showMessage(msg, level);
        if (entered)
        {
            b->getInteractMsg(this, msg);
            printer.showMessage(msg, level);

            b->interact(this, msg);
            printer.showMessage(msg, level, false);
        }
    }

    if (!b) {
        b = checkWalls(level, checkx, checky);
        if (b)
        {
            msg = b->getWalkMsg();
            blocked = true;
        }
    }

    Item* i = nullptr;
    if (!b) {
        i = checkItems(level, checkx, checky);
        if (i) {
            msg = i->getMsg();
            auto consume = i->interact(this);
            if (consume) {
                level.removeItem(i);
            }
        }
    }

    if (!blocked) {
        x = checkx;
        y = checky;
    }

    printer.showMessage(msg, level);

    if ((y >= level.sizey) || (x >= level.sizex)) {
        printer.showMessage("Ei karata pelialueelta !!", level);
        ret = false;
    }

    x = (x == level.sizex) ? level.sizex-1 : x;
    x = (x > level.sizex) ? 0 : x;
    y = (y == level.sizey) ? level.sizey-1 : y;
    y = (y > level.sizey) ? 0 : y;

    return ret;
}

}