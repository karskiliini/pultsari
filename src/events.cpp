#include "events.hpp"
#include "level.hpp"
#include "printer.hpp"
#include "player.hpp"
#include "building.hpp"
#include "animation.hpp"
#include "common.hpp"
#include <cstdlib>

using common::random;

EnvironmentEvents::EnvironmentEvents(Level* level) : level(level)
{
    droneCooldown = 10 + std::rand()%20;
    aaCooldown = 5 + std::rand()%15;
    droneNoiseCooldown = 3 + std::rand()%8;
}

void EnvironmentEvents::update()
{
    droneSound();
    droneStrike();
    aaFire();
    updateFires();
    fireDamage();

    if (random(5)) {
        spreadFire();
    }
}

bool EnvironmentEvents::hasFire(const Coord& c) const
{
    for (const auto& f : fires) {
        if (f.coord == c) return true;
    }
    return false;
}

void EnvironmentEvents::droneSound()
{
    if (droneNoiseCooldown > 0) {
        --droneNoiseCooldown;
        return;
    }
    droneNoiseCooldown = 8 + std::rand()%15;

    if (random(30)) {
        level->printer->showMessage("Kuulet droonin äänen taivaalta...", level, false);
    }
}

void EnvironmentEvents::droneStrike()
{
    if (droneCooldown > 0) {
        --droneCooldown;
        return;
    }
    droneCooldown = 15 + std::rand()%25;

    if (!random(20)) return;

    // pick a random building to hit
    if (level->buildings.empty()) return;
    uint32_t idx = std::rand() % level->buildings.size();
    Building* target = level->buildings[idx];

    uint32_t cx = (target->wallLeft + target->wallRight) / 2;
    uint32_t cy = (target->wallTop + target->wallBot) / 2;
    Coord strikeCoord { cx, cy };

    level->printer->showMessage("DRONE-ISKU! " + target->getName() + " saa osuman!!", level, false);

    Animation* anim = new Animation(strikeCoord, "===BOOM===", 10);
    level->addAnimation(anim);

    // damage persons near the strike
    for (auto& p : level->persons) {
        if (p->stats.health > 0 && p->coord.distance(strikeCoord) <= 4) {
            uint32_t dmg = std::rand()%6 + 2;
            p->damage(dmg);
        }
    }

    // start fires around the building
    for (int i = 0; i < 3 + std::rand()%3; ++i) {
        int fx = (int)cx + (std::rand()%7 - 3);
        int fy = (int)cy + (std::rand()%5 - 2);
        if (fx < 0 || fy < 0 || fx >= (int)common::SIZEX || fy >= (int)common::SIZEY)
            continue;
        Coord fc { (uint32_t)fx, (uint32_t)fy };
        if (!hasFire(fc)) {
            fires.push_back(Fire{ fc, 20 + (uint32_t)(std::rand()%15), 15 });
        }
    }

    // NPCs panic from explosions
    for (auto& p : level->persons) {
        if (p->stats.health > 0 && p->coord.distance(strikeCoord) < 10) {
            p->alerted = true;
            p->lastKnownPos = strikeCoord;
            p->alertCooldown = 8;
        }
    }
}

void EnvironmentEvents::aaFire()
{
    if (aaCooldown > 0) {
        --aaCooldown;
        return;
    }
    aaCooldown = 8 + std::rand()%12;

    if (!random(25)) return;

    // AA fires from a random edge
    uint32_t sx, sy;
    if (random(50)) {
        sx = std::rand() % common::SIZEX;
        sy = (random(50)) ? 0 : common::SIZEY - 1;
    } else {
        sx = (random(50)) ? 0 : common::SIZEX - 1;
        sy = std::rand() % common::SIZEY;
    }
    Coord aaCoord { sx, sy };

    level->printer->showMessage("Ilmatorjunta ampuu!!", level, false);

    Animation* anim = new Animation(aaCoord, "***", 5);
    anim->delay = 40;
    level->addAnimation(anim);

    // damage anyone near AA position
    for (auto& p : level->persons) {
        if (p->stats.health > 0 && p->coord.distance(aaCoord) <= 2) {
            uint32_t dmg = std::rand()%4 + 1;
            p->damage(dmg);
            if (p->type == pelaaja) {
                level->printer->showMessage("Ilmatorjunnan sirpale osui sinuun!", level, false);
            }
        }
    }

    // random debris lands elsewhere
    uint32_t debrisX = std::rand() % common::SIZEX;
    uint32_t debrisY = std::rand() % common::SIZEY;
    Coord debrisCoord { debrisX, debrisY };
    for (auto& p : level->persons) {
        if (p->stats.health > 0 && p->coord.distance(debrisCoord) <= 1) {
            p->damage(std::rand()%3 + 1);
            if (p->type == pelaaja) {
                level->printer->showMessage("Taivaalta putoaa sirpaleita!", level, false);
            }
        }
    }
}

void EnvironmentEvents::updateFires()
{
    auto it = fires.begin();
    while (it != fires.end()) {
        if (it->ttl > 0) {
            --it->ttl;
            ++it;
        } else {
            it = fires.erase(it);
        }
    }
}

void EnvironmentEvents::spreadFire()
{
    if (fires.empty()) return;

    std::vector<Fire> newFires;
    for (const auto& f : fires) {
        if (f.ttl < 5) continue;
        if (!random(f.spreadChance)) continue;

        int dx = std::rand()%3 - 1;
        int dy = std::rand()%3 - 1;
        int nx = (int)f.coord.x + dx;
        int ny = (int)f.coord.y + dy;
        if (nx < 0 || ny < 0 || nx >= (int)common::SIZEX || ny >= (int)common::SIZEY)
            continue;
        Coord nc { (uint32_t)nx, (uint32_t)ny };
        if (!hasFire(nc)) {
            newFires.push_back(Fire{ nc, 10 + (uint32_t)(std::rand()%10), 8 });
        }
    }

    for (auto& nf : newFires) {
        fires.push_back(nf);
    }
}

void EnvironmentEvents::fireDamage()
{
    for (const auto& f : fires) {
        for (auto& p : level->persons) {
            if (p->stats.health > 0 && p->coord == f.coord) {
                p->damage(1);
                if (p->type == pelaaja) {
                    level->printer->showMessage("Palat tulessa!", level, false);
                }
            }
        }
    }
}
