#include "persons.hpp"
#include "level.hpp"

Person::Person(const Person &obj)
{

}

void Person::setLevel(Level* l)
{
    level = l;
}

bool Person::move(DirectionNS::Direction d)
{
    return false;
}
