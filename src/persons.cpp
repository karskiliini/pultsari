#include "persons.hpp"
#include "level.hpp"
#include "item.hpp"

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

Mummo::Mummo(uint32_t x, uint32_t y) : Person(mummo)
{
    Person::x = x;
    Person::y = y;
}

void Mummo::npcAct() { }

bool Mummo::interact(std::string& message, Person* source)
{
    if (random(50))
    {
        message = "Eläkellääinen hakkaa sua käsilaukullaan !!!";
        uint32_t damage = rand() % 1 + 1;
        source->damage(damage);
    } else {
        message = "Mukiloit mummon kumoon. Haa,lompsa !!!";
        health = 0;
    }
    return true;
}

Item* Mummo::dropItem()
{
    return new Raha(x, y, rand()%25 + 1);
}
