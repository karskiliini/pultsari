
#include "level.hpp"
#include "item.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "building.hpp"
#include "printer.hpp"
#include "coord.hpp"
#include "common.hpp"
#include "animation.hpp"
#include <algorithm>
#include <iostream>

using std::find;
using std::cout;
using std::endl;
using std::string;
using PlayerNS::Player;

Level::Level(uint32_t difficulty) : stage(difficulty)
{
}

Level::~Level() {
    for (auto& p : buildings)
    {
        delete p;
    }
    buildings.clear();

    for (auto& p : persons)
    {
        // player needs to remain from level to level
        if (p->type != PersonType::pelaaja) {
            delete p;

        }
    }
    persons.clear();

    for (auto& p : items)
    {
        delete p;
    }
    items.clear();
}

void Level::buildingTurn()
{
    for (auto& b : buildings)
    {
        b->npcAct();
    }
}

void Level::npcTurn(Printer* printer)
{
    for(auto& p : persons) {
        string msg = "";
        if (p->health > 0)
        {
            p->npcAct(printer);
        }
    }
}

void Level::actThrow(Person* source)
{
    bool hit = false;
    while (!hit) {
        thrownItem->actThrow(this);

        auto p = getPerson(thrownItem->coord);
        if (p) {
            string msg = "";
            hit = p->interactThrow(thrownItem, source, msg);
            printer->showMessage(msg, *this, false);
        } else {
            if (hitBuilding(thrownItem->coord))
            {
                printer->showMessage("Osuit seinään.", *this, false);
                hit = true;
            } else {
                Coord copy { thrownItem->coord.x, thrownItem->coord.y };
                if (common::checkBounds(copy)) {
                    printer->showMessage("Huti meni.", *this, false);
                    hit = true;
                }
            }
        }

        if (hit)
        {
            delete thrownItem;
            thrownItem = nullptr;
        }

        printer->print(*this);
        common::sleep(16);
    }
}

void Level::playAnimation()
{
    bool running = true;
    while (animation && running)
    {
        running = animation->act();
        printer->print(*this);
        common::sleep(animation->delay);
    }

    delete animation;
    animation = nullptr;

    printer->print(*this);
}

Person* Level::createPerson(const Coord& coord, PersonType type)
{
    Person* p = nullptr;

    if (!hit(coord))
    {
        switch(type) {
            case vanki:
                p = new Vanki(coord);
                break;
            case skinhead:
                p = new Skinhead(coord);
                break;
            case yka:
                p = new Yka(coord);
                break;
            case mummo:
                p = new Mummo(coord);
                break;
            case poliisi:
                p = new Cop(coord);
                break;
            case varas:
                p = new Varas(coord);
                break;
            default:
                break;
        }
    }

    if (p) {
        p->setLevel(this);
        addPerson(p);
    }

    return p;
}

Person* Level::getPerson(const Coord& coord) const
{
    for (auto& p : persons) {
        if (p->coord == coord) {
            return p;
        }
    }
    return nullptr;
}

Player* Level::getPlayer() const
{
    for (const auto& p : persons) {
        if (p->type == pelaaja) return dynamic_cast<Player*>(p);
    }
    return nullptr;
}

bool Level::PersonExists(PersonType type) const
{
    for (auto& p : persons) {
        if (p->type == type) return true;
    }
    return false;
}

void Level::alertCops(Person* source)
{
    attack = source;
}

void Level::cleanDead()
{
    bool restart = true;
    while(restart) {
        restart = false;

        for (auto& p : persons) {
            if ((p->health == 0) && (p->type != pelaaja)) {
                delete p;
                p = nullptr;

                auto it = find(persons.begin(), persons.end(), p);
                persons.erase(it);

                restart = true;
                break;
            }
        }
    }
}

void Level::cleanDiscardedItems()
{
    bool restart = true;
    while(restart) {
        restart = false;

        for (auto& i : items) {
            if (i->discard) {
                delete i;
                i = nullptr;

                auto it = find(items.begin(), items.end(), i);
                items.erase(it);

                restart = true;
                break;
            }
        }
    }
}

bool Level::hitBuilding(const Coord& c) const
{
    for (const auto& b : buildings)
    {
        if (b->hitBuilding(c))
            return true;
    }
    return false;
}

bool Level::hitPerson(const Coord& c) const
{
    for (const auto& p : persons)
    {
        if ((p->coord == c) && (p->health > 0))
            return true;
    }
    return false;
}

bool Level::hitItem(const Coord& c) const
{
    for (const auto& i : items)
    {
        if ((i->coord == c))
            return true;
    }
    return false;
}

bool Level::hit(const Coord& c) const
{
    bool hit = hitBuilding(c);
    if (hit) return true;

    hit = hitPerson(c);
    if (hit) return true;

    return hitItem(c);
}

Item* Level::getItem(const Coord& c) const
{
    for (auto& i : items) {
        if (i->coord == c) return i;
    }
    return nullptr;
}

Coord Level::freePosition() const
{
    Coord coord;

    bool recheck;
    do {
        coord.x = rand() % sizex;
        coord.y = rand() % sizey;
        recheck = hit(coord);
    } while (recheck);

    return coord;
}

Person* Level::checkPerson(const Coord& c)
{
    for (auto& p : persons) {
        if (p->coord == c) {
            return p;
        }
    }
    return nullptr;
}

void Level::addBonas()
{
    for (uint32_t i = 0; i < stage; ++i) {
        Coord c = freePosition();
        auto b = new Bona(c);
        items.push_back(b);
    }
}

void Level::addItem(ItemType item)
{
    Coord c = freePosition();

    Item* i;
    switch(item)
    {
        default:
            cout << "ERROR: addItem\n";
            i = nullptr;
            break;
        case EKalja:
            i = new Kalja(c);
            break;
        case ELonkka:
            i = new Lonkka(c);
            break;
        case ELenkki:
            i = new Lenkki(c);
            break;
        case EPamppu:
            i = new Pamppu(c);
            break;
        case EVeitsi:
            i = new Veitsi(c);
            break;
        case EKetjut:
            i = new Ketjut(c);
            break;
        case EKivi:
            i = new Kivi(c);
            break;
        case EBootsit:
            i = new Bootsit(c);
            break;
        case EKala:
            i = new Kala(c);
            break;
        case EOmppo:
            i = new Omppo(c);
            break;
        case EBanaani:
            i = new Banaani(c);
            break;
        case ERaha:
            i = new Raha(c);
            break;
        case EPaska:
            i = new Paska(c);
            break;
        case EViesti:
            i = new Viesti(c);
            break;
    }
    if (i) {
        addItem(i);
    }
}

bool Level::addBuilding(Building* building)
{
    if (!building) return false;

    for (const auto &b : buildings)
    {
        if (b->type == building->type) return false;
    }

    building->level = this;
    buildings.push_back(building);
    return true;
}

bool Level::addPerson(Person* person)
{
    for (const auto &p : persons)
    {
        if ((p->coord.x == person->coord.x) && (p->coord.y == person->coord.y))
            return false;
    }

    person->setLevel(this);
    persons.push_back(person);
    return true;
}

void Level::addItem(Item* item)
{
    items.push_back(item);
}

void Level::addThrownItem(Item* item)
{
    thrownItem = item;
}

void Level::addAnimation(Animation* animation)
{
    Level::animation = animation;
    playAnimation();
}

void Level::removeItem(Item* item)
{
    delete item;
    auto it = find(items.begin(), items.end(), item);
    if (it != items.end())
    {
        items.erase(it);
    }
}

void moveAngle(float ang, Coord& from, const Coord& to, bool debug)
{
#if 1
#else
    if (ang > 1.f && (from.x != to.x)) {
        if (debug)
            cout << "ang: " << ang << " from: " << from.x << ", " << from.y << " to: " << to.x << ", " << to.y << "            " << endl;

        if (from.x > to.x) {
            --from.x;
        } else if (from.x < to.x) {
            ++from.x;
        }
    } else if ((ang < 1.f) && (from.y != to.y)) {
        if (debug)
            cout << "ang: " << ang << " from: " << from.x << ", " << from.y << " to: " << to.x << ", " << to.y << "            "<< endl;
        if (from.y > to.y) {
            --from.y;
        } else if (from.y < to.y) {
            ++from.y;
        }
    } else
    {
        if (from.x > to.x) {
            --from.x;
        } else if (from.x < to.x) {
            ++from.x;
        }
        if (from.y > to.y) {
            --from.y;
        } else if (from.y < to.y) {
            ++from.y;
        }
    }
#endif
}

// return true if visibility ok
bool Level::raycast(Coord from, const Coord& to) const
{
    bool originBuilding = hitBuilding(to);

    bool debug = false;
    if ((to.y == 3) && (to.x == 22)) debug = true;
    uint32_t block = 0;

    uint32_t dx = std::max(from.x, to.x) - std::min(from.x, to.x);
    uint32_t dy = std::max(from.y, to.y) - std::min(from.y, to.y);

    // todo: angle should be different if dx < 0
    float ang = (dy == 0) ? 0.f : (float)((float)dx / (float)dy);
    // if (debug) cout << "start: ang: " << ang << " " << from.x << ", " << from.y <<" to: " << to.x << ", " << to.y << "               " << endl;

    if (ang < 1.f) {
        bool ret = false;
        int k1 = (ang == 0) ? 100 : 1.f / ang;
        int k2 = (ang == 0) ? 100 : ceil(1.f / ang);
        Coord tmp = from;
        for (float k : { k1, k2 }) {
            tmp = from;
            while(from != to) {
                // if (debug)
                //     cout << ang << " k: " << k << " " << from.x << ", " << from.y << " to: " << to.x << ", " << to.y << "               " << endl;
                for (int i = (int)k; i > 0; --i)
                {
                    if (from.y < to.y) ++from.y;
                    else if (from.y > to.y) --from.y;
                }
                if (from.x < to.x) ++from.x;
                else if (from.x > to.x) --from.x;

                if (originBuilding) {
                    // always show building internals
                } else {
                    block += (hitBuilding(from) || hitPerson(from)) ? 1 : 0;
                }
                if ((block > 0) && (from == to))
                {
                    ret = true;
                    break;
                }
                if (block > 0) {
                    break;
                }
            }
        }
        if (block == 0) { ret = true; }
        return ret;

    } else if (ang >= 1.f) {
        bool ret = false;
        float k1 = ang;
        float k2 = ceil(ang);

        Coord tmp = from;
        for (float k : { k1, k2 }) {
            from = tmp;

            while(from != to) {
                for (int i = (int)k; i > 0; --i)
                {
                    if (from.x < to.x) ++from.x;
                    else if (from.x > to.x) --from.x;
                }
                if (from.y < to.y) ++from.y;
                else if (from.y > to.y) --from.y;

                if (originBuilding) {
                    // always show building internals
                } else {
                    block += (hitBuilding(from) || hitPerson(from)) ? 1 : 0;
                }
                if ((block > 0) && (from == to)) {
                    ret = true;
                    break;
                }
                if (block > 0) {
                    break;
                }
            }
        }
        if (block == 0) { ret = true; }
        return ret;
    }
    return true;
}

Person* Level::raycast(const Coordinate<int>& from, const Coordinate<int>& vector) const
{
    if (vector == Coordinate<int>(0, 0)) return nullptr;

    Person* p = nullptr;
    while (p == nullptr) {
        Coord c { (uint32_t)from.x, (uint32_t)from.y };
        p = getPerson(c);

        if (!p)
        {
            Coord copy { c.x, c.y };
            if (!common::checkBounds(copy)) {
                return nullptr;
            }
        }
    }
    return p;
}


Building* Level::raycastBuilding(const Coordinate<int>& from, const Coordinate<int>& vector) const
{
    return nullptr;
}
