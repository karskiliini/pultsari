#include "fpv.hpp"
#include "animation.hpp"
#include "building.hpp"
#include "common.hpp"
#include "events.hpp"
#include "level.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "printer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

using common::random;

bool isRussianSoldier(const Level& level, const Person* p)
{
    if (!p || p->stats.health == 0) return false;
    switch (p->type) {
        case pelaaja:
            return level.isWarLevel || static_cast<const PlayerNS::Player*>(p)->conscripted;
        case skinhead:      // at the front they are the conscripts of the meat assaults
            return level.isWarLevel;
        case sotilas:
        case ilmatorjunta:
        case veteraani:
        case korealainen:
        case rekryytti:
            return true;
        default:
            return false;
    }
}

static bool underOpenSky(const Level& level, const Coord& c)
{
    if (const Building* b = level.buildingAt(c)) return b->hitWall(c);
    return true;
}

static bool fairGame(const Level& level, const Person* p)
{
    return isRussianSoldier(level, p) && underOpenSky(level, p->coord);
}

// the prey is still a person on this level, alive and out in the open
static bool stillHunted(const Level& level, const Person* prey)
{
    if (!prey) return false;
    if (std::find(level.persons.begin(), level.persons.end(), prey) == level.persons.end()) return false;
    return fairGame(level, prey);
}

Person* fpvPickPrey(const Level& level, float x, float y)
{
    Person* best = nullptr;
    float bestScore = 1e9f;
    for (auto p : level.persons) {
        if (!fairGame(level, p)) continue;
        // the nearest, give or take: not every pilot picks the same man
        const float score = std::hypot(p->coord.x - x, p->coord.y - y) + (std::rand() % 6);
        if (score < bestScore) { bestScore = score; best = p; }
    }
    return best;
}

void launchFpv(Level& level, float x, float y, Person* prey)
{
    Drone d { x, y, FPV_SPEED, 0.0f };
    d.fpv = true;
    d.prey = prey ? prey : fpvPickPrey(level, x, y);
    if (d.prey) {
        const float off = ((std::rand() % 81) - 40) / 100.0f;
        const float h = std::atan2(d.prey->coord.y - y, d.prey->coord.x - x) + off;
        d.vx = std::cos(h) * FPV_SPEED;
        d.vy = std::sin(h) * FPV_SPEED;
    }
    level.drones.push_back(d);
}

void fpvTick(Level& level)
{
    if (!level.isWarLevel) return;

    uint32_t flying = 0;
    for (const auto& d : level.drones) {
        if (!d.fpv) continue;
        ++flying;
        level.audio.addSound(Coord{ (uint32_t)std::max(0.0f, d.x), (uint32_t)std::max(0.0f, d.y) }, SoundType::Drone,
            "Ziiiiiing! FPV-drooni ulisee aivan vieress\xC3\xA4!", "FPV-droonin ulinaa", "...kaukaista ininää...", 30);
    }
    if (flying >= FPV_MAX || !random(12)) return;   // a new one every few seconds

    // the pilots go for whoever is out in the open, the drunk new recruit as much as anyone
    std::vector<Person*> open;
    for (auto p : level.persons) if (fairGame(level, p)) open.push_back(p);
    if (open.empty()) return;
    Person* prey = open[std::rand() % open.size()];
    const PlayerNS::Player* player = level.getPlayer();
    if (player && !level.playerDead && fairGame(level, player) && random(25)) prey = const_cast<PlayerNS::Player*>(player);

    // they come in from the west, the Ukrainian side, out of sight
    const float a = 3.14159f + ((std::rand() % 161) - 80) / 100.0f;
    const float dist = 38.0f + std::rand() % 12;
    const float x = std::max(-6.0f, prey->coord.x + std::cos(a) * dist);
    const float y = std::max(-6.0f, std::min(common::SIZEY + 6.0f, prey->coord.y + std::sin(a) * dist * 0.6f));
    launchFpv(level, x, y, prey);
}

bool fpvFrame(Level& level, Drone& d, float dt, Coord& blastAt)
{
    d.age += dt;
    if (!d.diving) {
        if (!stillHunted(level, d.prey)) d.prey = fpvPickPrey(level, d.x, d.y);
        float heading = std::atan2(d.vy, d.vx);
        if (d.prey) {
            const float tx = d.prey->coord.x, ty = d.prey->coord.y;
            if (std::hypot(tx - d.x, ty - d.y) <= FPV_DIVE_RANGE) {
                // locked on: straight down onto where he stands now
                d.diving = true;
                d.diveX = tx;
                d.diveY = ty;
                if (d.prey == level.getPlayer()) {
                    if (level.printer) level.printer->showMessage("IIIIIIIIII! FPV-drooni sy\xC3\xB6ksyy suoraan kohti sinua!", &level, false);
                } else if (random(50)) {
                    static const char* cries[] = { "FPV! Maahan!", "Drooni! Ei, ei, ei...", "Mamochka!", "Ampukaa se!" };
                    d.prey->say(cries[std::rand() % 4]);
                }
            } else {
                float diff = std::atan2(ty - d.y, tx - d.x) - heading;
                while (diff > 3.14159f) diff -= 6.28318f;
                while (diff < -3.14159f) diff += 6.28318f;
                const float maxTurn = 4.0f * dt;
                heading += std::max(-maxTurn, std::min(maxTurn, diff));
            }
        }
        // the pilot's hand is never quite steady
        heading += std::sin(d.age * 9.0f) * 0.8f * dt;
        d.vx = std::cos(heading) * FPV_SPEED;
        d.vy = std::sin(heading) * FPV_SPEED;
    }
    if (d.diving) {
        const float dx = d.diveX - d.x, dy = d.diveY - d.y;
        const float dist = std::hypot(dx, dy);
        const float step = FPV_DIVE_SPEED * dt;
        if (dist <= step + 0.3f) {
            blastAt = Coord{ (uint32_t)std::max(0.0f, std::round(d.diveX)), (uint32_t)std::max(0.0f, std::round(d.diveY)) };
            return true;
        }
        d.vx = dx / dist * FPV_DIVE_SPEED;
        d.vy = dy / dist * FPV_DIVE_SPEED;
    }
    d.x += d.vx * dt;
    d.y += d.vy * dt;
    return false;
}

void fpvBlast(Level& level, const Coord& at)
{
    HurtBy who(&level, "FPV-drooni");
    common::boomEvent(at, 2);
    level.addAnimation(new Animation(Coord{ at.x > 2 ? at.x - 2 : 0, at.y }, "*BUM*", 6));
    level.audio.addSound(at, SoundType::Explosion,
        "BUM! FPV-drooni r\xC3\xA4j\xC3\xA4hti!", "R\xC3\xA4j\xC3\xA4hdys!", "...pieni r\xC3\xA4j\xC3\xA4hdys kaukaa...", 40);
    const PlayerNS::Player* player = level.getPlayer();
    for (auto p : level.persons) {
        if (p->stats.health == 0) continue;
        const uint32_t dist = p->coord.distance(at);
        if (dist > 2) continue;
        const bool isPlayer = p == player;
        uint32_t dmg;
        // the player is a lucky drunk: the grenade that tears a soldier apart only maims him
        if (dist == 0)      dmg = isPlayer ? 3 + std::rand() % 3 : 12 + std::rand() % 6;
        else if (dist == 1) dmg = isPlayer ? 2 : 4 + std::rand() % 4;
        else                dmg = 1;
        p->damage(dmg);
        if (isPlayer && level.printer)
            level.printer->showMessage(dist == 0 ? "FPV-drooni r\xC3\xA4j\xC3\xA4hti p\xC3\xA4\xC3\xA4ll\xC3\xA4si!" : "FPV-droonin sirpaleet osuvat sinuun!", &level, false);
    }
    level.addSmoke(at);
    if (level.events && random(30)) level.events->addFiresAround(at);
}
