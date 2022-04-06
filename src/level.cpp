
#include "level.hpp"
#include "item.hpp"
#include <algorithm>
#include <iostream>

using std::find;

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

void Level::addBonas()
{
    for (uint32_t i = 0; i < stage; ++i)
    {
        uint32_t x;
        uint32_t y;

        bool recheck;
        do {
            srand(time(NULL));
            x = rand() % sizex;
            y = rand() % sizey;
            recheck = hit(x, y);
        } while (recheck);

        auto b = new Bona(x, y);
        items.push_back(b);
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
