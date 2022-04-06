
#include "level.hpp"
#include "item.hpp"
#include <algorithm>
#include <iostream>

using std::find;

Level::Level(uint32_t difficulty) : stage(difficulty)
{
    for (uint32_t i = 0; i < stage; ++i)
    {
        uint32_t x = rand() % sizex;
        uint32_t y = rand() % sizey;
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

void Level::removeItem(Item* item)
{
    delete item;
    auto it = find(items.begin(), items.end(), item);
    if (it != items.end())
    {
        items.erase(it);
    }
}