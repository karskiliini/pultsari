#include "persons.hpp"
#include "player.hpp"
#include "level.hpp"
#include "item.hpp"
#include "common.hpp"
#include <iostream>

using std::string;
using PlayerNS::Player;
using std::cout;
using std::endl;
using std::max;
using std::min;

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

// MUMMO
Mummo::Mummo(uint32_t x, uint32_t y) : Person(mummo)
{
    Person::coord.x = x;
    Person::coord.y = y;
}

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

    level->alertCops();

    return true;
}

Item* Mummo::dropItem()
{
    return new Raha(coord.x, coord.y, rand()%25 + 1);
}


// COP
Cop::Cop(uint32_t x, uint32_t y) : Person(poliisi)
{
    Person::coord.x = x;
    Person::coord.y = y;
}

bool Cop::move(DirectionNS::Direction d)
{
    uint32_t checkx = coord.x;
    uint32_t checky = coord.y;
    switch(d)
    {
        case DirectionNS::up: --checky; break;
        case DirectionNS::right: ++checkx; break;
        case DirectionNS::down: ++checky; break;
        case DirectionNS::left: --checkx; break;
        default: return false;
    }
    if (!level->hit(checkx, checky))
    {
        coord.x = checkx;
        coord.y = checky;
    }
    checkBounds(level->sizex, level->sizey);
    return true;
}

void Cop::npcAct(string& msg)
{
    if (!attack)
    {
        Player* p = level->findPlayer();
        if (distance(coord.x, coord.y, p->coord.x, p->coord.y) == 1)
        {
            msg = "Pollari varoittaa!";
        }
    } else {
        Player* p = level->findPlayer();
        if (distance(coord.x, coord.y, p->coord.x, p->coord.y) < 2)
        {
            msg = "Pollari pamputtaa !!!";
            p->damage(rand()%2+1);
        } else {
            auto dx = max(coord.x, p->coord.x) - (min(coord.x, p->coord.x));
            auto dy = max(coord.y, p->coord.y) - (min(coord.y, p->coord.y));

            if (dx > dy) {
                if (coord.x > p->coord.x) {
                    move(DirectionNS::left);
                } else if (coord.x < p->coord.x) {
                    move(DirectionNS::right);
                } else if (coord.y > p->coord.y) {
                    move(DirectionNS::up);
                } else if (coord.y < p->coord.y) {
                    move(DirectionNS::down);
                }
            } else {
                if (coord.y < p->coord.y) {
                    move(DirectionNS::down);
                } else if (coord.y > p->coord.y) {
                    move(DirectionNS::up);
                } else if (coord.x > p->coord.x) {
                    move(DirectionNS::left);
                } else if (coord.x < p->coord.x) {
                    move(DirectionNS::right);
                }
            }
        }
    }
}

bool Cop::interact(std::string& message, Person* source)
{
    if (random(50))
    {
        message = "Kyttä suuttuu sinulle!";
        attack = true;
    } else {
        message = "Kyttä kellahtaa ketoon !!!";
        health = 0;
    }

    level->alertCops();

    return true;
}

Item* Cop::dropItem()
{
    return new Pamppu(coord.x, coord.y);
}
