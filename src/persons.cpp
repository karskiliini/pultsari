#include "persons.hpp"
#include "player.hpp"
#include "level.hpp"
#include "item.hpp"
#include "common.hpp"
#include "coord.hpp"
#include <iostream>

using std::string;
using PlayerNS::Player;
using std::cout;
using std::endl;
using std::max;
using std::min;
using DirectionNS::Direction;


void Person::damage(uint32_t damage)  {
    if (damage >= health) {
        if (this == level->attack) level->attack = nullptr;
        health = 0;
    } else {
        health -= damage;
    }
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
    if (source->type == pelaaja)
    {
        if (common::random(50))
        {
            message = "Eläkellääinen hakkaa sua käsilaukullaan !!!";
            uint32_t damage = rand() % 1 + 1;
            source->damage(damage);
        } else {
            message = "Mukiloit mummon kumoon. Haa,lompsa !!!";
            damage(health);
        }

        level->alertCops(source);
    }

    return true;
}

void Mummo::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Mummo heitti kuperkeikan ja hukkasi käsilaukkunsa.";
    damage(health);
    level->alertCops(source);

    auto i = dropItem();
    level->addItem(i);
}

Item* Mummo::dropItem()
{
    return new Raha(coord, rand()%25 + 1);
}


// COP
Cop::Cop(const Coord& pos) : Person(poliisi, pos)
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
                p->damage(p->health);
            }
        }
    }
    common::checkBounds(coord);
    return true;
}

void Cop::npcAct(string& msg)
{
    if (!level->attack)
    {
        Player* p = level->findPlayer();
        if (coord.distance(p->coord) == 1)
        {
            msg = "Pollari varoittaa!";
        }
    } else {
        Player* p = level->findPlayer();
        if (p == level->attack)
        {
            if (coord.distance(level->attack->coord) == 1)
            {
                msg = "Pollari pamputtaa !!!";
                p->damage(rand()%2+1);
            } else {
                Direction d = getMoveDirection(level->attack->coord);
                move(d, msg);
            }
        } else {
            Direction d = getMoveDirection(level->attack->coord);
            move(d, msg);
        }
    }
}

bool Cop::interact(std::string& message, Person* source)
{
    if (source->type == pelaaja)
    {
        if (common::random(50))
        {
            message = "Huitaiset kyylää!";
        } else {
            message = "Kyttä kellahtaa ketoon !!!";
            damage(health);
        }
        level->alertCops(source);
    }

    return true;
}

void Cop::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Poliisi kuoli ja pudotti pampun.";
    damage(1);

    level->alertCops(source);

    auto i = dropItem();
    level->addItem(i);
}

Item* Cop::dropItem()
{
    return new Pamppu(coord);
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
        case DirectionNS::up:    check -= { 0, 1 }; break;
        case DirectionNS::right: check += { 1, 0 }; break;
        case DirectionNS::down:  check += { 0, 1 }; break;
        case DirectionNS::left:  check -= { 1, 0 }; break;
        default: return false;
    }

    if (!level->hit(check)) {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            if (p->type == poliisi) {
                msg = "Poliisi pidätti varkaan!";
                damage(health);
            } else if (p->type == mummo) {
                msg = "Mummo jäi varkaan kynsiin!";
                level->alertCops(this);
                p->damage(p->health);
            }
        }

        auto i = level->getItem(check);
        if (i)
        {
            if (i->type == EPaska)
            {
                msg += (msg == "") ? "" : "\n";
                msg += "Varas liukastui paskaan, ja kuoli.";
                i->discard = true;
                damage(health);
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Varas::npcAct(string& msg)
{
    Player* p = level->findPlayer();
    target = p->coord;

    if (coord.distance(target) == 1)
    {
        msg = "Varas hipelöi sinua !!!";

        if (rand()%2)
        {
            uint32_t sum = rand() % 16 + 1;
            if (sum > p->money) sum = p->money;
            p->money -= sum;
            msg = "Lompakkosi kevenee " + std::to_string(sum) + " markalla.";
            damage(health);
        }
    } else {
        Direction d = getMoveDirection(target);
        move(d, msg);
    }
}

bool Varas::interact(std::string& message, Person* source)
{
    if (source->type == pelaaja)
    {
        if (common::random(50))
        {
            message = "VARAS suuttuu sinulle!";
        } else {
            message = "VARAS kellahtaa ketoon !!!";
            damage(health);
        }
    }

    return true;
}


// VANKI
Vanki::Vanki(const Coord& pos) : Person(vanki, pos), target(pos)
{
    health = 4;
}

bool Vanki::move(Direction d)
{
    return false;
}

bool Vanki::move(Direction d, std::string& msg)
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
                msg = "Poliisi hapettaa vankikarkurin!";
                damage(health);
            } else if (p->type == mummo) {
                msg = "Vankikarkuri tönäisee muorin nurin!";
                p->damage(p->health);
                level->alertCops(this);
            }
        }

        auto i = level->getItem(check);
        if (i)
        {
            switch(i->type) {
                case EPaska:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri liukastuu paskaan";
                    i->discard = true;
                    damage(1);
                    if (health == 0)
                    {
                        msg += "\nja vankikarkuri kuoli.";
                    }
                    break;
                case ELenkki:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri ahmii kiekuralenkin ja voimistuu";
                    ++health;
                    i->discard = true;
                    break;
                case EKivi:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri nappaa kiven.";
                    i->discard = true;
                    kivi = true;
                    break;
                default:
                    break;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Vanki::npcAct(string& msg)
{
    if (firstMove)
    {
        firstMove = false;
        return;
    }

    Player* p = level->findPlayer();
    target = p->coord;

    if (coord.distance(target) == 1)
    {
        msg = "Vankikarkuri kurmottaa sinua!";

        uint32_t damage = rand()%3 + 1;
        p->damage(damage);
    } else {
        if (kivi && ((coord.x == p->coord.x) || (coord.y == p->coord.y)))
        {
            msg = "Vankikarkuri heittää kivellä !!!";
            kivi = false;
        } else {
            Direction d = getMoveDirection(target);
            move(d, msg);
        }
    }
}

bool Vanki::interact(std::string& message, Person* source)
{
    if (source->type == pelaaja)
    {
        message = "Hakkaat hullun lailla vankikarkuria!";
        damage(1);
    }

    if (health == 0)
    {
        message = message + "\n Vanki kellahtaa ketoon !!!";
        damage(health);
    }

    return true;
}

void Vanki::interactThrow(Item* item, Person* source, std::string& msg)
{
    if (common::random(50)) {
        msg = "Roisto väisti heittosi !";
    } else {
        uint32_t d = rand() % 3 + 1;
        if (d >= health) {
            msg = "Vankikarkuri tuupertuu ketoon.";
        } else {
            msg = "Osuit vankikarkuriin, mutta hän näyttää entistä vihaisemmalta!";
        }
        damage(d);
    }
}
