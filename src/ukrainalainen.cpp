#include "persons.hpp"
#include "animation.hpp"
#include "common.hpp"
#include "fpv.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include <algorithm>
#include <cstdlib>

using common::random;
using DirectionNS::Direction;
using PlayerNS::Player;
using std::string;

namespace {

constexpr uint32_t RIFLE_RANGE = 70;
constexpr uint32_t AK_RANGE = 8;

// nothing solid between the two, and close enough for an aimed shot
bool clearShot(const Level& l, const Coord& from, const Coord& to, uint32_t range)
{
    return from.distance(to) <= range && l.wallsBetween(from, to) == 0;
}

void muzzleFlash(Level& l, const Coord& at)
{
    l.addAnimation(new Animation(at, "*", 3));
}

} // namespace

Ukrainalainen::Ukrainalainen(const Coord& pos, Mode m) : Person(ukrainalainen, pos), mode(m)
{
    stats.health = 12;
    visionRange = 25;
}

void Ukrainalainen::snipe(Person* target, string& msg)
{
    if (!ammo) return;
    --ammo;
    flash = 2;
    muzzleFlash(*level, coord);
    level->audio.addSound(coord, SoundType::Gunshot,
        "PAM! Kiv\xC3\xA4\xC3\xA4rin laukaus id\xC3\xA4st\xC3\xA4!", "Kiv\xC3\xA4\xC3\xA4rin laukaus!", "...yksitt\xC3\xA4inen laukaus kaukaa...", 70);

    // a sure hand close by, a lucky one far away; half the man shows above a crater's lip
    const uint32_t d = coord.distance(target->coord);
    int chance = 60 - (int)(45 * std::min(d, RIFLE_RANGE) / RIFLE_RANGE);
    if (level->trenchAt(target->coord) == Trench::Crater) chance /= 2;
    const bool isPlayer = target->type == pelaaja;
    if (random(chance)) {
        HurtBy who(level, "ukrainalainen tarkka-ampuja");
        target->damage(2 + std::rand() % 3);
        if (isPlayer) msg = "PAM! Tarkka-ampujan luoti osuu sinuun!";
        else target->say(random(50) ? "Aaargh!" : "Snaiperi!");
    } else if (isPlayer) {
        msg = "Ziuu! Luoti viuhuu ohi!";
    }
}

void Ukrainalainen::burst(Person* target, string& msg)
{
    const uint32_t rounds = 2 + std::rand() % 2;
    uint32_t hurt = 0, fired = 0;
    for (; fired < rounds && ammo > 0; ++fired) {
        --ammo;
        if (random(35)) hurt += 2 + std::rand() % 3;
    }
    if (!fired) return;
    flash = 2;
    muzzleFlash(*level, coord);
    HurtBy who(level, "ukrainalainen jalkav\xC3\xA4kimies");
    if (hurt) target->damage(hurt);
    if (target->type == pelaaja)
        msg = hurt ? "Ukrainalainen ampuu AK-sarjan, osuu sinuun!" : "Ukrainalaisen AK-sarja menee ohi!";
    level->audio.addSound(coord, SoundType::Gunshot,
        "R\xC3\x84K\xC3\x84K\xC3\x84K\xC3\x84! AK-sarja aivan vieress\xC3\xA4!", "Sarjatulta!", "...sarjatulta kaukaa...", 35);
}

void Ukrainalainen::npcAct()
{
    if (firstMove) { firstMove = false; return; }
    if (flash) --flash;
    if (stats.health == 0) return;
    string msg;

    if (mode == Mode::Lurk) {
        // heads down while their own guns pound our line
        if (level->assaultPhase == AssaultPhase::Barrage) { report(msg); return; }
        // shift along the trench now and then, never out of it
        if (random(10)) {
            const Direction d = randomDirection();
            Coord c = coord;
            switch (d) {
                case DirectionNS::up:    --c.y; break;
                case DirectionNS::down:  ++c.y; break;
                case DirectionNS::left:  --c.x; break;
                case DirectionNS::right: ++c.x; break;
                default: break;
            }
            if (level->trenchAt(c) == Trench::Enemy && !level->hit(c)) coord = c;
        }
        // the player first: the longer he has been up there, the readier the rifle
        Player* p = level->getPlayer();
        if (p && !level->playerDead && p->stats.health > 0 && !level->inCover(p->coord) &&
            clearShot(*level, coord, p->coord, RIFLE_RANGE) && random(2 + p->exposure / 4)) {
            snipe(p, msg);
            // he never sees the rifleman, but he knows when a bullet comes for him
            if (!msg.empty() && level->printer) level->printer->showMessage(msg, level, false);
            return;
        }
        // any other Russian soldier caught out in the open
        if (random(3)) {
            for (Person* o : level->persons) {
                if (o == p || !isRussianSoldier(*level, o) || level->inCover(o->coord)) continue;
                if (!clearShot(*level, coord, o->coord, RIFLE_RANGE)) continue;
                snipe(o, msg);
                break;
            }
        }
        report(msg);
        return;
    }

    // the assault: the nearest Russian in sight, the officer above all
    Person* best = nullptr;
    int bestScore = 1 << 30;
    for (Person* o : level->persons) {
        if (!isRussianSoldier(*level, o)) continue;
        const uint32_t d = coord.distance(o->coord);
        if (d > visionRange || !clearShot(*level, coord, o->coord, visionRange)) continue;
        int score = (int)d;
        if (o->type == sotilas && static_cast<Sotilas*>(o)->officer) score -= 10;
        if (score < bestScore) { bestScore = score; best = o; }
    }
    if (best && coord.distance(best->coord) <= AK_RANGE && random(70)) {
        burst(best, msg);
        if (best->type == pelaaja && level->printer) { level->printer->showMessage(msg, level, false); msg.clear(); }
    } else {
        // on towards him, or west, rolling up the trench
        const Coord goal = best ? best->coord : Coord{ 0, coord.y };
        Coord c = coord;
        const Direction d = getMoveDirection(goal);
        switch (d) {
            case DirectionNS::up:    --c.y; break;
            case DirectionNS::down:  ++c.y; break;
            case DirectionNS::left:  --c.x; break;
            case DirectionNS::right: ++c.x; break;
            default: break;
        }
        if (!level->hit(c)) coord = c;
        else if (c.x > 0 && !level->hit(Coord{ coord.x - 1, coord.y })) --coord.x;
        if (random(8)) say(random(50) ? "Slava Ukraini!" : "Vpered!");
    }
    report(msg);
}

bool Ukrainalainen::interact(string& msg, Person* source)
{
    if (source->type != pelaaja) return false;
    damage(2 + std::rand() % 3);
    msg = stats.health ? "Lyöt ukrainalaista kiv\xC3\xA4\xC3\xA4rinper\xC3\xA4ll\xC3\xA4!" : "Ukrainalainen kaatuu.";
    return true;
}

bool Ukrainalainen::interactThrow(Item* item, Person* source, string& msg)
{
    damage(1 + std::rand() % 2);
    msg = "Osuit ukrainalaiseen!";
    return true;
}
