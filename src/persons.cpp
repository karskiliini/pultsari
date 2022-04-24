#include "persons.hpp"
#include "player.hpp"
#include "level.hpp"
#include "item.hpp"
#include "common.hpp"
#include "coord.hpp"
#include "animation.hpp"
#include "pathmask.hpp"
#include <string>
#include <iostream>

using std::string;
using PlayerNS::Player;
using std::cout;
using std::endl;
using std::max;
using std::min;
using DirectionNS::Direction;
using common::random;

void Person::powerup(uint32_t damage)  {
    health += damage;

#ifdef ANIMATIONS_ENABLED
    string text = "+" + std::to_string(damage) + "hp";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a);
#endif
}

void Person::damage(uint32_t damage)  {
    if (damage >= health) {
        if (this == level->attack) level->attack = nullptr;
        health = 0;
    } else {
        health -= damage;
    }

#ifdef ANIMATIONS_ENABLED
    string text = "-" + std::to_string(damage) + "hp";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a);
#endif
}

Direction Person::getMoveDirection(const Coord& target) const
{
    if (pathFinding) {
        // use advanced pathfinding
        PathNS::PathMask mask(level);
        bool found = mask.findPath(coord, target);

        if (found) {
#if 0
            printer->pathmask = &mask;
            printer->print(*level);
            printer->pathmask = nullptr;
#endif
            // path is done, now trace back to origin
            auto player = level->getPlayer();
            const auto& pCoord = player->coord;
            return mask.traceBack(pCoord);
        } else {
            return DirectionNS::none;
        }

    } else {
        // use classic pultsari 1.0 pathfinding
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
    }

    return DirectionNS::none;
}

// MUMMO
Mummo::Mummo(const Coord& pos) : Person(mummo, pos)
{
}

bool Mummo::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        if (random(50))
        {
            msg = "Eläkellääinen hakkaa sua käsilaukullaan !!!";
            uint32_t damage = rand() % 1 + 1;
            source->damage(damage);
        } else {
            msg = "Mukiloit mummon kumoon. Haa,lompsa !!!";
            damage(health);
            ++dynamic_cast<Player*>(source)->scoreBoard.mummot;
        }

        level->alertCops(source);
    } else if (source->type == varas) {
        msg = "Mummo jäi varkaan kynsiin!";
        level->alertCops(source);
        damage(health);
    }

    return true;
}

bool Mummo::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(50);
    if (source->type == pelaaja) {
        if (!hit) {
            if (source->type == pelaaja) msg = "Et osunut mummoon!";
        } else {
            msg = "Mummo heitti kuperkeikan ja hukkasi käsilaukkunsa.";
            damage(health);
            ++dynamic_cast<Player*>(source)->scoreBoard.mummot;
            level->alertCops(source);

            auto i = dropItem();
            level->addItem(i);
        }
    }
    return hit;
}

bool Mummo::interactpuke(Person* source)
{
    printer->showMessage("Ykäs meni mummon niskoille. Se kuoli.", *level);
    damage(health);
    return true;
}

Item* Mummo::dropItem()
{
    return new Raha(coord, rand()%25 + 1);
}

// COP
Cop::Cop(const Coord& pos) : Person(poliisi, pos)
{
}

bool Cop::move(Direction d, std::string& msg)
{
    Coord check = coord;
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
            p->interact(msg, this);
        } else if (!level->hitBuilding(check)) {
            coord = check;
        }
    }
    common::checkBounds(coord);
    return true;
}

void Cop::npcAct()
{
    string msg = "";

    if (!level->attack)
    {
        Player* p = level->getPlayer();
        if (coord.distance(p->coord) == 1)
        {
            msg = "Pollari varoittaa!";
        }
    } else {
        Player* p = level->getPlayer();
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
    printer->showMessage(msg, *level);
}

bool Cop::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        if (random(50))
        {
            msg = "Huitaiset kyylää!";
        } else {
            msg = "Kyttä kellahtaa ketoon !!!";
            damage(health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    } else if (source->type == varas) {
        msg = "Poliisi pidätti varkaan!";
        source->damage(health);
    }

    return true;
}

bool Cop::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Poliisi kuoli ja pudotti pampun.";
    damage(1);

    if (source->type == pelaaja) {
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }

    level->alertCops(source);

    auto i = dropItem();
    level->addItem(i);
    return true;
}

bool Cop::interactpuke(Person* source)
{
    printer->showMessage("Polsu tukehtui yrjöös, muut pollarit ei taija tykätä susta.", *level);
    level->alertCops(source);
    damage(health);
    return true;
}

Item* Cop::dropItem()
{
    return new Pamppu(coord);
}


// VARAS
Varas::Varas(const Coord& pos) : Person(varas, pos), target(pos)
{
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
            p->interact(msg, this);
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
            } else if (!level->hitBuilding(check)) {
                coord = check;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

bool Varas::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(30);
    if (source->type == pelaaja) {
        if (!hit) {
            msg = "Et osunut varkaaseen.";
        } else {
            if (random(50)) msg = "Varas ulvahtaa kivusta kun osut häntä munille.";
            else msg = "Osuit varkaaseen, joka kupsahti.";
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            damage(health);
        }
    } else {
        if (!hit) {
        } else {
            if (random(50)) msg = "Varas ulvahtaa kivusta kun häntä osuu munille.";
            else msg = "Varkaaseen osui ja se kupsahti.";
            damage(health);
        }
    }
    return hit;
}

void Varas::npcAct()
{
    string msg = "";
    Player* player = level->getPlayer();
    target = player->coord;

    if (coord.distance(target) == 1)
    {
        msg = "Varas hipelöi sinua !!!";

        if (rand()%2)
        {
            uint32_t sum = rand() % 16 + 1;
            if (sum > player->money) sum = player->money;
            player->updateMoney(-sum);
            msg = "Lompakkosi kevenee " + std::to_string(sum) + " markalla.";
            damage(health);
        }
    } else {
        Direction d = getMoveDirection(target);
        move(d, msg);
    }
    printer->showMessage(msg, *level);
}

bool Varas::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        if (random(50))
        {
            msg = "VARAS suuttuu sinulle!";
        } else {
            msg = "VARAS kellahtaa ketoon !!!";
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            damage(health);
        }
    } else if (source->type == poliisi) {
        msg = "Poliisi pidätti varkaan!";
        damage(health);
    }

    return true;
}


// VANKI
Vanki::Vanki(const Coord& pos) : Person(vanki, pos), target(pos)
{
    health = 4;
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
        if (!p && i)
        {
            switch(i->type) {
                case EPaska:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri liukastuu paskaan";
                    i->discard = true;
                    coord = check;
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
                    coord = check;
                    break;
                case EKivi:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri nappaa kiven.";
                    i->discard = true;
                    kivi = true;
                    coord = check;
                    break;
                default:
                    if (!level->hitBuilding(check)) coord = check;
                    break;
            }
        } else if (!level->hitBuilding(check)) {
            coord = check;
        }
    }
    common::checkBounds(coord);
    return true;
}

void Vanki::npcAct()
{
    string msg = "";
    if (firstMove)
    {
        firstMove = false;
        return;
    }

    Player* p = level->getPlayer();
    target = p->coord;

    if (coord.distance(target) == 1)
    {
        msg = "Vankikarkuri kurmottaa sinua!";

        uint32_t damage = rand()%3 + 1;
        p->damage(damage);
    } else {
        if (kivi && ((coord.x == p->coord.x) || (coord.y == p->coord.y)))
        {
            kivi = false;

            {
                int dx = coord.x - p->coord.x;
                int dy = coord.y - p->coord.y;
                dx = (dx < 0) ? 1 : (dx > 0) ? -1 : 0;
                dy = (dy < 0) ? 1 : (dy > 0) ? -1 : 0;

                {
                    Coordinate<int> c { static_cast<int>(dx), static_cast<int>(dy) };

                    // kivi
                    Item* i = Item::createItem(6, coord);
                    i->thrown = true;
                    i->throwVec = c;
                    level->addThrownItem(i);
                    level->actThrow(this);
                }
            }

        } else {
            Direction d = getMoveDirection(target);
            move(d, msg);
        }
    }
    printer->showMessage(msg, *level);
}

bool Vanki::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        msg = "Hakkaat hullun lailla vankikarkuria!";
        damage(1);
    } else if (source->type == poliisi) {
        msg = "Poliisi ottaa vankikarkurin hallintaotteeseen!!!";
        damage(health);
    }

    if (health == 0)
    {
        msg = msg + "\n Vanki kellahtaa ketoon !!!";
        damage(health);

        if (source->type == pelaaja) {
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
    }

    return true;
}

bool Vanki::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(50);
    if (!hit) {
        msg = "Roisto väisti heittosi !";
    } else {
        uint32_t d = rand() % 3 + 1;
        if (d >= health) {
            msg = "Vankikarkuri pokertyy heittosi ansiosta maahan...";
        } else {
            msg = "Osuit vankikarkuriin, mutta hän näyttää entistä vihaisemmalta!";
            if (source->type == pelaaja) {
                ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            }
        }
        damage(d);
    }
    return hit;
}

// SKINHEAD
Skinhead::Skinhead(const Coord& pos) : Person(skinhead, pos)
{
    health = 7;
}

bool Skinhead::move(Direction d, std::string& msg)
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
                msg = "Skinhead uhoaa poliisille!";
            }
        }

        auto i = level->getItem(check);
        if (i)
        {
            switch(i->type) {
                case EPaska:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Skini näkee maassa kikkareen ja päättää maistaa sitä.";
                    damage(1);
                    if (health == 0)
                    {
                        msg += "\nSkinhead tukehtui paskaan.";
                    }
                    break;
                case ELenkki:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Skinin massa kasvaa pilottitakin alla.";
                    ++health;
                    i->discard = true;
                    break;
                default:
                    coord = check;
                    break;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Skinhead::npcAct()
{
    if (firstMove)
    {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();
    Coord target = p->coord;

    if (coord.distance(target) == 1)
    {
        msg = "Skinhead murjoo sinua apinan raivolla!!";

        uint32_t damage = rand()%4 + 1;
        p->damage(damage);
    } else {
        Direction d = getMoveDirection(target);
        move(d, msg);
    }
    printer->showMessage(msg, *level);
}

bool Skinhead::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        msg = "Pusket SHITHEADia naamaan,mutta sehan murjoo myos sua.";
        damage(1);

        if (health == 0)
        {
            msg = "Osuit skiniä kiveksiin !!!";
            damage(health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
    }

    return true;
}

bool Skinhead::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(67);
    if (!hit) {
        msg = "Et osunut skinheadiin !";
    } else {
        uint32_t d = rand() % 5 + 1;
        if (d >= health) {
            msg = "Heittosi ossuupi skinukkelia vyon alle. Se koskee tuntuvasti !!!";
        } else {
            msg = "Osuit skiniin, mutta hän näyttää entistä vihaisemmalta!";
            if (source->type == pelaaja) {
                ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            }
        }
        damage(d);
    }
    return hit;
}

// YKÄ
Yka::Yka(const Coord& pos) : Person(yka, pos)
{
    health = 1;
}

bool Yka::move(Direction d, std::string& msg)
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

    if (!level->hit(check)) {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            if (p->type == pelaaja) {
                bool received = p->interact(msg, this);
                kaljaKesken = received ? 5 : 1;
            } else if (p->type == poliisi) {
                if (!puhallus) {
                    msg = "Ykä utelee poliisilta että saisikohan puhaltaa.";
                    puhallus = true;
                }
            }
        }

        auto i = level->getItem(check);
        if (i)
        {
            switch(i->type) {
                case EKalja:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Ykä löysi maasta kaljan ja lähti lipittämään sitä.";
                    damage(1);
                    break;
                default:
                    coord = check;
                    break;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Yka::npcAct()
{
    string msg = "";

    if (kaljaKesken > 0)
    {
        --kaljaKesken;
        return;
    }

    Player* p = level->getPlayer();
    Coord target = p->coord;

    if (p->inventory.kalja > 0) {
        Direction d = getMoveDirection(target);
        move(d, msg);
    }
    printer->showMessage(msg, *level);
}

bool Yka::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        Player* p = dynamic_cast<Player*>(source);
        if (p->inventory.kalja > 0)
        {
            msg = "Huitaiset ykaa,joka pollii sulta yhen kaljan.";
            --p->inventory.kalja;
            damage(health);
        } else {
            msg = "Mätät Ykää sikana naamaan!";
        }
    }

    return true;
}

bool Yka::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(67);
    if (!hit) {
        msg = "Ykä ihmettelee mikä ihme suhahti pään vierestä !";
        return false;
    } else {
        msg = "Ykä simahtaa !";
        damage(health);
        if (source->type == pelaaja) {
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
    }
    return hit;
}
