#include "item.hpp"
#include "player.hpp"
#include "level.hpp"

// bona
Bona::Bona(uint32_t x, uint32_t y) : Item(x, y) { }

char Bona::typeToChar() const
{
    return '.';
}

std::string Bona::getMsg() const
{
    return "Löysit bonan!";
}

// paska
Paska::Paska(uint32_t x, uint32_t y) : Item(x, y) { }

char Paska::typeToChar() const
{
    return 'p';
}

std::string Paska::getMsg() const
{
    return "Huomaat sen liian myohaan... liukastut paskaan.";
}

bool Paska::interact(PlayerNS::Player* player)
{
    player->health -= 1;
    return true;
}
