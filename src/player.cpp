
#include "player.hpp"
#include "input.hpp"
#include "building.hpp"
#include <iostream>
#include <string>

namespace PlayerNS {

using std::string;
using InputNS::Input;
using std::cout;
using std::endl;

Player::Player() : Person(PersonType::pelaaja, {0, 0})
{
}

void Player::damage(uint32_t damage)  {
    if (damage > health)
    {
        health = 0;
    } else {
        health -= damage;
    }
}

bool Player::interact(std::string& message, Person* source)
{
    if (health == 0) return true;
    return false;
}

void Player::resetPosition()
{
    coord = {10, 10};
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
                printer.showMessage("Glub glub glub glub ..... Burb !!!", level, false);
                promilles += 2;
                break;
            case InputNS::lonkka:
                --inventory.lonkka;
                printer.showMessage("Glub glub glub glub ..... Burb !!!", level, false);
                promilles += 4;
                break;
            default:
                printer.showMessage("Päätit olla juomatta mitään.", level, false);
                return false;
        }
    }
    else if (inventory.lonkka > 0) {
        printer.showMessage("Glub glub .. gulb gulu... ooorbbbs.", level);
        --inventory.lonkka;
        promilles += 4;
    }
    else if (inventory.kalja > 0) {
        printer.showMessage("<Tsuuuhss> .. gluuuub gluub glub ... ooorrroyyh hh hh", level);
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
                    printer.showMessage("Ei ole niitä.", level, false);
                } else {
                    --inventory.lenkki;
                    printer.showMessage("Mamamamakkaraa.", level, false);
                }
                break;
            case InputNS::foodkala:
                if (inventory.kalat == 0) {
                    printer.showMessage("Kalat loppu.", level, false);
                } else {
                    --inventory.kalat;
                    if (random(70)) {
                        health -= 2;
                        printer.showMessage("Syomasi kala sisalsi matoja...", level, false);
                    } else {
                        health += 4;
                        printer.showMessage("Kaloreja kalasta.", level, false);
                    }
                }
                break;
            case InputNS::foodomppo:
                if (inventory.omppo == 0) {
                    printer.showMessage("EE OO OMMPPOJA.", level, false);
                } else {
                    --inventory.omppo;
                    if (random(50)) {
                        health -= 1;
                        printer.showMessage("Joku oli kaytellyt keski keppiaan omppoosi.", level, false);
                    } else {
                        health += 2;
                        printer.showMessage("orb.", level, false);
                    }
                }
                break;
            case InputNS::foodbansku:
                if (inventory.bansku == 0) {
                    printer.showMessage("Bailu ilman banskuu?", level, false);
                } else {
                    --inventory.bansku;
                    if (random(50)) {
                        printer.showMessage("Bansku ei vaikuttanut.", level, false);
                    } else {
                        health += 1;
                        printer.showMessage("Babababananiii", level, false);
                    }
                }
                break;
            default:
                printer.showMessage("Päätit olla syömättä mitään.", level, false);
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

    uint32_t checkx = coord.x;
    uint32_t checky = coord.y;

    switch(d)
    {
        case DirectionNS::down:
            ++checky;
            break;
        case DirectionNS::up:
            --checky;
            break;
        case DirectionNS::right:
            ++checkx;
            break;
        case DirectionNS::left:
            --checkx;
            break;
        case DirectionNS::none:
            return false;
    }

    string msg;
    Building* b = checkDoor(level, checkx, checky);
    if (b) {
        blocked = true;

        // check whether can enter
        bool entered = b->getEnterMsg(this, msg);
        printer.showMessage(msg, level, entered);
        if (entered)
        {
            // step into building
            b->getInteractMsg(this, msg);
            printer.showMessage(msg, level, true);

            b->interact(this, msg);
            printer.showMessage(msg, level, false);
        }
    }

    if (!blocked) {
        if (!b) {
            b = checkWalls(level, checkx, checky);
            if (b)
            {
                msg = b->getWalkMsg();
                blocked = true;
            }
        }
    }

    if (!blocked) {
        Person* p = level.checkPerson(checkx, checky);
        if (p)
        {
            blocked = p->interact(msg, this);
            printer.showMessage(msg, level);

            if (p->health == 0) {
                Item* i = p->dropItem();
                if (i) {
                    level.addItem(i);
                }
            }
        }
    }

    if (!blocked) {
        Item* i = nullptr;
        if (!b) {
            i = checkItems(level, checkx, checky);
            if (i) {
                msg = i->getMsg();
                printer.showMessage(msg, level, false);
                auto consume = i->interact(this);

                if (consume) {
                    level.removeItem(i);
                }
            }
        }
    }

    if (!blocked) {
        coord.x = checkx;
        coord.y = checky;
    }

    if (checkBounds(level.sizex, level.sizey)) {
        printer.showMessage("Ei karata pelialueelta !!", level, true);
        ret = false;
    }

    return ret;
}

}