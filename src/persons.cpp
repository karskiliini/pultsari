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
using DirectionNS::Direction;

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

Direction Person::getMoveDirection(const Coord& target) const
{
    auto dx = max(coord.x, target.x) - (min(coord.x, target.x));
    auto dy = max(coord.y, target.y) - (min(coord.y, target.y));

    if (dx > dy) {
        if (coord.x > target.x) {
            return DirectionNS::left;
        } else if (coord.x < target.x) {
            return DirectionNS::right;
        } else if (coord.y > target.y) {
            return DirectionNS::up;
        } else if (coord.y < target.y) {
            return DirectionNS::down;
        }
    } else {
        if (coord.y < target.y) {
            return DirectionNS::down;
        } else if (coord.y > target.y) {
            return DirectionNS::up;
        } else if (coord.x > target.x) {
            return DirectionNS::left;
        } else if (coord.x < target.x) {
            return DirectionNS::right;
        }
    }

    return DirectionNS::none;
}

bool Person::move(Direction d)
{
    return true;
}

// MUMMO
Mummo::Mummo(const Coord& pos) : Person(mummo, pos)
{
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
Cop::Cop(const Coord& pos) : Person(poliisi, pos), target(pos)
{
}

bool Cop::move(Direction d)
{
    return false;
}

bool Cop::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            if (p->type == varas) {
                msg = "Poliisi pidätti varkaan!";
                p->health = 0;
            }
        }
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
        target = p->coord;

        if (distance(coord.x, coord.y, target.x, target.y) == 1)
        {
            msg = "Pollari pamputtaa !!!";
            p->damage(rand()%2+1);
        } else {
            Direction d = getMoveDirection(target);
            move(d, msg);
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


// VARAS
Varas::Varas(const Coord& pos) : Person(varas, pos), target(pos)
{
}

bool Varas::move(Direction d)
{
    return false;
}

bool Varas::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            if (p->type == poliisi) {
                msg = "Poliisi pidätti varkaan!";
                health = 0;
            }
        }
    }
    checkBounds(level->sizex, level->sizey);
    return true;
}

void Varas::npcAct(string& msg)
{
    Player* p = level->findPlayer();
    target = p->coord;

    if (distance(coord.x, coord.y, target.x, target.y) == 1)
    {
        msg = "Varas hipelöi sinua !!!";

        if (rand()%2)
        {
            uint32_t sum = rand() % 16 + 1;
            if (sum > p->money) sum = p->money;
            p->money -= sum;
            msg = "Lompakkosi kevenee " + std::to_string(sum) + " markalla.";
            health = 0;
        }
    } else {
        Direction d = getMoveDirection(target);
        move(d, msg);
    }
}

bool Varas::interact(std::string& message, Person* source)
{
    if (random(50))
    {
        message = "VARAS suuttuu sinulle!";
    } else {
        message = "VARAS kellahtaa ketoon !!!";
        health = 0;
    }

    level->alertCops();

    return true;
}
