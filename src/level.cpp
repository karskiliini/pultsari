
#include "level.hpp"
#include "item.hpp"

Level::Level(uint32_t difficulty) : stage(difficulty)
{
    for (uint32_t i = 0; i < stage; ++i)
    {
        uint32_t x = rand() % sizex;
        uint32_t y = rand() % sizey;
        Bona b(x, y);
        bonat.push_back(b);
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
