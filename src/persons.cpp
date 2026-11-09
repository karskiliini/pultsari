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
    stats.health += damage;

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
    if (damage >= stats.health) {
        if (this == level->attack) level->attack = nullptr;
        stats.health = 0;
    } else {
        stats.health -= damage;
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

bool Person::canSeeTarget(const Coord& target) const
{
    if (visionRange == 0) return true;
    if (coord.distance(target) > visionRange) return false;
    return level->raycast(coord, target);
}

Direction Person::randomDirection() const
{
    switch (std::rand() % 4) {
        case 0: return DirectionNS::up;
        case 1: return DirectionNS::down;
        case 2: return DirectionNS::left;
        default: return DirectionNS::right;
    }
}

void Person::say(const std::string& text)
{
    uint32_t bx = (coord.x > text.length()/2) ? coord.x - text.length()/2 : 0;
    uint32_t by = (coord.y > 1) ? coord.y - 1 : 0;
    Coord bubblePos { bx, by };
    Animation* anim = new Animation(bubblePos, text, 12);
    anim->isBubble = true;
    anim->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(anim);
}

Direction Person::getMoveDirection(const Coord& target) const
{
    if (pathFinding) {
        // use advanced pathfinding
        PathNS::PathMask mask(level);
        pathmaskHook(&mask);
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
    stats.health = 3;
    visionRange = 6;
}

bool Mummo::move(Direction d, std::string& msg)
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
    }
    common::checkBounds(coord);
    return true;
}

void Mummo::npcAct()
{
    Player* p = level->getPlayer();
    if (!p) return;

    // if player returned from war (wearing SMO uniform) and mummo is close
    if (p->returnedFromWar && !reactedToUniform && canSeeTarget(p->coord) &&
        coord.distance(p->coord) <= 3) {
        reactedToUniform = true;
        if (random(50)) {
            say("Kiitos palveluksestasi, poikaseni!");
            p->inventory.vodka++;
            p->updatepromilles(3);
        } else {
            say("MURHAAJA! Tapoit poikani!");
            p->damage(std::rand()%3 + 2);
        }
        return;
    }

    // slow random walk
    if (random(10)) {
        string msg = "";
        Direction d = randomDirection();
        move(d, msg);
    }
}

bool Mummo::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        if (random(50))
        {
            msg = "Eläkellääinen hakkaa sua käsilaukullaan !!!";
            uint32_t damage = std::rand() % 1 + 1;
            source->damage(damage);
        } else {
            msg = "Mukiloit mummon kumoon. Haa,lompsa !!!";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.mummot;
        }

        level->alertCops(source);
    } else if (source->type == varas) {
        msg = "Mummo jäi varkaan kynsiin!";
        level->alertCops(source);
        damage(stats.health);
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
            damage(stats.health);
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
    printer->showMessage("Ykäs meni mummon niskoille. Se kuoli.", level);
    damage(stats.health);
    return true;
}

Item* Mummo::dropItem()
{
    return new Raha(coord, std::rand()%25 + 1);
}

// COP
Cop::Cop(const Coord& pos) : Person(poliisi, pos)
{
    visionRange = 10;
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

void Cop::pathmaskHook(PathNS::PathMask* pathmask) const
{
    pathmask->skipConvict = true;
    pathmask->skipVaras = true;
}

void Cop::npcAct()
{
    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (!level->attack)
    {
        if (seesPlayer && coord.distance(p->coord) == 1)
        {
            {
                int roll = std::rand() % 100;
                if (roll < 30) {
                    // corrupt cop demands bribe
                    static const char* bribes[] = {
                        "Maksa tai rintamalle!",
                        "Katutullia pitää maksaa!",
                        "'Vapaaehtoinen' lahjoitus armeijalle?",
                        "Sinulla on epäilyttävä olemus. Maksa.",
                    };
                    say(bribes[std::rand()%4]);
                    if (p->stats.money >= 10) {
                        uint32_t bribe = std::min(p->stats.money, (uint32_t)(rand()%20 + 10));
                        p->updateMoney(-(int)bribe);
                        say(std::to_string(bribe) + " ruplaa. Jatka matkaa.");
                    } else {
                        say("Ei rahaa? Rintamalle!");
                        p->inJail = true;
                    }
                } else if (roll < 50 && p->stats.promilles > 15) {
                    // stupid fine for being drunk
                    static const char* fines[] = {
                        "Sakko: julkijuopumus!",
                        "Sakko: häiriökäyttäytyminen!",
                        "Sakko: kävely ilman passia!",
                        "Sakko: ulkonaliikkumiskiellon rikkominen!",
                    };
                    say(fines[std::rand()%4]);
                    if (p->stats.money >= 5) {
                        uint32_t fine = std::min(p->stats.money, (uint32_t)(rand()%15 + 5));
                        p->updateMoney(-(int)fine);
                    } else {
                        say("Ei rahaa sakkoihin? Putkaan!");
                        p->inJail = true;
                    }
                } else if (roll < 60 && p->stats.promilles > 25) {
                    // cart drunk to the front
                    say("Juoppo! Tarvitaan miehiä rintamalle!");
                    p->inJail = true;
                } else {
                    static const char* idle[] = {
                        "Liiku eteenpäin!",
                        "Älä jää tuijottamaan!",
                        "Täällä ei ole mitään nähtävää!",
                        "Paperit kuntoon!",
                    };
                    say(idle[std::rand()%4]);
                }
            }
        } else if (seesPlayer && coord.distance(p->coord) <= 5) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        } else {
            // patrol - cops wander actively looking for trouble
            if (random(60)) {
                Direction d = randomDirection();
                move(d, msg);
            }
        }
    } else {
        Person* target = level->attack;
        bool seesTarget = (target == p) ? seesPlayer : canSeeTarget(target->coord);

        if (seesTarget) {
            alerted = true;
            lastKnownPos = target->coord;
            alertCooldown = 15;
        }

        if (alerted) {
            Coord chasePos = seesTarget ? target->coord : lastKnownPos;

            if (seesTarget && coord.distance(target->coord) == 1)
            {
                if (target == p) {
                    say("Pamppu maistuu!");
                    p->damage(rand()%2+1);
                } else {
                    msg = "";
                }
            } else {
                Direction d = getMoveDirection(chasePos);
                move(d, msg);
            }

            if (!seesTarget && coord.distance(lastKnownPos) <= 1) {
                if (alertCooldown > 0) {
                    --alertCooldown;
                } else {
                    alerted = false;
                }
            }
        } else {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
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
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    } else if (source->type == varas) {
        msg = "Poliisi pidätti varkaan!";
        source->damage(stats.health);
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
    printer->showMessage("Polsu tukehtui yrjöös, muut pollarit ei taija tykätä susta.", level);
    level->alertCops(source);
    damage(stats.health);
    return true;
}

Item* Cop::dropItem()
{
    return new Pamppu(coord);
}


// VARAS
Varas::Varas(const Coord& pos) : Person(varas, pos), target(pos)
{
    visionRange = 8;
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
                damage(stats.health);
            } else if (!level->hitBuilding(check)) {
                coord = check;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Varas::pathmaskHook(PathNS::PathMask* pathmask) const
{
    pathmask->skipCop = true;
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
            damage(stats.health);
        }
    } else {
        if (!hit) {
        } else {
            if (random(50)) msg = "Varas ulvahtaa kivusta kun häntä osuu munille.";
            else msg = "Varkaaseen osui ja se kupsahti.";
            damage(stats.health);
        }
    }
    return hit;
}

void Varas::npcAct()
{
    string msg = "";
    Player* player = level->getPlayer();
    bool seesPlayer = canSeeTarget(player->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = player->coord;
        alertCooldown = 10;
        target = player->coord;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? player->coord : lastKnownPos;

        if (seesPlayer && coord.distance(player->coord) == 1)
        {
            msg = "Varas hipelöi sinua !!!";

            auto r = std::rand();
            if (r % 2)
            {
                uint32_t sum = r % 16 + 1;
                if (sum > player->stats.money) sum = player->stats.money;
                player->updateMoney(-sum);
                msg = "Lompakkosi kevenee " + std::to_string(sum) + " markalla.";
                damage(stats.health);
            }
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(30)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
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
            damage(stats.health);
        }
    } else if (source->type == poliisi) {
        msg = "Poliisi pidätti varkaan!";
        damage(stats.health);
    }

    return true;
}


// VANKI
Vanki::Vanki(const Coord& pos) : Person(vanki, pos), target(pos)
{
    stats.health = 4;
    visionRange = 8;
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
                damage(stats.health);
            } else if (p->type == mummo) {
                msg = "Vankikarkuri tönäisee muorin nurin!";
                p->damage(p->stats.health);
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
                    if (stats.health == 0)
                    {
                        msg += "\nja vankikarkuri kuoli.";
                    }
                    break;
                case ELenkki:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri ahmii kiekuralenkin ja voimistuu";
                    ++stats.health;
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

void Vanki::pathmaskHook(PathNS::PathMask* pathmask) const
{
    pathmask->skipCop = true;
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
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 10;
        target = p->coord;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1)
        {
            msg = "Vankikarkuri kurmottaa sinua!";
            uint32_t damage = std::rand()%3 + 1;
            p->damage(damage);
        } else if (seesPlayer && kivi && ((coord.x == p->coord.x) || (coord.y == p->coord.y))) {
            kivi = false;
            int dx = coord.x - p->coord.x;
            int dy = coord.y - p->coord.y;
            dx = (dx < 0) ? 1 : (dx > 0) ? -1 : 0;
            dy = (dy < 0) ? 1 : (dy > 0) ? -1 : 0;

            Coordinate<int> c { static_cast<int>(dx), static_cast<int>(dy) };
            Item* i = Item::createItem(6, coord);
            i->thrown = true;
            i->throwVec = c;
            level->addThrownItem(i);
            level->actThrow(this);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(25)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
}

bool Vanki::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        msg = "Hakkaat hullun lailla vankikarkuria!";
        damage(1);
    } else if (source->type == poliisi) {
        msg = "Poliisi ottaa vankikarkurin hallintaotteeseen!!!";
        damage(stats.health);
    }

    if (stats.health == 0)
    {
        msg = msg + "\n Vanki kellahtaa ketoon !!!";
        damage(stats.health);

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
        uint32_t d = std::rand() % 3 + 1;
        if (d >= stats.health) {
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
    stats.health = 7;
    visionRange = 7;
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
                    if (stats.health == 0)
                    {
                        msg += "\nSkinhead tukehtui paskaan.";
                    }
                    break;
                case ELenkki:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Skinin massa kasvaa pilottitakin alla.";
                    ++stats.health;
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
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 12;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1)
        {
            msg = "Skinhead murjoo sinua apinan raivolla!!";
            uint32_t damage = std::rand()%4 + 1;
            p->damage(damage);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(25)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
}

bool Skinhead::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        msg = "Pusket SHITHEADia naamaan,mutta sehan murjoo myos sua.";
        damage(1);

        if (stats.health == 0)
        {
            msg = "Osuit skiniä kiveksiin !!!";
            damage(stats.health);
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
        uint32_t d = std::rand() % 5 + 1;
        if (d >= stats.health) {
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
    stats.health = 1;
    visionRange = 6;
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
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer && p->inventory.kalja > 0) {
        Direction d = getMoveDirection(p->coord);
        move(d, msg);
    } else {
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
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
            damage(stats.health);
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
        damage(stats.health);
        if (source->type == pelaaja) {
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
    }
    return hit;
}


// JENGI (Gang member)
Jengi::Jengi(const Coord& pos) : Person(jengi, pos)
{
    stats.health = 5;
    visionRange = 8;
}

bool Jengi::move(Direction d, std::string& msg)
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
        if (p && p->type == poliisi) {
            msg = "Jengiläinen ja poliisi taistelevat!";
        }
    }
    common::checkBounds(coord);
    return true;
}

void Jengi::npcAct()
{
    if (firstMove) { firstMove = false; return; }

    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 8;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1) {
            auto r = std::rand() % 3;
            if (r == 0) {
                uint32_t sum = std::rand() % 20 + 5;
                if (sum > p->stats.money) sum = p->stats.money;
                p->updateMoney(-(int)sum);
                msg = "Jengiläinen ryöstää " + std::to_string(sum) + " markkaa!";
            } else if (r == 1) {
                uint32_t dmg = std::rand() % 3 + 1;
                p->damage(dmg);
                msg = "Jengiläinen lyö sinua nyrkillä!";
            } else {
                msg = "Jengiläinen uhkailee sinua.";
            }
            level->alertCops(this);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(30)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
}

bool Jengi::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (random(40)) {
            msg = "Jengiläinen iskee takaisin!";
            source->damage(std::rand()%2+1);
        } else {
            msg = "Moukaroit jengiläisen tajuttomaksi!";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    }
    return true;
}

bool Jengi::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(50);
    if (!hit) {
        msg = "Jengiläinen väisti!";
    } else {
        msg = "Jengiläinen sai osuman ja kaatuu!";
        damage(stats.health);
        if (source->type == pelaaja)
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return hit;
}


// RYÖSTÄJÄ (Armed robber)
Ryostaja::Ryostaja(const Coord& pos) : Person(ryostaja, pos)
{
    stats.health = 3;
    visionRange = 9;
}

bool Ryostaja::move(Direction d, std::string& msg)
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
        if (p && p->type == poliisi) {
            msg = "Ryöstäjä ampuu poliisia!";
            p->damage(p->stats.health);
            level->alertCops(this);
        }
    }
    common::checkBounds(coord);
    return true;
}

void Ryostaja::npcAct()
{
    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 12;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1) {
            if (armed) {
                msg = "Ryöstäjä osoittaa sinua aseella: RAHAT TAI HENKI!";
                if (p->stats.money > 0) {
                    uint32_t take = std::min(p->stats.money, (uint32_t)(rand()%30 + 10));
                    p->updateMoney(-(int)take);
                    msg = "Ryöstäjä vie " + std::to_string(take) + " markkaa aseella uhaten!";
                } else {
                    p->damage(std::rand()%3+2);
                    msg = "Ryöstäjä ampuu sinua kun ei ole rahaa!";
                }
                level->alertCops(this);
            } else {
                p->damage(std::rand()%2+1);
                msg = "Ryöstäjä lyö sinua!";
            }
        } else if (seesPlayer && coord.distance(p->coord) <= 4 && armed && random(20)) {
            p->damage(std::rand()%2+1);
            msg = "Ryöstäjä ampuu sinua kaukaa!";
            level->alertCops(this);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
}

bool Ryostaja::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (random(60)) {
            msg = "Ryöstäjä ampuu sinua!";
            source->damage(std::rand()%3+1);
            armed = false;
        } else {
            msg = "Kaadoit ryöstäjän! Ase putoaa maahan.";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    }
    return true;
}

bool Ryostaja::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(40);
    if (!hit) {
        msg = "Ryöstäjä väistää heiton!";
    } else {
        msg = "Ryöstäjä kaatuu ja pudottaa aseensa!";
        damage(stats.health);
        auto i = dropItem();
        level->addItem(i);
        if (source->type == pelaaja)
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return hit;
}

Item* Ryostaja::dropItem()
{
    return new Raha(coord, std::rand()%30 + 10);
}


// DIILERI (Drug dealer)
Diileri::Diileri(const Coord& pos) : Person(diileri, pos)
{
    stats.health = 2;
    visionRange = 7;
}

bool Diileri::move(Direction d, std::string& msg)
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
    }
    common::checkBounds(coord);
    return true;
}

void Diileri::npcAct()
{
    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer && coord.distance(p->coord) == 1 && !offered) {
        offered = true;
        if (common::realtimeEnabled) {
            if (p->stats.money >= 10 && random(50)) {
                p->updateMoney(-10);
                if (random(30)) {
                    msg = "Diileri: Kokeile tätä... Pääsi pyörii!";
                    p->updatepromilles(8);
                    p->damage(1);
                } else {
                    msg = "Diileri: Tässä on hyvää kamaa! Olet energinen!";
                    p->powerup(3);
                    p->stats.hunger += 20;
                    if (p->stats.hunger > 100) p->stats.hunger = 100;
                }
            } else {
                msg = "Diileri: Hei, haluatko ostaa jotain?";
            }
        } else {
            msg = "Diileri: Hei, haluatko ostaa jotain?";
        }
    } else if (seesPlayer && coord.distance(p->coord) <= 3) {
        if (random(40)) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        }
    } else {
        offered = false;
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
}

bool Diileri::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (random(70)) {
            msg = "Diileri karkaa juosten!";
            damage(stats.health);
        } else {
            msg = "Löylyytät diilerin ja löydät rahaa!";
            damage(stats.health);
            dynamic_cast<Player*>(source)->updateMoney(std::rand()%20+5);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    }
    return true;
}

bool Diileri::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Diileri kaatuu ja karkaa!";
    damage(stats.health);
    if (source->type == pelaaja)
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    return true;
}

// VETERAANI (SMO Veteran)
Veteraani::Veteraani(const Coord& pos) : Person(veteraani, pos)
{
    stats.health = 12;
    visionRange = 10;
}

std::string Veteraani::typeToChar() const
{
    return aggressive ? "W" : "Z";
}

bool Veteraani::move(Direction d, std::string& msg)
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
        if (p && aggressive)
        {
            if (p->type == pelaaja) {
                msg = "Veteraani ampuu sinua!";
                p->damage(std::rand()%5 + 3);
            } else if (p->type != veteraani) {
                msg = "Veteraani raivostuu ja hyökkää!";
                p->damage(std::rand()%6 + 2);
            }
        }
    }
    common::checkBounds(coord);
    return true;
}

void Veteraani::npcAct()
{
    if (firstMove)
    {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    // PTSD trigger: random chance to become aggressive each turn
    if (!aggressive) {
        bool trigger = false;
        // loud noises nearby (thrown items, fights) can trigger
        if (level->attack && coord.distance(level->attack->coord) < 6) {
            trigger = random(60);
        }
        // small random chance
        if (!trigger) trigger = random(3);

        if (trigger) {
            aggressive = true;
            rageCooldown = 15 + std::rand()%10;
            say("AAARRGH!! TANKIT!! DROONIT!!");
            msg = "Veteraani saa PTSD-kohtauksen ja raivostuu!!";

            // nearby NPCs flee
            for (auto& person : level->persons) {
                if (person != this && person->type != pelaaja &&
                    person->stats.health > 0 &&
                    coord.distance(person->coord) < 8) {
                    person->alerted = true;
                    person->lastKnownPos = coord;
                    person->alertCooldown = 10;
                }
            }
        }
    }

    if (aggressive) {
        if (rageCooldown > 0) --rageCooldown;
        else {
            aggressive = false;
            msg = "Veteraani rauhoittuu...";
            printer->showMessage(msg, level);
            return;
        }

        // grenade: rare, area damage
        if (grenades > 0 && random(8)) {
            --grenades;
            msg = "Veteraani heittää KRANAATIN!!";
            // damage everyone within radius 3
            for (auto& person : level->persons) {
                if (person != this && person->stats.health > 0 &&
                    coord.distance(person->coord) <= 3) {
                    uint32_t dmg = std::rand()%8 + 4;
                    person->damage(dmg);
                }
            }
            Animation* anim = new Animation(coord, "***BOOM***", 8);
            level->addAnimation(anim);
        }
        // shoot at nearest visible target
        else if (ammo > 0 && seesPlayer && coord.distance(p->coord) <= 8) {
            --ammo;
            if (random(60)) {
                msg = "Veteraani ampuu sinua!";
                p->damage(std::rand()%4 + 2);
            } else {
                msg = "Veteraani ampuu mutta menee ohi!";
            }
        }
        // chase nearest person
        else if (seesPlayer && coord.distance(p->coord) <= visionRange) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        } else {
            // wander aggressively
            if (random(50)) {
                Direction d = randomDirection();
                move(d, msg);
            }
        }
    } else {
        // peaceful mode: wander slowly
        if (random(15)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    printer->showMessage(msg, level);
}

bool Veteraani::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (!aggressive) {
            // attacking a peaceful veteran triggers rage
            aggressive = true;
            rageCooldown = 20;
            msg = "Veteraani raivostuu hyökkäyksestä!!";
            source->damage(std::rand()%3 + 1);
            level->alertCops(source);

            for (auto& person : level->persons) {
                if (person != this && person->type != pelaaja &&
                    person->stats.health > 0 &&
                    coord.distance(person->coord) < 8) {
                    person->alerted = true;
                    person->lastKnownPos = coord;
                    person->alertCooldown = 10;
                }
            }
        } else {
            msg = "Veteraani murskaa sinut!";
            source->damage(std::rand()%6 + 3);
        }
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return true;
}

bool Veteraani::interactThrow(Item* item, Person* source, std::string& msg)
{
    if (!aggressive) {
        aggressive = true;
        rageCooldown = 20;
        msg = "Veteraani raivostuu heitosta!!";
    } else {
        msg = "Veteraani ei edes huomaa.";
    }
    damage(1);
    if (source->type == pelaaja)
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    return true;
}

// ============ Juoppo (drunk) ============

Juoppo::Juoppo(const Coord& pos) : Person(juoppo, pos)
{
    stats.health = 4;
    visionRange = 3;
}

bool Juoppo::move(Direction d, std::string& msg)
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
    }
    common::checkBounds(coord);
    return true;
}

void Juoppo::npcAct()
{
    string msg = "";

    // stumble around drunkenly
    if (random(40)) {
        Direction d = randomDirection();
        move(d, msg);
        ++stumbleCount;
    }

    // occasionally say something
    if (random(5)) {
        static const char* drunk_lines[] = {
            "Hic!",
            "*RÖYH*",
            "Vodkaa...",
            "Missä on ALKO?",
            "Blyat...",
            "Elämä on kaunista...",
            "*horskaa*",
            "Putin on... *hic*",
            "Ei ole kylmä...",
            "Missä minä olen?",
        };
        say(drunk_lines[std::rand()%10]);
    }

    // occasionally vomit
    if (stumbleCount > 10 && random(5)) {
        say("*YÖÖK*");
        stumbleCount = 0;
    }
}

bool Juoppo::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (random(50)) {
            msg = "Juoppo tarjoaa ryyppyä!";
            dynamic_cast<Player*>(source)->updatepromilles(2);
        } else {
            msg = "Juoppo kaatuu päällesi!";
            source->damage(1);
        }
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return true;
}

bool Juoppo::interactThrow(Item* item, Person* source, std::string& msg)
{
    say("Hei!");
    damage(2);
    if (source->type == pelaaja)
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    return true;
}

// ============ FSB Agent ============

FSBAgent::FSBAgent(const Coord& pos) : Person(fsb, pos)
{
    stats.health = 20;
    visionRange = 12;
}

bool FSBAgent::move(Direction d, std::string& msg)
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
        if (p && p->type != fsb && p->type != poliisi) {
            msg = "FSB-agentti tarttuu kiinni!";
            p->damage(std::rand()%3 + 2);
        }
    }
    common::checkBounds(coord);
    return true;
}

void FSBAgent::npcAct()
{
    if (firstMove) {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();

    // defenestration event: randomly pick an NPC to throw out a window
    if (defenCooldown > 0) --defenCooldown;

    if (!hasTarget && defenCooldown == 0 && random(4)) {
        for (auto& person : level->persons) {
            if (person != this && person->type != pelaaja &&
                person->type != poliisi && person->type != fsb &&
                person->stats.health > 0 && coord.distance(person->coord) < 3) {
                hasTarget = true;
                defenTarget = person->coord;
                msg = "FSB-agentit tarttuvat " + person->typeToChar() + " kiinni!";
                person->damage(50);
                printer->showMessage("DEFENESTRAATIO! Joku lensi ikkunasta!", level);

                Animation* anim = new Animation(person->coord, "AAAH!", 8);
                anim->coordMove = Coordinate<int>(0, 1);
                level->addAnimation(anim);

                defenCooldown = 30;
                hasTarget = false;
                break;
            }
        }
    }

    bool seesPlayer = canSeeTarget(p->coord);

    // if player is nearby, FSB gets suspicious and chases
    if (seesPlayer && coord.distance(p->coord) <= visionRange) {
        if (coord.distance(p->coord) <= 1) {
            say("Rintamalle, svinya!");
            p->damage(3);
            dynamic_cast<Player*>(p)->inJail = true;
        } else if (random(60)) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        }
    } else {
        // patrol: slow movement
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }

    printer->showMessage(msg, level);
}

bool FSBAgent::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        msg = "FSB-agentti väistää iskusi ja lyö takaisin!";
        source->damage(std::rand()%5 + 3);
        dynamic_cast<Player*>(source)->inJail = true;
    }
    return true;
}

bool FSBAgent::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "FSB-agentti ei reagoi heittoosi.";
    damage(1);
    return true;
}

Item* Veteraani::dropItem()
{
    if (grenades > 0) {
        return new Granata(coord);
    }
    return new Raha(coord, std::rand()%50 + 20);
}

// ============ Sotilas (Soldier) ============

Sotilas::Sotilas(const Coord& pos) : Person(sotilas, pos)
{
    stats.health = 15;
    visionRange = 11;
}

bool Sotilas::move(Direction d, std::string& msg)
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
        if (p && p->type != sotilas && p->type != poliisi && p->type != fsb) {
            msg = "Sotilas tönäisee tieltään!";
            p->damage(std::rand()%2 + 1);
        }
    }
    common::checkBounds(coord);
    return true;
}

void Sotilas::npcAct()
{
    if (firstMove) {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer && coord.distance(p->coord) <= visionRange) {
        if (coord.distance(p->coord) <= 1) {
            say("Rintamalle, svinya!");
            p->damage(2);
            dynamic_cast<Player*>(p)->inJail = true;
        } else if (ammo > 0 && coord.distance(p->coord) <= 6 && random(30)) {
            --ammo;
            uint32_t dmg = std::rand()%3 + 3;
            if (random(60)) {
                p->damage(dmg);
                msg = "Sotilas ampuu sinua!";
            } else {
                msg = "Sotilas ampuu mutta menee ohi!";
            }
            level->alertCops(this);
        } else {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        }
    } else {
        if (random(50)) {
            Direction d = randomDirection();
            move(d, msg);
        }
        if (random(5)) {
            static const char* lines[] = {
                "Stoi!",
                "Paperit!",
                "Käänny takaisin!",
                "Alue kielletty!",
            };
            say(lines[std::rand()%4]);
        }
    }

    printer->showMessage(msg, level);
}

bool Sotilas::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        msg = "Sotilas iskee takaisin kiväärin perällä!";
        uint32_t dmg = std::rand()%4 + 4;
        source->damage(dmg);
        dynamic_cast<Player*>(source)->inJail = true;
        level->alertCops(source);
    }
    return true;
}

bool Sotilas::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Sotilas väistää ja ampuu!";
    damage(1);
    if (source->type == pelaaja) {
        source->damage(std::rand()%3 + 2);
        dynamic_cast<Player*>(source)->inJail = true;
    }
    return true;
}
