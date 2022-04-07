
#include "level.hpp"
#include "item.hpp"
#include <algorithm>
#include <iostream>

using std::find;
using std::cout;
using std::endl;


Level::Level(uint32_t difficulty) : stage(difficulty)
{
}

Level::~Level() {
    for (auto& i : items)
    {
        delete i;
    }
    items.clear();
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
        if ((p->x == x) && (p->y == y))
            return true;
    }

    for (const auto& i : items)
    {
        if ((i->x == x) && (i->y == y))
            return true;
    }

    return false;
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

void Level::addBonas()
{
    srand(time(NULL));
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

bool Level::addBuilding(Building& building)
{
    for (const auto &b : buildings)
    {
        if (b->type == building.type) return false;
    }

    buildings.push_back(&building);
    return true;
}

bool Level::addPerson(Person& person)
{
    for (const auto &p : persons)
    {
        if ((p->x == person.x) && (p->y == person.y)) return false;
    }

    persons.push_back(&person);
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
