
#include "player.hpp"
#include "input.hpp"
#include "building.hpp"
#include <iostream>
#include <string>
#include <exception>

namespace PlayerNS {

using std::string;
using InputNS::Input;
using InputNS::InputType;
using std::cout;
using std::endl;

Player::Player() : Person(PersonType::pelaaja, {0, 0})
{
    health = 20;
}

void Player::damage(uint32_t damage)  {
    if (damage > health) {
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
            damage(1);
            printer.showMessage("Omppo oli pilaantunut,oksennat.", level);
        } else {
            health += 2;
            printer.showMessage("Omena oli herkullinen !!!", level);
        }
    } else if ((inventory.omppo == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        --inventory.kalat;
        if (random(70)) {
            damage(2);
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
                        damage(2);
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
                        damage(1);
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

static InputType validateDirection(InputType in) {
    switch(in) {
        case InputNS::up:
        case InputNS::right:
        case InputNS::down:
        case InputNS::left:
            return in;
        default:
            return InputNS::inputpending;
    }
}

static DirectionNS::Direction getDirection(Printer& printer, Level* level)
{
    printer.showMessage("Anna suunta: ", *level, false);

    InputType direction = InputNS::inputpending;
    while(direction == InputNS::inputpending) {
        direction = Input::getInput();
        direction = validateDirection(direction);
    }

    switch(direction) {
        default:
        case InputNS::up:
            return DirectionNS::up;
        case InputNS::right:
            return DirectionNS::right;
        case InputNS::down:
            return DirectionNS::down;
        case InputNS::left:
            return DirectionNS::left;
    }
}

bool Player::decrementInventory(uint32_t index, Printer& printer)
{
    switch(index) {
        case 1:
            if (inventory.kalja == 0) {
                printer.showMessage("Ei ole kaljaa...", *level, false);
                return false;
            } else {
                --inventory.kalja;
            }
            break;
        case 2:
            if (inventory.lonkka == 0) {
                printer.showMessage("Ei ole giniä...", *level, false);
                return false;
            } else {
                --inventory.lonkka;
            }
            break;
        case 3:
            if (inventory.lenkki == 0) {
                printer.showMessage("Ei ole kiekuralenkkiä...", *level, false);
                return false;
            } else {
                --inventory.lenkki;
            }
            break;
        case 4:
            if (inventory.ketjut == 0) {
                printer.showMessage("Ei ole kettinkiä...", *level, false);
                return false;
            } else {
                --inventory.ketjut;
            }
            break;
        case 5:
            if (inventory.veitset == 0) {
                printer.showMessage("Ei ole puukkoja...", *level, false);
                return false;
            } else {
                --inventory.veitset;
            }
            break;
        case 6:
            if (inventory.kivet == 0) {
                printer.showMessage("Ei ole kiviä...", *level, false);
                return false;
            } else {
                --inventory.kivet;
            }
            break;
        case 7:
            if (inventory.pamput == 0) {
                printer.showMessage("Ei ole pamppuja...", *level, false);
                return false;
            } else {
                --inventory.pamput;
            }
            break;
        case 8:
            if (inventory.bootsit == 0) {
                printer.showMessage("Ei ole camel bootseja...", *level, false);
                return false;
            } else {
                --inventory.bootsit;
            }
            break;
        case 9:
            if (inventory.kalat == 0) {
                printer.showMessage("Ei ole turskaa...", *level, false);
                return false;
            } else {
                --inventory.kalat;
            }
            break;
        case 10:
            if (inventory.omppo == 0) {
                printer.showMessage("Ei ole omenia...", *level, false);
                return false;
            } else {
                --inventory.omppo;
            }
            break;
        case 11:
            if (inventory.veitset == 0) {
                printer.showMessage("Ei ole banskuja...", *level, false);
                return false;
            } else {
                --inventory.veitset;
            }
            break;
    }
    return true;
}

static Coordinate<int> dirToCoord(DirectionNS::Direction d) {
    switch(d)
    {
        case DirectionNS::up:
            return { 0, -1 };
        case DirectionNS::right:
            return { 1, 0 };
        case DirectionNS::down:
            return { 0, 1 };
        case DirectionNS::left:
            return { -1, 0 };
        default:
            throw std::out_of_range("invalid direction!");
    }
}

bool Player::throwItem(Printer& printer)
{
    printer.showMessage("ANNA NUMERO ..", *level, false);
    auto number = InputNS::Input::getThrow();
    if (!decrementInventory(number, printer))
    {
        return false;
    }
    auto dir = getDirection(printer, level);


    {
        Coordinate<int> c { coord.x, coord.y };
        auto d = dirToCoord(dir);
        printer.showMessage("raycast " + std::to_string(d.x) + " " + std::to_string(d.y), *level);

        Person* hit = level->raycast(c, dirToCoord(dir));
        if (!hit)
        {
            Building* bhit = level->raycastBuilding(c, dirToCoord(dir));
        }
    }

    return true;
}

static Building* checkDoor(Level& level, const Coord& c)
{
    for (auto& b : level.buildings) {
        if (b->hitDoor(c)) {
            return b;
        }
    }
    return nullptr;
}

static Building* checkWalls(Level& level, const Coord& c)
{
    for (auto& b : level.buildings) {
        if (b->hitWall(c)) {
            return b;
        }
    }
    return nullptr;
}

static Item* checkItems(Level& level, const Coord& c)
{
    for (auto& i : level.items) {
        if (i->coord == c) {
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

    Coord check { coord.x, coord.y };

    switch(d)
    {
        case DirectionNS::down:
            check += Coord { 0, 1 };
            break;
        case DirectionNS::up:
            check -= Coord { 0, 1 };
            break;
        case DirectionNS::right:
            check += Coord { 1, 0 };
            break;
        case DirectionNS::left:
            check -= Coord { 1, 0 };
            break;
        case DirectionNS::none:
            return false;
    }

    string msg;
    Building* b = checkDoor(level, check);
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
            b = checkWalls(level, check);
            if (b)
            {
                msg = b->getWalkMsg();
                blocked = true;
            }
        }
    }

    if (!blocked) {
        Person* p = level.checkPerson(check);
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
            i = checkItems(level, check);
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
        coord = check;
    }

    if (common::checkBounds(coord)) {
        printer.showMessage("Ei karata pelialueelta !!", level, true);
        ret = false;
    }

    return ret;
}

}