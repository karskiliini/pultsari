#include "events.hpp"
#include "level.hpp"
#include "printer.hpp"
#include "player.hpp"
#include "persons.hpp"
#include "building.hpp"
#include "animation.hpp"
#include "common.hpp"
#include <cstdlib>

using common::random;

EnvironmentEvents::EnvironmentEvents(Level* level) : level(level)
{
    if (level->isWarLevel) {
        droneCooldown = 5 + std::rand()%8;
        aaCooldown = 3 + std::rand()%6;
        droneNoiseCooldown = 1 + std::rand()%3;
    } else {
        droneCooldown = 10 + std::rand()%20;
        aaCooldown = 5 + std::rand()%15;
        droneNoiseCooldown = 3 + std::rand()%8;
        paradeCooldown = 15 + std::rand()%10;
    }
}

void EnvironmentEvents::update()
{
    droneSound();
    droneStrike();
    aaFire();
    updateFires();
    fireDamage();
    randomEvent();

    if (gasActive) {
        gasTimer++;
        if (gasTimer > 30) {
            gasActive = false;
        } else {
            // gas spreads from center, damages everyone in radius
            uint32_t radius = gasTimer / 3 + 2;
            for (auto& p : level->persons) {
                if (p->stats.health > 0 && p->coord.distance(gasCenter) <= radius) {
                    p->damage(1);
                    if (p->type == pelaaja && gasTimer % 3 == 0) {
                        level->printer->showMessage("Kaasu polttaa keuhkojasi! Pakene!", level, false);
                    }
                    // NPCs panic
                    if (p->type != pelaaja) {
                        p->alerted = true;
                        p->alertCooldown = 5;
                    }
                }
            }
        }
    }

    if (paradeCooldown > 0) --paradeCooldown;

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

    // a harmless drone buzzes over the city
    if (random(30)) level->launchDrone(nullptr);
}

void EnvironmentEvents::addFiresAround(const Coord& c)
{
    for (int i = 0; i < 3 + std::rand()%3; ++i) {
        int fx = (int)c.x + (std::rand()%7 - 3);
        int fy = (int)c.y + (std::rand()%5 - 2);
        if (fx < 0 || fy < 0 || fx >= (int)common::SIZEX || fy >= (int)common::SIZEY)
            continue;
        Coord fc { (uint32_t)fx, (uint32_t)fy };
        if (!hasFire(fc)) {
            fires.push_back(Fire{ fc, 20 + (uint32_t)(std::rand()%15), 15 });
        }
    }
}

void EnvironmentEvents::droneStrike()
{
    if (droneCooldown > 0) {
        --droneCooldown;
        return;
    }
    droneCooldown = level->isWarLevel ? (8 + std::rand()%10) : (15 + std::rand()%25);

    if (!random(level->isWarLevel ? 40 : 20)) return;
    if (level->buildings.empty()) return;

    // refineries are drone magnets
    std::vector<Building*> refineries;
    for (auto b : level->buildings) if (b->type == ERefinery) refineries.push_back(b);
    Building* target = (!refineries.empty() && random(60))
        ? refineries[std::rand() % refineries.size()]
        : level->buildings[std::rand() % level->buildings.size()];
    level->launchDrone(target);
}

void EnvironmentEvents::aaFire()
{
    if (aaCooldown > 0) {
        --aaCooldown;
        return;
    }
    aaCooldown = level->isWarLevel ? (4 + std::rand()%6) : (8 + std::rand()%12);

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

void EnvironmentEvents::gasAttack()
{
    if (gasActive) return;

    // pick a building as gas target
    if (level->buildings.empty()) return;
    uint32_t idx = std::rand() % level->buildings.size();
    Building* target = level->buildings[idx];

    gasActive = true;
    gasTimer = 0;
    uint32_t cx = (target->wallLeft + target->wallRight) / 2;
    uint32_t cy = (target->wallTop + target->wallBot) / 2;
    gasCenter = Coord{ cx, cy };

    if (level->isBolshoi) {
        level->printer->showMessage(
            "PANTTIVANKITILANNE BOLSHOI-TEATTERISSA! Erikoisjoukot pumppavat kaasua sisään! PAKENE!!", level, false);
    } else {
        level->printer->showMessage("PANTTIVANKITILANNE! Erikoisjoukot kaasuttavat " +
            target->getName() + "! PAKENE!!", level, false);
    }

    Animation* anim = new Animation(gasCenter, "~~KAASU~~", 15);
    level->addAnimation(anim);
}

void EnvironmentEvents::defenestration()
{
    // FSB throws someone out of a window - ambient event
    static const char* msgs[] = {
        "Kuulet huudon ylhäältä... joku tippui ikkunasta.",
        "Näet liikkeen kerrostalon katolla... joku putoaa!",
        "Sirpaleet räsähtävät! Joku lensi ikkunasta.",
        "'Se oli itsemurha', kuulet poliisin sanovan...",
    };

    uint32_t idx = std::rand() % 4;
    level->printer->showMessage(msgs[idx], level, false);

    // random debris/body lands near a building
    if (!level->buildings.empty()) {
        uint32_t bi = std::rand() % level->buildings.size();
        Building* b = level->buildings[bi];
        Coord impact { b->wallRight + 1, b->wallBot };
        if (impact.x < common::SIZEX && impact.y < common::SIZEY) {
            Animation* anim = new Animation(impact, "*THUD*", 8);
            level->addAnimation(anim);

            // damage anyone at impact
            for (auto& p : level->persons) {
                if (p->stats.health > 0 && p->coord.distance(impact) <= 1) {
                    p->damage(std::rand()%3 + 1);
                }
            }
        }
    }
}

void EnvironmentEvents::novichok()
{
    // random novichok poisoning attempt on the street
    uint32_t px = std::rand() % common::SIZEX;
    uint32_t py = std::rand() % common::SIZEY;
    Coord poisonCoord { px, py };

    level->printer->showMessage("Kadulla haistuu outo kemikaali... Novitshok!", level, false);

    // damage everyone in small radius
    for (auto& p : level->persons) {
        if (p->stats.health > 0 && p->coord.distance(poisonCoord) <= 2) {
            p->damage(std::rand()%4 + 2);
            if (p->type == pelaaja) {
                level->printer->showMessage("Tunnet polttavaa kipua! Myrkkyä!", level, false);
            }
        }
    }

    Animation* anim = new Animation(poisonCoord, "~~~", 10);
    level->addAnimation(anim);
}

void EnvironmentEvents::randomEvent()
{
    if (eventCooldown > 0) {
        --eventCooldown;
        return;
    }

    if (!random(2)) return;

    eventCooldown = 20 + std::rand()%30;

    int roll = std::rand() % 100;
    int gasChance = level->isBolshoi ? 30 : 5;
    if (roll < gasChance) {
        gasAttack();
    } else if (roll < gasChance + 10) {
        defenestration();
    } else if (roll < gasChance + 15) {
        novichok();
    }

    // war-specific events
    if (level->isWarLevel) {
        if (roll >= 20 && roll < 50) {
            artilleryBarrage();
        } else if (roll >= 50 && roll < 65) {
            meatAssault();
        }
    }

    // Moscow parade event
    if (!level->isWarLevel && roll >= 60 && roll < 70 && paradeCooldown == 0) {
        militaryParade();
    }

    // Hypermarket fire (Ukrainian drone strike)
    if (!level->isWarLevel && roll >= 70 && roll < 80) {
        hypermarketFire();
    }
}

void EnvironmentEvents::artilleryBarrage()
{
    level->printer->showMessage("TYKISTÖISKU!! Maahan!!", level, false);

    int shells = 3 + std::rand()%4;
    for (int i = 0; i < shells; ++i) {
        uint32_t sx = std::rand() % common::SIZEX;
        uint32_t sy = std::rand() % common::SIZEY;
        Coord shellCoord { sx, sy };

        common::boomEvent(shellCoord, 3);
        Animation* anim = new Animation(shellCoord, "*KRUMP*", 6);
        level->addAnimation(anim);

        for (auto& p : level->persons) {
            if (p->stats.health > 0 && p->coord.distance(shellCoord) <= 3) {
                uint32_t dmg = std::rand()%8 + 3;
                p->damage(dmg);
                if (p->type == pelaaja) {
                    level->printer->showMessage("Kranaatin sirpale osuu!", level, false);
                }
            }
        }

        // craters leave fires
        if (!hasFire(shellCoord)) {
            fires.push_back(Fire{ shellCoord, 10 + (uint32_t)(std::rand()%8), 10 });
        }
    }
}

void EnvironmentEvents::meatAssault()
{
    level->printer->showMessage("LIHAHYÖKKÄYS! Pakkovärvätyt rynnäköivät!", level, false);

    // spawn 3-5 skinheads from the right edge rushing left
    int count = 3 + std::rand()%3;
    for (int i = 0; i < count; ++i) {
        uint32_t sy = std::rand() % common::SIZEY;
        Coord spawnCoord { common::SIZEX - 2, sy };
        if (!level->hit(spawnCoord)) {
            level->createPerson(spawnCoord, skinhead);
        }
    }
}

void EnvironmentEvents::militaryParade()
{
    level->printer->showMessage("SOTILASPARAATI! Kolonna marssii kadulla!", level, false);

    uint32_t paradeY = std::rand() % (common::SIZEY - 4) + 2;

    for (uint32_t x = 0; x < common::SIZEX; x += 4) {
        Animation* anim = new Animation(Coord{x, paradeY}, "=>", 8);
        level->addAnimation(anim);
    }

    for (auto& p : level->persons) {
        if (p->stats.health > 0 && p->coord.y >= paradeY - 1 && p->coord.y <= paradeY + 1) {
            if (p->type == pelaaja) {
                level->printer->showMessage("Sotilaspoliisi: 'Pois tielt\xC3\xA4 tai rintamalle!'", level, false);
                p->damage(std::rand()%2 + 1);
            } else if (p->type != poliisi && p->type != fsb) {
                p->damage(std::rand()%2 + 1);
                p->alerted = true;
                p->alertCooldown = 5;
            }
        }
    }

    paradeCooldown = 40 + std::rand()%20;
}

void EnvironmentEvents::hypermarketFire()
{
    for (auto b : level->buildings) {
        if (b->type == EHypermarket) {
            Hypermarket* hm = dynamic_cast<Hypermarket*>(b);
            if (hm && !hm->onFire) {
                hm->startFire();

                level->printer->showMessage("RÄJÄHDYS! Ukrainalainen drone iski hypermarkettiin!", level, false);

                // guards flee
                for (auto& p : level->persons) {
                    if (p->type == vartija) {
                        Vartija* v = dynamic_cast<Vartija*>(p);
                        if (v && v->homePos.distance(hm->getDoor()) < 5) {
                            v->fleeing = true;
                            static const char* reactions[] = {
                                "ПОМОГИТЕ!!",
                                "Tulipalo! JUOSKAA!",
                                "Äää! Kaikki ulos!",
                            };
                            v->say(reactions[std::rand()%3]);
                        }
                    }
                }

                // start fire in the events system too
                Coord center = {(hm->wallLeft + hm->wallRight) / 2, hm->wallBot + 1};
                fires.push_back({center, 50, 30});

                break;
            }
        }
    }
}
