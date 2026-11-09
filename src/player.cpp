
#include "player.hpp"
#include "level.hpp"
#include "common.hpp"
#include "item.hpp"
#include "input.hpp"
#include "building.hpp"
#include "animation.hpp"
#include "vehicle.hpp"
#include <iostream>
#include <string>
#include <cmath>
#include <exception>

namespace PlayerNS {

using common::random;
using std::string;
using InputNS::Input;
using InputNS::InputType;
using std::cout;
using std::endl;

bool Player::isVeteran() const
{
    return returnedFromWar && !(level && level->isWarLevel);
}

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

    string text = prefix + ptext + "\xE2\x82\xBD";
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

    string text = prefix + ptext + "\xE2\x80\xB0";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a);
#endif
}

void Player::feedHunger(uint32_t amount)
{
    stats.hunger += amount;
    if (stats.hunger > 100) stats.hunger = 100;
}

void Player::updateTurn()
{
    ++stats.turn;

    if (common::realtimeEnabled) {
        // turns tick 2.5x per second in real-time: lose 0.1 promille every 50 s
        if (stats.turn % 125 == 0) {
            updatepromilles(-1);
        }
        // hunger decreases every 40 turns
        if (stats.turn % 40 == 0 && stats.hunger > 0) {
            --stats.hunger;
        }
        // starving: take damage
        if (stats.hunger == 0 && stats.turn % 60 == 0) {
            damage(1);
            printer->showMessage("Nälkä kalvaa sisuksiasi...", level, false);
        }
        // low hunger warnings
        if (stats.hunger == 20 && stats.turn % 40 == 1) {
            printer->showMessage("Vatsasi kurnii kovasti!", level, false);
        } else if (stats.hunger == 10 && stats.turn % 40 == 1) {
            printer->showMessage("Olet nälästä sekaisin!", level, false);
        }
    } else {
        if (stats.turn % 260 == 0) {
            updatepromilles(-1);
        }
    }
}

bool Player::interact(std::string& msg, Person* source)
{
    if (stats.health == 0) return true;

    if (source->type == yka) {
        if (common::realtimeEnabled) {
            if (inventory.kalja > 0) {
                msg = "Yevgeni pummaa pivoa. Annat yhden.";
                --inventory.kalja;
                return true;
            } else {
                msg = "Yevgeni taputtelee palleaasi. Nyrkillä.";
                damage(1);
                return false;
            }
        }

        msg = "Yevgeni pummaa yhtä pivoa, annatko? (k/e)";
        printer->showMessage(msg, level);
        char c = InputNS::Input::inputYka();
        switch(c) {
            default:
            case 'k':
            case 'K':
                msg = "Yevgeni kiittelee: \"Спасибо, брат!\" ja istuu alas kittaamaan pivoasi.";
                --inventory.kalja;
                return true;
                break;
            case 'e':
            case 'E':
                msg = "Yevgeni taputtelee palleaasi. Nyrkillä.";
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
        printer->showMessage("Kumpaakos horaat, pivoa vai Jaguaria (1,2)?", level);
        auto type = InputNS::Input::getDrink();
        switch(type)
        {
            case InputNS::kalja:
                --inventory.kalja;
                ++inventory.pullot;
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
        ++inventory.pullot;
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
            printer->showMessage("Banaani teki teraa !!!", level);
        }
        feedHunger(10);
    } else if ((inventory.kalat == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        --inventory.omppo;
        if (random(50)) {
            damage(1);
            printer->showMessage("Antonovka oli pilaantunut,oksennat.", level);
        } else {
            powerup(2);
            printer->showMessage("Omena oli herkullinen !!!", level);
            feedHunger(15);
        }
    } else if ((inventory.omppo == 0) && (inventory.bansku == 0) && (inventory.lenkki == 0)) {
        --inventory.kalat;
        if (random(70)) {
            damage(2);
            printer->showMessage("Syomasi kala sisalsi matoja...", level);
        } else {
            powerup(4);
            printer->showMessage("Kala antaa sinulle uutta puhtia !!!", level);
            feedHunger(30);
        }
    } else if ((inventory.kalat == 0) && (inventory.omppo == 0) && (inventory.bansku == 0)) {
        powerup(1);
        --inventory.lenkki;
        printer->showMessage("Oispa edes samogonia kaveriksi.", level);
        feedHunger(25);
    } else {
        printer->showMessage("Mitas...1=makkara 2=kala 3=antonovka 4=banaani ???", level);
        auto type = InputNS::Input::getFood();

        switch(type) {
            case InputNS::foodlenkki:
                if (inventory.lenkki == 0) {
                    printer->showMessage("Ei ole niitä.", level, false);
                } else {
                    --inventory.lenkki;
                    printer->showMessage("Mamamamakkaraa.", level, false);
                    feedHunger(25);
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
                        feedHunger(30);
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
                        printer->showMessage("Joku oli kaytellyt keski keppiaan antonovkaasi.", level, false);
                    } else {
                        powerup(2);
                        printer->showMessage("orb.", level, false);
                        feedHunger(15);
                    }
                }
                break;
            case InputNS::foodbansku:
                if (inventory.bansku == 0) {
                    printer->showMessage("Bailu ilman banaania?", level, false);
                } else {
                    --inventory.bansku;
                    if (random(50)) {
                        printer->showMessage("Banaani ei vaikuttanut.", level, false);
                    } else {
                        powerup(1);
                        printer->showMessage("Babababananiii", level, false);
                    }
                    feedHunger(10);
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
    common::vomitEvent(coord, d == DirectionNS::right ? 1 : d == DirectionNS::left ? -1 : 0,
                       d == DirectionNS::down ? 1 : (d == DirectionNS::up || d == DirectionNS::none) ? -1 : 0);
    common::sound("vomit", 1.0f);
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
                printer->showMessage("Ei ole pivoa...", level, false);
                return false;
            } else {
                --inventory.kalja;
            }
            break;
        case 2:
            if (inventory.lonkka == 0) {
                printer->showMessage("Ei ole Jaguaria...", level, false);
                return false;
            } else {
                --inventory.lonkka;
            }
            break;
        case 3:
            if (inventory.lenkki == 0) {
                printer->showMessage("Ei ole makkaraa...", level, false);
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
                printer->showMessage("Ei ole verkkareita...", level, false);
                return false;
            } else {
                --inventory.bootsit;
            }
            break;
        case 9:
            if (inventory.kalat == 0) {
                printer->showMessage("Ei ole voblaa...", level, false);
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
            if (inventory.bansku == 0) {
                printer->showMessage("Ei ole banaaneja...", level, false);
                return false;
            } else {
                --inventory.bansku;
            }
            break;
        case 13:
            if (inventory.vodka == 0) {
                printer->showMessage("Ei ole vodkaa...", level, false);
                return false;
            } else {
                --inventory.vodka;
                if (inventory.pissVodka.size() > inventory.vodka) inventory.pissVodka.pop_back();
            }
            break;
        case 14:
            if (inventory.granata == 0) {
                printer->showMessage("Ei ole kranaatteja...", level, false);
                return false;
            } else {
                --inventory.granata;
            }
            break;
        default:
            return false;
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
    return executeThrow(number, dir);
}

bool Player::executeThrow(uint32_t itemNum, DirectionNS::Direction dir)
{
    if (level->isWarLevel && std::rand() % 100 < 15) {
        printer->showMessage("Heitto ep\xC3\xA4""onnistuu! Liukkaat k\xC3\xA4""det!", level, false);
        damage(1);
        return true;
    }

    Coordinate<int> c { static_cast<int>(coord.x), static_cast<int>(coord.y) };
    auto d = dirToCoord(dir);

    Item* i = Item::createItem(itemNum, coord);
    i->thrown = true;
    i->throwVec = d;
    level->addThrownItem(i);
    level->actThrow(this);

    return true;
}

static float dirAngle(DirectionNS::Direction d)
{
    switch (d) {
        case DirectionNS::right: return 0.0f;
        case DirectionNS::down:  return 1.5708f;
        case DirectionNS::left:  return 3.14159f;
        default:                 return -1.5708f;
    }
}

bool Player::executeThrowAt(uint32_t itemNum, float angle)
{
    if (level->isWarLevel && std::rand() % 100 < 15) {
        printer->showMessage("Heitto ep\xC3\xA4""onnistuu! Liukkaat k\xC3\xA4""det!", level, false);
        damage(1);
        return true;
    }
    Item* i = Item::createItem(itemNum, coord);
    i->thrown = true;
    i->angled = true;
    i->fx = coord.x + 0.5f;
    i->fy = coord.y + 0.5f;
    i->fdx = std::cos(angle);
    i->fdy = std::sin(angle);
    level->addThrownItem(i);
    level->actThrow(this);
    return true;
}

bool Player::shoot(DirectionNS::Direction dir)
{
    return shootAt(dirAngle(dir));
}

bool Player::shootAt(float angle)
{
    if (inventory.ammukset == 0) {
        printer->showMessage("Ei ammuksia!", level, false);
        return false;
    }
    --inventory.ammukset;

    if (level->isWarLevel) {
        int roll = std::rand() % 100;
        if (roll < 20) {
            printer->showMessage("*KLIK* Ase jumittaa! Ruosteinen romu!", level, false);
            return true;
        } else if (roll < 30) {
            printer->showMessage("PANG! Ase r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4 k\xC3\xA4""dess\xC3\xA4""si!", level, false);
            damage(std::rand()%3 + 2);
            return true;
        }
    }

    level->audio.addSound(coord, SoundType::Gunshot,
        "PAM! Pistooli pamahtaa!", "Pistoolinlaukaus", "...pamaus kaukaa...", 30);
    level->alertCops(this, 30);

    Person* target = level->fireBullet(this, angle, 20, 5, 90);
    if (!target) printer->showMessage("PAM! Huti.", level, false);
    else printer->showMessage(target->stats.health == 0 ? "PAM! Osuma! Kohde kaatuu maahan." : "PAM! Osuma!", level, false);
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
    if (b && b->walkable) {
        // walkable buildings are entered through the door; the way out is always open
        const bool leaving = b->hitBuilding(coord);
        if (leaving || b->getEnterMsg(this, msg)) {
            if (!leaving) {
                printer->showMessage(msg, level, false);
                if (b->getInteractMsg(this, msg)) printer->showMessage(msg, level, false);
                msg.clear();
                b->interact(this, msg);
                if (!msg.empty()) printer->showMessage(msg, level, false);
            }
            coord = check;
            return ret;
        }
        printer->showMessage(msg, level, false);
        return ret;
    }
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
                if (!b->bumpWall(this, check, msg)) msg = b->getWalkMsg();
                printer->showMessage(msg, level, true);
                blocked = true;
            }
        }
    }

    if (!blocked) {
        Car* getaway = dynamic_cast<Car*>(level->getVehicleAt(check));
        if (getaway && getaway->loot) {
            // the robbers left the glovebox unlocked
            getaway->loot = false;
            msg = "Penkot ry\xC3\xB6st\xC3\xA4jien autoa: ";
            const uint32_t rounds = 8 + std::rand() % 10;
            inventory.ammukset += rounds;
            msg += std::to_string(rounds) + " patruunaa";
            if (inventory.pistooli == 0 || common::random(40)) { ++inventory.pistooli; msg += ", pistooli"; }
            if (common::random(40)) { ++inventory.granata; msg += ", kranaatti"; }
            if (common::random(25)) { ++inventory.rynkky; inventory.ammukset += 30; msg += ", RYNKKY"; }
            msg += "!";
            printer->showMessage(msg, level, false);
            blocked = true;
        } else if (Vehicle* car = level->getVehicleAt(check)) {
            msg = car->name() + " on tiell\xC3\xA4. Kolautat p\xC3\xA4\xC3\xA4si sen kylkeen.";
            printer->showMessage(msg, level, false);
            blocked = true;
        }
        Person* p = blocked ? nullptr : level->checkPerson(check);
        if (p && p->hasLoot()) {
            // caught the pickpocket: money, bottles and whatever else he grabbed
            msg = "Sait kiinni! Otat takaisin:";
            if (p->stolenMoney > 0) msg += " " + std::to_string(p->stolenMoney) + " ruplaa,";
            if (p->stolenBottles > 0) msg += " " + std::to_string(p->stolenBottles) + (p->stolenBottles == 1 ? " pullon," : " pulloa,");
            for (int k : p->stolenItems) {
                ++stealable(inventory, k);
                msg += string(" ") + stealableName(k) + ",";
            }
            msg.back() = '.';
            stats.money += p->stolenMoney;
            inventory.pullot += p->stolenBottles;
            p->stolenMoney = 0;
            p->stolenBottles = 0;
            p->stolenItems.clear();
            p->fleeActions = 0;
            p->brawlActions = 0;
            p->say("Ei ollut minun! Ei ollut minun!");
            printer->showMessage(msg, level);
            blocked = true;
            p = nullptr;
        }
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