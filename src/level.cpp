
#include "level.hpp"
#include "item.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "building.hpp"
#include "printer.hpp"
#include "coord.hpp"
#include "common.hpp"
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

Person* Level::getPerson(const Coord& coord) const
{
    for (auto& p : persons) {
        cout << "person: " << this << endl;
        if (p->coord == coord) {
            return p;
        }
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

bool Level::hit(const Coord& c) const
{
    for (const auto& b : buildings)
    {
        if (b->hitBuilding(c))
            return true;
    }

    for (const auto& p : persons)
    {
        if ((p->coord == c) && (p->health > 0))
            return true;
    }

    for (const auto& i : items)
    {
        if ((i->coord == c))
            return true;
    }

    return false;
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

void Level::removeItem(Item* item)
{
    delete item;
    auto it = find(items.begin(), items.end(), item);
    if (it != items.end())
    {
        items.erase(it);
    }
}

Person* Level::raycast(const Coordinate<int>& from, const Coordinate<int>& vector) const
{
    Person* p = nullptr;
    while (p == nullptr) {
        Coord c { (uint32_t)from.x, (uint32_t)from.y };
        p = getPerson(c);

        {
            Coord copy { c.x, c.y };
            if (!common::checkBounds(copy)) {
                return nullptr;
            }
        }
    }
    return p;
}
