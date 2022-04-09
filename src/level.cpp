
#include "level.hpp"
#include "item.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "building.hpp"
#include "printer.hpp"
#include "coord.hpp"
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
        if ((p->health > 0) && (p->type != pelaaja))
        {
            p->npcAct(msg);
            printer->showMessage(msg, *this, true);
        }
    }
}

Person* Level::getPerson(uint32_t x, uint32_t y) const
{
    for (auto& p : persons) {
        if ((p->coord.x == x) && (p->coord.y == y)) {
            return p;
        }
    }
    return nullptr;
}

Person* Level::getPerson(const Coord& coord) const
{
    return getPerson(coord.x, coord.y);
}

void Level::alertCops()
{
    for (auto& p : persons) {
        if (p->type == poliisi)
        {
            dynamic_cast<Cop*>(p)->attack = true;
        }
    }
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

Player* Level::findPlayer()
{
    for (const auto& p : persons) {
        if (p->type == pelaaja) return dynamic_cast<Player*>(p);
    }
    return nullptr;
}

bool Level::hit(uint32_t x, uint32_t y) const
{
    for (const auto& b : buildings)
    {
        if (b->hitBuilding(x, y))
            return true;
    }

    for (const auto& p : persons)
    {
        if ((p->coord.x == x) && (p->coord.y == y) && (p->health > 0))
            return true;
    }

    for (const auto& i : items)
    {
        if ((i->coord.x == x) && (i->coord.y == y))
            return true;
    }

    return false;
}

bool Level::hit(const Coord& coord) const
{
    return hit(coord.x, coord.y);
}

Item* Level::getItem(const Coord& c) const
{
    for (auto& i : items) {
        if (i->coord == c) return i;
    }
    return nullptr;
}

void Level::freePosition(uint32_t& x, uint32_t& y) const
{
    bool recheck;
    do {
        x = rand() % sizex;
        y = rand() % sizey;
        recheck = hit(x, y);
    } while (recheck);
}

Coord Level::freePosition() const
{
    Coord c {0, 0};
    freePosition(c.x, c.y);
    return c;
}

Person* Level::checkPerson(uint32_t checkx, uint32_t checky)
{
    for (auto& p : persons){
        if ((p->coord.x == checkx) && (p->coord.y == checky))
        {
            return p;
        }
    }
    return nullptr;
}

void Level::addBonas()
{
    for (uint32_t i = 0; i < stage; ++i)
    {
        uint32_t x;
        uint32_t y;

        freePosition(x, y);

        auto b = new Bona(x, y);
        items.push_back(b);
    }
}

void Level::addItem(ItemType item)
{
    uint32_t x;
    uint32_t y;
    freePosition(x, y);

    Item* i;
    switch(item)
    {
        default:
            cout << "ERROR: addItem\n";
            i = nullptr;
            break;
        case EKalja:
            i = new Kalja(x, y);
            break;
        case ELonkka:
            i = new Lonkka(x, y);
            break;
        case ELenkki:
            i = new Lenkki(x, y);
            break;
        case EPamppu:
            i = new Pamppu(x, y);
            break;
        case EVeitsi:
            i = new Veitsi(x, y);
            break;
        case EKetjut:
            i = new Ketjut(x, y);
            break;
        case EKivi:
            i = new Kivi(x, y);
            break;
        case EBootsit:
            i = new Bootsit(x, y);
            break;
        case EKala:
            i = new Kala(x, y);
            break;
        case EOmppo:
            i = new Omppo(x, y);
            break;
        case EBanaani:
            i = new Banaani(x, y);
            break;
        case ERaha:
            i = new Raha(x, y);
            break;
        case EPaska:
            i = new Paska(x, y);
            break;
        case EViesti:
            i = new Viesti(x, y);
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

    persons.push_back(person);
    return true;
}

void Level::addItem(Item* item)
{
    items.push_back(item);
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
