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
    Person::x = x;
    Person::y = y;
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
    return new Raha(x, y, rand()%25 + 1);
}


// COP
Cop::Cop(uint32_t x, uint32_t y) : Person(poliisi)
{
    Person::x = x;
    Person::y = y;
}

bool Cop::move(DirectionNS::Direction d)
{
    uint32_t checkx = x;
    uint32_t checky = y;
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
        x = checkx;
        y = checky;
    }
    checkBounds(level->sizex, level->sizey);
    return true;
}

void Cop::npcAct(string& msg)
{
    if (!attack)
    {
        Player* p = level->findPlayer();
        if (distance(x, y, p->x, p->y) == 1)
        {
            msg = "Pollari varoittaa!";
        }
    } else {
        Player* p = level->findPlayer();
        if (distance(x, y, p->x, p->y) < 2)
        {
            msg = "Pollari pamputtaa !!!";
            p->damage(rand()%2+1);
        } else {
            auto dx = max(x, p->x) - (min(x, p->x));
            auto dy = max(y, p->y) - (min(y, p->y));

            if (dx > dy) {
                if (x > p->x) {
                    move(DirectionNS::left);
                } else if (x < p->x) {
                    move(DirectionNS::right);
                } else if (y > p->y) {
                    move(DirectionNS::up);
                } else if (y < p->y) {
                    move(DirectionNS::down);
                }
            } else {
                if (y < p->y) {
                    move(DirectionNS::down);
                } else if (y > p->y) {
                    move(DirectionNS::up);
                } else if (x > p->x) {
                    move(DirectionNS::left);
                } else if (x < p->x) {
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
    return new Pamppu(x, y);
}
