#include "persons.hpp"
#include "level.hpp"

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

Mummo::Mummo() : Person(mummo)
{
}

void Mummo::npcAct() { }

bool Mummo::interact(std::string& message, Person* source)
{
    if (random(50))
    {
        message = "Eläkellääinen hakkaa sua käsilaukullaan !!!";
        uint32_t damage = rand() % 1 + 1;
        source->damage(damage);
        return false;
    } else {
        message = "Mukiloit mummon kumoon. Haa,lompsa !!!";
        return true;
    }
}
