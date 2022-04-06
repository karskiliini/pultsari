
#include "level.hpp"

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
