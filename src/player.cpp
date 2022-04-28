
#include "player.hpp"
#include "level.hpp"
#include "common.hpp"
#include "item.hpp"
#include "input.hpp"
#include "building.hpp"
#include "animation.hpp"
#include <iostream>
#include <string>
#include <exception>

namespace PlayerNS {

using common::random;
using std::string;
using InputNS::Input;
using InputNS::InputType;
using std::cout;
using std::endl;


Player::Player() : Person(PersonType::pelaaja, {0, 0})
{
    stats.health = INITIAL_HP;
    stats.promilles = 2;
    stats.money = 20;
    stats.turn = 1;
}

void Player::updateMoney(int money)
{
    stats.money += money;

#ifdef ANIMATIONS_ENABLED
    string prefix = "+";
    if (money < 0) {
        prefix = "-";
    }

    string ptext = std::to_string(abs(money));

    string text = prefix + ptext + "$";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a, false);
#endif
}

void Player::updatepromilles(int p)
{
    stats.promilles += p;

#ifdef ANIMATIONS_ENABLED
    string prefix = "+";
    if (p < 0) {
        prefix = "-";
    }

    string ptext = std::to_string(abs(p)/10) + "." + std::to_string(abs(p)%10);

    string text = prefix + ptext + "%.";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a);
#endif
}

void Player::updateTurn()
{
    ++stats.turn;

// original pultsari has this feature, but it makes the game too easy
#if 0
    if (stats.turn % 50 == 0)
    {
        powerup(1);
    }
#endif
    if (stats.turn % 260 == 0) {
        updatepromilles(-1);
    }
}

bool Player::interact(std::string& msg, Person* source)
{
    if (stats.health == 0) return true;

    if (source->type == yka) {
        msg = "Ykä pummaa yhtä kaljaa, annatko? (k/e)";
        printer->showMessage(msg, level);
        char c = InputNS::Input::inputYka();
        switch(c) {
            default:
            case 'k':
            case 'K':
                msg = "Yka kiitteleepi, ja istuu alas kittaamaan kaljaasi.";
                --inventory.kalja;
                return true;
                break;
            case 'e':
            case 'E':
                msg = "Yka taputteleepi palleaasi.";
                damage(1);
                return false;
                break;
        }
    }

    return false;
}

bool Player::interactThrow(Item* item, Person* source, std::string& msg)
{
    if ((item->type == EKivi) && (source->type == vanki))
    {
        msg = "Vankikarkuri nakkasi sinua kivellä!";
    } else {
        msg = "Sinua viskattiin jollain, ouch!";
    }
    damage(1);
    return true;
}

void Player::resetPosition()
{
    coord = {common::PLAYER_START_X, common::PLAYER_START_Y};
}

bool Player::drink(Level* level)
{
    if ((inventory.kalja > 0) && (inventory.lonkka > 0))
    {
        printer->showMessage("Kumpaakos horaat, kaljaa vai lonkkaa (1,2)?", level);
        auto type = InputNS::Input::getDrink();
        switch(type)
        {
            case InputNS::kalja:
                --inventory.kalja;
                printer->showMessage("Glub glub glub glub ..... Burb !!!", level, false);
                updatepromilles(2);
                break;
            case InputNS::lonkka:
                --inventory.lonkka;
                printer->showMessage("Glub glub glub glub ..... Burb !!!", level, false);
                updatepromilles(4);
                break;
            default:
                printer->showMessage("Päätit olla juomatta mitään.", level, false);
                return false;
        }
    }
    else if (inventory.lonkka > 0) {
        printer->showMessage("Glub glub .. gulb gulu... ooorbbbs.", level);
        --inventory.lonkka;
        updatepromilles(4);
    }
    else if (inventory.kalja > 0) {
        printer->showMessage("<Tsuuuhss> .. gluuuub gluub glub ... ooorrroyyh hh hh", level);
        --inventory.kalja;
        updatepromilles(3);
    } else {
        printer->showMessage("Ei oo ehtaa tavaraa.", level);
        return false;
    }

    return true;
}

bool Player::eat(Level* level)
{
    if ((inventory.kalat == 0) && (inventory.omppo == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        printer->showMessage("Ei syötävää.", level);
    } else if ((inventory.kalat == 0) && (inventory.omppo == 0) && (inventory.lenkki == 0)) {
        --inventory.bansku;
        if (random(50)) {
            printer->showMessage("Ei vaikutusta.", level);
        } else {
            powerup(1);
            printer->showMessage("Bansku teki teraa !!!", level);
        }
    } else if ((inventory.kalat == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        --inventory.omppo;
        if (random(50)) {
            damage(1);
            printer->showMessage("Omppo oli pilaantunut,oksennat.", level);
        } else {
            powerup(2);
            printer->showMessage("Omena oli herkullinen !!!", level);
        }
    } else if ((inventory.omppo == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        --inventory.kalat;
        if (random(70)) {
            damage(2);
            printer->showMessage("Syomasi kala sisalsi matoja...", level);
        } else {
            powerup(4);
            printer->showMessage("Kala antaa sinulle uutta puhtia !!!", level);
        }
    } else if ((inventory.kalat == 0) && (inventory.omppo == 0) && (inventory.bansku == 0)) {
        powerup(1);
        --inventory.lenkki;
        printer->showMessage("Oispa edes kossua kaveriksi.", level);
    } else {
        printer->showMessage("Mitas...1=makkara 2=kala 3=omppo 4=bansku ???", level);
        auto type = InputNS::Input::getFood();

        switch(type) {
            case InputNS::foodlenkki:
                if (inventory.lenkki == 0) {
                    printer->showMessage("Ei ole niitä.", level, false);
                } else {
                    --inventory.lenkki;
                    printer->showMessage("Mamamamakkaraa.", level, false);
                }
                break;
            case InputNS::foodkala:
                if (inventory.kalat == 0) {
                    printer->showMessage("Kalat loppu.", level, false);
                } else {
                    --inventory.kalat;
                    if (random(70)) {
                        damage(2);
                        printer->showMessage("Syomasi kala sisalsi matoja...", level, false);
                    } else {
                        powerup(4);
                        printer->showMessage("Kaloreja kalasta.", level, false);
                    }
                }
                break;
            case InputNS::foodomppo:
                if (inventory.omppo == 0) {
                    printer->showMessage("EE OO OMMPPOJA.", level, false);
                } else {
                    --inventory.omppo;
                    if (random(50)) {
                        damage(1);
                        printer->showMessage("Joku oli kaytellyt keski keppiaan omppoosi.", level, false);
                    } else {
                        powerup(2);
                        printer->showMessage("orb.", level, false);
                    }
                }
                break;
            case InputNS::foodbansku:
                if (inventory.bansku == 0) {
                    printer->showMessage("Bailu ilman banskuu?", level, false);
                } else {
                    --inventory.bansku;
                    if (random(50)) {
                        printer->showMessage("Bansku ei vaikuttanut.", level, false);
                    } else {
                        powerup(1);
                        printer->showMessage("Babababananiii", level, false);
                    }
                }
                break;
            default:
                printer->showMessage("Päätit olla syömättä mitään.", level, false);
                return false;

        }
    }
    return true;
}

bool Player::puke(DirectionNS::Direction d, Level* level)
{
    Coord check = coord;
    switch(d)
    {
        default:
        case DirectionNS::up:
            --check.y;
            break;
        case DirectionNS::right:
            ++check.x;
            break;
        case DirectionNS::down:
            ++check.y;
            break;
        case DirectionNS::left:
            --check.x;
            break;
    }

    bool ret = false;
    auto p = level->getPerson(check);
    if (p)
    {
        ret = p->interactpuke(this);
    }

    if (!ret) {
        auto puke = Item::createItem(12, check);
        level->addItem(puke);
    }

    return ret;
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

static DirectionNS::Direction getDirection(Level* level, Printer* printer)
{
    printer->showMessage("Anna suunta: ", level, false);

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

bool Player::decrementInventory(uint32_t index)
{
    switch(index) {
        case 1:
            if (inventory.kalja == 0) {
                printer->showMessage("Ei ole kaljaa...", level, false);
                return false;
            } else {
                --inventory.kalja;
            }
            break;
        case 2:
            if (inventory.lonkka == 0) {
                printer->showMessage("Ei ole giniä...", level, false);
                return false;
            } else {
                --inventory.lonkka;
            }
            break;
        case 3:
            if (inventory.lenkki == 0) {
                printer->showMessage("Ei ole kiekuralenkkiä...", level, false);
                return false;
            } else {
                --inventory.lenkki;
            }
            break;
        case 4:
            if (inventory.ketjut == 0) {
                printer->showMessage("Ei ole kettinkiä...", level, false);
                return false;
            } else {
                --inventory.ketjut;
            }
            break;
        case 5:
            if (inventory.veitset == 0) {
                printer->showMessage("Ei ole puukkoja...", level, false);
                return false;
            } else {
                --inventory.veitset;
            }
            break;
        case 6:
            if (inventory.kivet == 0) {
                printer->showMessage("Ei ole kiviä...", level, false);
                return false;
            } else {
                --inventory.kivet;
            }
            break;
        case 7:
            if (inventory.pamput == 0) {
                printer->showMessage("Ei ole pamppuja...", level, false);
                return false;
            } else {
                --inventory.pamput;
            }
            break;
        case 8:
            if (inventory.bootsit == 0) {
                printer->showMessage("Ei ole camel bootseja...", level, false);
                return false;
            } else {
                --inventory.bootsit;
            }
            break;
        case 9:
            if (inventory.kalat == 0) {
                printer->showMessage("Ei ole turskaa...", level, false);
                return false;
            } else {
                --inventory.kalat;
            }
            break;
        case 10:
            if (inventory.omppo == 0) {
                printer->showMessage("Ei ole omenia...", level, false);
                return false;
            } else {
                --inventory.omppo;
            }
            break;
        case 11:
            if (inventory.veitset == 0) {
                printer->showMessage("Ei ole banskuja...", level, false);
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

bool Player::throwItem()
{
    printer->showMessage("ANNA NUMERO ..", level, false);
    auto number = InputNS::Input::getThrow();
    if (!decrementInventory(number))
    {
        return false;
    }

    auto dir = getDirection(level, printer);
    {
        Coordinate<int> c { static_cast<int>(coord.x), static_cast<int>(coord.y) };
        auto d = dirToCoord(dir);

        Item* i = Item::createItem(number, coord);
        i->thrown = true;
        i->throwVec = d;
        level->addThrownItem(i);
        level->actThrow(this);
    }

    return true;
}

static Building* checkDoor(Level* level, const Coord& c)
{
    for (auto& b : level->buildings) {
        if (b->hitDoor(c)) {
            return b;
        }
    }
    return nullptr;
}

static Building* checkWalls(Level* level, const Coord& c)
{
    for (auto& b : level->buildings) {
        if (b->hitWall(c)) {
            return b;
        }
    }
    return nullptr;
}

static Item* checkItems(Level* level, const Coord& c)
{
    for (auto& i : level->items) {
        if (i->coord == c) {
            return i;
        }
    }
    return nullptr;
}

bool Player::stagger(Level* level)
{
    printer->showMessage("Horjahtelet...", level, false);

    DirectionNS::Direction d;
    if (common::random(50)) {
        d = (common::random(50) ? DirectionNS::left : DirectionNS::left);
    } else {
        d = (common::random(50) ? DirectionNS::up : DirectionNS::down);
    }

    return move(d, level);
}

bool Player::move(DirectionNS::Direction d, Level* level)
{
    // spend turn
    bool ret = true;
    // prevent movement
    bool blocked = false;

    Coord check { coord.x, coord.y };

    switch(d)
    {
        case DirectionNS::down:
            check += { 0, 1 };
            break;
        case DirectionNS::up:
            check -= { 0, 1 };
            break;
        case DirectionNS::right:
            check += { 1, 0 };
            break;
        case DirectionNS::left:
            check -= { 1, 0 };
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
        printer->showMessage(msg, level, entered);
        if (entered)
        {
            // step into building
            b->getInteractMsg(this, msg);
            printer->showMessage(msg, level, true);

            b->interact(this, msg);
            printer->showMessage(msg, level, false);
        }
    }

    if (!blocked) {
        if (!b) {
            b = checkWalls(level, check);
            if (b)
            {
                msg = b->getWalkMsg();
                printer->showMessage(msg, level, true);
                blocked = true;
            }
        }
    }

    if (!blocked) {
        Person* p = level->checkPerson(check);
        if (p)
        {
            blocked = p->interact(msg, this);
            printer->showMessage(msg, level);

            if (p->stats.health == 0) {
                Item* i = p->dropItem();
                if (i) {
                    level->addItem(i);
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
                printer->showMessage(msg, level, false);
                auto consume = i->interact(this);

                if (consume) {
                    level->removeItem(i);
                }
            }
        }
    }

    if (!blocked) {
        coord = check;
    }

    level->playAnimation();

    if (common::checkBounds(coord)) {
        printer->showMessage("Ei karata pelialueelta !!", level, true);
        ret = false;
    }

    return ret;
}

}