#include "item.hpp"
#include "player.hpp"
#include "level.hpp"

Bona::Bona(uint32_t x, uint32_t y) : Item(x, y)
{
}

Bona::~Bona() { }

char Bona::typeToChar() const
{
    return '.';
}

std::string Bona::getMsg() const
{
    return "Löysit bonan!";
}

/*
void Bona::interact(Level* level, PlayerNS::Player* player)
{
}
*/