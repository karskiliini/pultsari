
#include "level.hpp"
#include "item.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "building.hpp"
#include "printer.hpp"
#include "coord.hpp"
#include "common.hpp"
#include "animation.hpp"
#include "vehicle.hpp"
#include "events.hpp"
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>

using std::find;
using std::cout;
using std::endl;
using std::string;
using PlayerNS::Player;

Level::Level(uint32_t difficulty) : stage(difficulty)
{
}

void Level::updateCamera()
{
    auto p = getPlayer();
    if (!p) return;
    cameraX = (int32_t)p->coord.x - (int32_t)common::VIEWPORT_W / 2;
    cameraY = (int32_t)p->coord.y - (int32_t)common::VIEWPORT_H / 2;
    if (cameraX < 0) cameraX = 0;
    if (cameraY < 0) cameraY = 0;
    if (cameraX > (int32_t)(sizex - common::VIEWPORT_W))
        cameraX = (int32_t)(sizex - common::VIEWPORT_W);
    if (cameraY > (int32_t)(sizey - common::VIEWPORT_H))
        cameraY = (int32_t)(sizey - common::VIEWPORT_H);
}

Level::~Level() {
    for (auto& p : buildings)
    {
        delete p;
    }
    buildings.clear();

    for (auto& p : persons)
    {
        // player needs to remain from level to level
        if (p->type != PersonType::pelaaja) {
            delete p;

        }
    }
    persons.clear();

    for (auto& p : items)
    {
        delete p;
    }
    items.clear();

    for (auto& v : vehicles)
    {
        delete v;
    }
    vehicles.clear();

    if (events) {
        delete events;
        events = nullptr;
    }

    for (auto a : animations) delete a;
    animations.clear();
    delete thrownItem;
    thrownItem = nullptr;
}

void Level::buildingTurn()
{
    for (auto& b : buildings)
    {
        b->npcAct();
    }
}

void Level::npcTurn()
{
    for(auto& p : persons) {
        string msg = "";
        if (p->stats.health > 0)
        {
            p->npcAct();
        }
    }
    spreadNews();
}

void Level::actThrow(Person* source)
{
    bool hit = false;
    while (!hit) {
        thrownItem->actThrow(this);

        auto p = getPerson(thrownItem->coord);
        if (p) {
            string msg = "";
            hit = p->interactThrow(thrownItem, source, msg);
            printer->showMessage(msg, this, false);
        } else {
            if (thrownItem->flight > 14) {
                printer->showMessage("Heitto putoaa maahan.", this, false);
                hit = true;
            } else if (hitWall(thrownItem->coord))
            {
                SuljettuLiike* shop = nullptr;
                for (auto b : buildings) {
                    if (b->hitWindow(thrownItem->coord)) { shop = dynamic_cast<SuljettuLiike*>(b); break; }
                }
                const bool heavy = thrownItem->type == EKivi || thrownItem->type == EPamppu ||
                                   thrownItem->type == EKetjut || thrownItem->type == EVodka;
                if (shop && heavy) {
                    shop->smashWindow(thrownItem->coord, source);
                    printer->showMessage("KILIN\xC3\x84! Ikkuna hajoaa ja h\xC3\xA4lytin alkaa ulvoa!", this, false);
                } else {
                    printer->showMessage("Osuit seinään.", this, false);
                }
                hit = true;
            } else {
                Coord copy { thrownItem->coord.x, thrownItem->coord.y };
                if (common::checkBounds(copy)) {
                    printer->showMessage("Huti meni.", this, false);
                    hit = true;
                }
            }
        }

        if (hit)
        {
            Coord landed = thrownItem->coord;
            bool grenade = thrownItem->type == EGranata;
            // it clattered: door guards nearby go and look
            for (auto p : persons) {
                Vartija* v = dynamic_cast<Vartija*>(p);
                if (v && v->stats.health > 0 && v->coord.distance(landed) <= 16) v->hearNoise(landed);
            }
            delete thrownItem;
            thrownItem = nullptr;
            if (grenade) {
                printer->showMessage("KRANAATTI R\xC3\x84J\xC3\x84HT\xC3\x84\xC3\x84!!", this, false);
                explode(landed, 3, 10, source);
            }
        }

        printer->print(this);
        common::sleep(16);
    }
}

void Level::stepAnimations()
{
    for (auto it = animations.begin(); it != animations.end();) {
        // real-time frames are 50 ms: floating texts move every 4th frame (~1 s in all)
        const uint32_t every = (common::realtimeEnabled && !(*it)->isBubble) ? 4 : 1;
        if (++(*it)->tick % every != 0) { ++it; continue; }
        if ((*it)->act()) {
            ++it;
        } else {
            delete *it;
            it = animations.erase(it);
        }
    }
}

void Level::playAnimation()
{
    // realtime advances animations once per frame in the main loop instead
    if (common::realtimeEnabled) return;

    while (!animations.empty()) {
        uint32_t delay = animations.front()->delay;
        stepAnimations();
        printer->print(this);
        common::sleep(delay);
    }
}

void Level::explode(const Coord& c, uint32_t radius, uint32_t maxDamage, Person* source)
{
    common::boomEvent(c, radius);
    for (auto& p : persons) {
        if (p->stats.health > 0 && p->coord.distance(c) <= radius) {
            p->damage(std::rand() % maxDamage + 1);
        }
    }
    uint32_t x = c.x > 4 ? c.x - 4 : 0;
    addAnimation(new Animation(Coord{ x, c.y }, "***BOOM***", 8));
    audio.addSound(c, SoundType::Explosion,
        "PAUUUUM! Kranaatti r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4!", "R\xC3\xA4j\xC3\xA4hdys!", "...r\xC3\xA4j\xC3\xA4hdys kaukaa...", 40);
    if (source) alertCops(source);
}

Person* Level::createPerson(const Coord& coord, PersonType type)
{
    Person* p = nullptr;

    if (!hit(coord))
    {
        switch(type) {
            case vanki:
                p = new Vanki(coord);
                break;
            case skinhead:
                p = new Skinhead(coord);
                break;
            case yka:
                p = new Yka(coord);
                break;
            case mummo:
                p = new Mummo(coord);
                break;
            case poliisi:
                p = new Cop(coord);
                break;
            case varas:
                p = new Varas(coord);
                break;
            case jengi:
                p = new Jengi(coord);
                break;
            case ryostaja:
                p = new Ryostaja(coord);
                break;
            case diileri:
                p = new Diileri(coord);
                break;
            case veteraani:
                p = new Veteraani(coord);
                break;
            case fsb:
                p = new FSBAgent(coord);
                break;
            case juoppo:
                p = new Juoppo(coord);
                break;
            case sotilas:
                p = new Sotilas(coord);
                break;
            case ilmatorjunta:
                p = new ITYksikko(coord);
                break;
            case vartija:
                p = new Vartija(coord);
                break;
            case putin:
                p = new Putin(coord);
                break;
            case kotiaiti:
                p = new Kotiaiti(coord);
                break;
            case rekryytti:
            case korealainen:
                p = new Varvatty(coord, type);
                break;
            default:
                break;
        }
    }

    if (p) {
        p->pathFinding = pathFinding;
        p->printer = printer;
        p->setLevel(this);
        addPerson(p);
    }

    return p;
}

Person* Level::getPerson(const Coord& coord) const
{
    for (auto& p : persons) {
        if (p->coord == coord) {
            return p;
        }
    }
    return nullptr;
}

Player* Level::getPlayer() const
{
    for (const auto& p : persons) {
        if (p->type == pelaaja) return dynamic_cast<Player*>(p);
    }
    return nullptr;
}

bool Level::PersonExists(PersonType type) const
{
    for (auto& p : persons) {
        if (p->type == type) return true;
    }
    return false;
}

bool Level::isAlive(const Person* p) const
{
    if (!p) return false;
    for (auto q : persons) {
        if (q == p) return q->stats.health > 0;
    }
    return false;
}

void Level::roadsAt(const Coord& c, const Road*& h, const Road*& v) const
{
    h = v = nullptr;
    for (const auto& r : roads) {
        if (!r.contains(c)) continue;
        if (r.horizontal) h = &r; else v = &r;
    }
}

void Level::vehicleFrame(uint32_t ms)
{
    for (auto v : vehicles) v->frame(ms);

    // drones fly on; a striking drone dives in when it reaches its target
    for (size_t i = 0; i < drones.size();) {
        Drone& d = drones[i];
        const float dt = ms / 1000.0f;
        d.age += dt;
        float heading = std::atan2(d.vy, d.vx);
        const float speed = std::hypot(d.vx, d.vy);
        if (d.target) {
            // homing: it banks toward the target in a wide arc, tighter the longer it has flown
            const float tx = (d.target->wallLeft + d.target->wallRight) / 2.0f;
            const float ty = (d.target->wallTop + d.target->wallBot) / 2.0f;
            float diff = std::atan2(ty - d.y, tx - d.x) - heading;
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            const float maxTurn = (0.45f + 0.25f * d.age) * dt;
            heading += std::max(-maxTurn, std::min(maxTurn, diff));
        } else {
            heading += d.turn * dt;
        }
        d.vx = std::cos(heading) * speed;
        d.vy = std::sin(heading) * speed;
        d.x += d.vx * dt;
        d.y += d.vy * dt;
        if (d.target) {
            const float tx = (d.target->wallLeft + d.target->wallRight) / 2.0f;
            const float ty = (d.target->wallTop + d.target->wallBot) / 2.0f;
            if (std::abs(d.x - tx) < 1.5f && std::abs(d.y - ty) < 1.5f) {
                Building* target = d.target;
                drones.erase(drones.begin() + i);
                droneImpact(Coord{ (uint32_t)tx, (uint32_t)ty }, target);
                continue;
            }
        }
        if (d.x < -12 || d.y < -12 || d.x > common::SIZEX + 12 || d.y > common::SIZEY + 12) {
            drones.erase(drones.begin() + i);
            continue;
        }
        ++i;
    }
}

void Level::launchDrone(Building* target)
{
    constexpr float SPEED = 13.0f;
    Drone d;
    if (target) {
        // comes in from a map edge straight at the target
        const float tx = (target->wallLeft + target->wallRight) / 2.0f;
        const float ty = (target->wallTop + target->wallBot) / 2.0f;
        d.x = common::random(50) ? -8.0f : common::SIZEX + 8.0f;
        d.y = (float)(std::rand() % common::SIZEY);
        const float len = std::max(1.0f, std::hypot(tx - d.x, ty - d.y));
        // it does not come straight in: it starts off at an angle and curves onto the target
        const float off = (0.5f + (std::rand() % 50) / 100.0f) * (common::random(50) ? 1.0f : -1.0f);
        const float h = std::atan2(ty - d.y, tx - d.x) + off;
        (void)len;
        d.vx = std::cos(h) * SPEED;
        d.vy = std::sin(h) * SPEED;
        d.target = target;
    } else {
        // a flyover, often close enough to the player to be seen
        const PlayerNS::Player* p = getPlayer();
        const float px = p ? p->coord.x : common::SIZEX / 2.0f;
        const float py = p ? p->coord.y : common::SIZEY / 2.0f;
        const float speed = SPEED * (0.8f + (std::rand() % 40) / 100.0f);
        if (common::random(60)) {
            const bool east = common::random(50);
            d.x = east ? -8.0f : common::SIZEX + 8.0f;
            d.y = py + (std::rand() % 25 - 12);
            d.vx = east ? speed : -speed;
            d.vy = 0;
        } else {
            const bool south = common::random(50);
            d.x = px + (std::rand() % 41 - 20);
            d.y = south ? -8.0f : common::SIZEY + 8.0f;
            d.vx = 0;
            d.vy = south ? speed * 0.6f : -speed * 0.6f; // cells are taller than wide
        }
    }
    if (!target && common::random(50)) d.turn = ((std::rand() % 2) ? 1.0f : -1.0f) * (0.04f + (std::rand() % 10) / 100.0f);
    drones.push_back(d);
}

void Level::droneImpact(const Coord& c, Building* target)
{
    common::boomEvent(c, 4);
    addAnimation(new Animation(Coord{ c.x > 5 ? c.x - 5 : 0, c.y }, "===BOOM===", 10));
    audio.addSound(c, SoundType::Explosion,
        "VALTAVA R\xC3\x84J\xC3\x84HDYS! Drooni iski!", "R\xC3\xA4j\xC3\xA4hdys!", "...kaukainen r\xC3\xA4j\xC3\xA4hdys...", 60);
    for (auto p : persons) {
        if (p->stats.health > 0 && p->coord.distance(c) <= 4) p->damage(std::rand() % 6 + 2);
    }
    if (events) {
        if (target && target->type != EKremlin && !dynamic_cast<Jalostamo*>(target))
            events->setAblaze(target->wallLeft, target->wallTop, target->wallRight, target->wallBot);
        else
            events->addFiresAround(c);
    }

    Jalostamo* refinery = dynamic_cast<Jalostamo*>(target);
    if (refinery) refinery->burning = 900;
    if (target && target->walkable && target->type != EKremlin) {
        // the blast tears holes in the walls: the building can now be walked into
        const uint32_t cx = (target->wallLeft + target->wallRight) / 2;
        for (int k = -1; k <= 1; ++k) {
            target->breaches.push_back(Coord{ cx + k, target->wallTop });
            target->breaches.push_back(Coord{ cx + k, target->wallBot });
        }
        for (const auto& w : target->windows) target->breaches.push_back(w);
    }

    const PlayerNS::Player* player = getPlayer();
    if (player && !playerDead && canHear(player->coord, c, 60, true)) {
        if (refinery) printer->showMessage("DRONE-ISKU! \xD0\x9D\xD0\x9F\xD0\x97 syttyi palamaan! Myrkyllist\xC3\xA4 savua!", this, false);
        else if (target) printer->showMessage("DRONE-ISKU! " + target->getName() + " sai osuman!", this, false);
        else printer->showMessage("DRONE-ISKU!", this, false);
    }
}

void Level::robberyTick()
{
    Robbery& r = robbery;
    const PlayerNS::Player* player = getPlayer();
    auto heard = [&](const Coord& c, uint32_t range) { return player && !playerDead && canHear(player->coord, c, range, true); };

    // robbers who made it back to the car get in
    for (size_t i = 0; i < persons.size();) {
        Pankkirosvo* rob = dynamic_cast<Pankkirosvo*>(persons[i]);
        if (rob && rob->boarded) {
            ++r.boarded;
            delete rob;
            persons.erase(persons.begin() + i);
            continue;
        }
        ++i;
    }
    std::vector<Pankkirosvo*> robbers;
    for (auto p : persons) {
        if (auto rob = dynamic_cast<Pankkirosvo*>(p)) if (rob->stats.health > 0) robbers.push_back(rob);
    }

    switch (r.stage) {
    case Robbery::Stage::Idle: {
        if (r.cooldown > 0) { --r.cooldown; return; }
        r.cooldown = 600 + std::rand() % 600;   // 4-8 minutes between robberies
        for (auto b : buildings) if (b->type == EBank) r.bank = b;
        if (!r.bank) return;
        const uint32_t windowX = r.bank->wallLeft + 4;
        const uint32_t lane = r.bank->wallBot + 3;  // top lane of Tverskaja (westbound)
        Car* car = new Car(Coord{ std::min<uint32_t>(common::SIZEX - 1, windowX + 70), lane }, true, -1, Car::Kind::Getaway);
        for (const auto& c : car->cells()) if (getVehicleAt(c)) { delete car; return; }
        car->script = Car::Script::ToBank;
        car->bank = r.bank;
        car->laneY = lane;
        addVehicle(car);
        r.car = car;
        r.police = nullptr;
        r.policeParked = false;
        r.boarded = 0;
        r.stage = Robbery::Stage::Arriving;
        break;
    }
    case Robbery::Stage::Arriving:
        if (r.car->script != Car::Script::Parked) break;
        // three robbers storm into the bank through the smashed window
        for (int k = 0; k < 3; ++k) {
            const Coord at { r.car->coord.x + (k == 0 ? (uint32_t)0 : k == 1 ? 1u : (uint32_t)-1), r.bank->wallBot - 2 };
            if (hitWall(at) || hitPerson(at)) continue;
            const Coord goal { r.bank->wallLeft + 2 + std::rand() % (r.bank->wallRight - r.bank->wallLeft - 3),
                               r.bank->wallTop + 1 + std::rand() % (r.bank->wallBot - r.bank->wallTop - 2) };
            Pankkirosvo* rob = new Pankkirosvo(at, goal);
            rob->pathFinding = pathFinding;
            rob->printer = printer;
            if (!addPerson(rob)) delete rob;
            else robbers.push_back(rob);
        }
        audio.addSound(Coord{ (r.bank->wallLeft + r.bank->wallRight) / 2, r.bank->wallTop }, SoundType::Siren,
            "Sberbankin h\xC3\xA4lytyskello soi!", "H\xC3\xA4lytyskello", "...h\xC3\xA4lytyskello soi jossain...", 45);
        if (heard(r.car->coord, 45)) printer->showMessage("PANKKIRY\xC3\x96ST\xC3\x96! Auto ajoi Sberbankin ikkunasta sis\xC3\xA4\xC3\xA4n!", this, false);
        if (!robbers.empty()) alertCops(robbers.front(), 45);
        r.timer = 60;
        r.stage = Robbery::Stage::Robbing;
        break;

    case Robbery::Stage::Robbing:
        if (r.timer > 0) --r.timer;
        if (r.timer % 8 == 0) {
            audio.addSound(Coord{ (r.bank->wallLeft + r.bank->wallRight) / 2, r.bank->wallTop }, SoundType::Siren,
                "Sberbankin h\xC3\xA4lytyskello soi!", "H\xC3\xA4lytyskello", "...h\xC3\xA4lytyskello soi jossain...", 45);
        }
        // the police turn up after a while
        if (r.timer == 45 && !r.police) {
            Car* police = new Car(Coord{ std::min<uint32_t>(common::SIZEX - 1, r.car->coord.x + 60), r.car->laneY }, true, -1, Car::Kind::Police);
            bool free = true;
            for (const auto& c : police->cells()) free = free && !getVehicleAt(c);
            if (free) { addVehicle(police); r.police = police; } else delete police;
        }
        // the patrol car pulls up by the bank and two officers jump out
        if (r.police && !r.policeParked && r.police->coord.x <= r.car->coord.x + 12) {
            r.police->script = Car::Script::Parked;
            r.policeParked = true;
            for (int k = 0; k < 2; ++k) {
                Person* cop = createPerson(Coord{ r.police->coord.x + k * 2, r.car->laneY - 1 }, poliisi);
                if (cop && !robbers.empty()) {
                    cop->chaseTarget = robbers.front();
                    cop->alerted = true;
                    cop->lastKnownPos = robbers.front()->coord;
                    cop->alertCooldown = 30;
                    cop->say(k ? "Politsija! Antautukaa!" : "Pysy suojassa!");
                }
            }
        }
        if (r.timer == 0 || robbers.empty()) {
            for (auto rob : robbers) {
                rob->phase = Pankkirosvo::Phase::Return;
                rob->goal = Coord{ r.car->coord.x, r.car->coord.y - 1 };
            }
            r.stage = Robbery::Stage::Returning;
        }
        break;

    case Robbery::Stage::Returning:
        if (!robbers.empty()) break;
        if (r.boarded == 0) {
            // every robber is dead: the car stays where it is, glovebox and all
            r.stage = Robbery::Stage::Idle;
            r.car = nullptr;
            break;
        }
        // back out of the bank and away
        r.car->script = Car::Script::Reverse;
        r.car->direction = 1;
        r.car->coord.y += r.car->length - 1;
        r.car->speed = 40;
        if (heard(r.car->coord, 40)) printer->showMessage("Ry\xC3\xB6st\xC3\xA4j\xC3\xA4t pakenevat!", this, false);
        r.stage = Robbery::Stage::Fleeing;
        break;

    case Robbery::Stage::Fleeing:
        // the police give chase, not caring whom they run over
        if (r.police && !r.police->gone && r.police->script != Car::Script::Chase &&
            r.car->script == Car::Script::Flee) {
            r.police->script = Car::Script::Chase;
            r.police->chasing = r.car;
            r.police->reckless = true;
            r.police->speed = 100;
            r.police->direction = -1;
            if (heard(r.police->coord, 40)) printer->showMessage("Politsijan auto l\xC3\xA4htee takaa-ajoon!", this, false);
        }
        if (r.car->gone || r.car->script == Car::Script::Parked) {
            r.stage = Robbery::Stage::Idle;
            r.car = nullptr;
            r.police = nullptr;
        }
        break;
    }

    // police cars that drove off the map are gone for good
    for (size_t i = 0; i < vehicles.size();) {
        Car* car = dynamic_cast<Car*>(vehicles[i]);
        if (car && car->gone && car != r.car && car != r.police) {
            delete car;
            vehicles.erase(vehicles.begin() + i);
            continue;
        }
        ++i;
    }
}

Person* Level::fireBullet(Person* shooter, float angle, uint32_t range, uint32_t damage, uint32_t hitPercent)
{
    const float dx = std::cos(angle), dy = std::sin(angle);
    float x = shooter->coord.x + 0.5f, y = shooter->coord.y + 0.5f;
    Coord last = shooter->coord;
    for (uint32_t step = 0; step < range * 2; ++step) {
        x += dx * 0.5f;
        y += dy * 0.5f;
        if (x < 0 || y < 0 || x >= common::SIZEX || y >= common::SIZEY) return nullptr;
        const Coord c { (uint32_t)x, (uint32_t)y };
        if (c == last) continue;
        last = c;
        if (hitWall(c)) return nullptr;
        Person* p = getPerson(c);
        if (p && p != shooter && p->stats.health > 0 && common::random(hitPercent)) {
            common::setHitDirection(dx > 0.4f ? 1 : dx < -0.4f ? -1 : 0, dy > 0.4f ? 1 : dy < -0.4f ? -1 : 0);
            p->damage(damage);
            return p;
        }
    }
    return nullptr;
}

void Level::addGas(const Coord& c, bool toxic)
{
    if (gas.size() >= 700) return;
    gas.push_back({ (float)c.x, (float)c.y, toxic ? 60u : 6u, toxic });
}

void Level::addSmoke(const Coord& c)
{
    if (gas.size() >= 900) return;
    GasPuff g { (float)c.x, (float)c.y, 18u + (uint32_t)(std::rand() % 14), false };
    g.smoke = true;
    gas.push_back(g);
}

int Level::gasAt(const Coord& c) const
{
    if (gasGrid.empty() || c.x >= common::SIZEX || c.y >= common::SIZEY) return 0;
    return gasGrid[c.y * common::SIZEX + c.x];
}

void Level::skyTick()
{
    // now and then a drone just passes over
    if (common::random(isWarLevel ? 4 : 1)) launchDrone(nullptr);

    // the wind veers a little all the time, and now and then swings right round
    windAngle += (std::rand() % 100 - 50) / 900.0f;
    if (common::random(1) && std::rand() % 4 == 0) windAngle += (std::rand() % 2 ? 1.0f : -1.0f) * (1.0f + (std::rand() % 100) / 80.0f);
    windSpeed = std::max(0.1f, std::min(0.8f, windSpeed + (std::rand() % 100 - 50) / 1500.0f));

    // every so often a drone comes down on a building close to the player and sets it ablaze
    if (nearStrikeCooldown > 0) --nearStrikeCooldown;
    const PlayerNS::Player* pl = getPlayer();
    if (nearStrikeCooldown == 0 && pl && !playerDead) {
        nearStrikeCooldown = 250 + std::rand() % 350; // about two to four minutes
        std::vector<Building*> near;
        for (auto b : buildings) {
            if (b->type == EKremlin) continue;
            const Coord mid { (b->wallLeft + b->wallRight) / 2, (b->wallTop + b->wallBot) / 2 };
            const uint32_t d = mid.distance(pl->coord);
            if (d >= 10 && d <= 30) near.push_back(b);
        }
        if (!near.empty()) launchDrone(near[std::rand() % near.size()]);
    }

    // fires send up smoke
    if (events) {
        for (const auto& f : events->getFires()) if (common::random(35)) addSmoke(f.coord);
    }
    for (const auto& d : drones) {
        audio.addSound(Coord{ (uint32_t)std::max(0.0f, d.x), (uint32_t)std::max(0.0f, d.y) }, SoundType::Drone,
            "Drooni p\xC3\xB6risee aivan yl\xC3\xA4puolella!", "Droonin p\xC3\xB6rin\xC3\xA4\xC3\xA4", "...kaukaista p\xC3\xB6rin\xC3\xA4\xC3\xA4...", 35);
    }

    // burning refineries pour out toxic smoke
    for (auto b : buildings) {
        Jalostamo* r = dynamic_cast<Jalostamo*>(b);
        if (!r || !r->burning) continue;
        --r->burning;
        for (int k = 0; k < 6; ++k) {
            addGas(Coord{ r->wallLeft + std::rand() % (r->wallRight - r->wallLeft + 1),
                          r->wallTop + std::rand() % (r->wallBot - r->wallTop + 1) }, true);
        }
        if (events && common::random(10)) events->addFiresAround(Coord{ (r->wallLeft + r->wallRight) / 2, r->wallTop });
        if (common::random(20)) {
            audio.addSound(Coord{ (r->wallLeft + r->wallRight) / 2, (r->wallTop + r->wallBot) / 2 }, SoundType::Fire,
                "Jalostamo roihuaa! S\xC3\xA4ili\xC3\xB6t pamahtelevat!", "Valtavan palon jyrin\xC3\xA4\xC3\xA4", "...palon jyrin\xC3\xA4\xC3\xA4...", 50);
        }
    }

    // smoke goes wherever the wind blows; exhaust just hangs where the car left it
    const float wx = std::cos(windAngle) * windSpeed, wy = std::sin(windAngle) * windSpeed * 0.6f;
    for (size_t i = 0; i < gas.size();) {
        GasPuff& g = gas[i];
        if (g.toxic || g.smoke) {
            g.x += wx + (std::rand() % 100 - 50) / 110.0f;
            g.y += wy + (std::rand() % 100 - 50) / 160.0f;
        }
        if (--g.ttl == 0 || g.x < 0 || g.y < 0 || g.x >= common::SIZEX || g.y >= common::SIZEY) {
            gas[i] = gas.back();
            gas.pop_back();
            continue;
        }
        ++i;
    }
    gasGrid.assign(common::SIZEX * common::SIZEY, 0);
    for (const auto& g : gas) {
        uint8_t& cell = gasGrid[(uint32_t)g.y * common::SIZEX + (uint32_t)g.x];
        cell = std::max<uint8_t>(cell, g.toxic ? 2 : g.smoke ? 3 : 1);
    }

    // breathing it
    for (auto p : persons) {
        if (p->stats.health == 0) continue;
        const int kind = gasAt(p->coord);
        if (kind == 3 && common::random(20)) {
            if (p->type == pelaaja) {
                if (common::random(30)) p->damage(1);
                printer->showMessage(common::random(50) ? "Savu kirvelee silmi\xC3\xA4. *K\xC3\xB6h k\xC3\xB6h*" : "Vedät palaneen muovin savua keuhkoihin.", this, false);
                common::sound("cough", 0.6f);
            }
        } else if (kind == 2 && common::random(35)) {
            p->damage(1);
            if (p->type == pelaaja) {
                printer->showMessage("Myrkyllinen savu polttaa keuhkojasi! *K\xC3\x96H K\xC3\x96H*", this, false);
                common::sound("cough", 0.8f);
            }
        }
    }
}

bool Level::isLit(const Coord& c) const
{
    const Building* b = buildingAt(c);
    return b && b->lit;
}

uint32_t Level::wallsBetween(const Coord& a, const Coord& b) const
{
    const int dx = (int)b.x - (int)a.x;
    const int dy = (int)b.y - (int)a.y;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    uint32_t walls = 0;
    const Building* inWall = nullptr;
    for (int i = 1; i < steps; ++i) {
        Coord c { (uint32_t)((int)a.x + dx * i / steps), (uint32_t)((int)a.y + dy * i / steps) };
        const Building* bld = buildingAt(c);
        const Building* hit = (bld && bld->hitWall(c) && !bld->hitWindow(c)) ? bld : nullptr;
        // a run of wall cells is one crossing
        if (hit && hit != inWall) walls += hit->type == EKremlin ? 2 : 1;
        inWall = hit;
    }
    return walls;
}

bool Level::canHear(const Coord& listener, const Coord& source, uint32_t range, bool loud) const
{
    if (listener.distance(source) > range) return false;
    const uint32_t walls = wallsBetween(listener, source);
    if (walls > 0 && !loud) return false;
    for (uint32_t w = walls; w > 0 && range > 0; --w) range /= 3;
    return listener.distance(source) <= range;
}

bool Level::radioReaches(const Coord& from, const Coord& to) const
{
    constexpr uint32_t RADIO_RANGE = 60;
    uint32_t range = RADIO_RANGE;
    if (from.distance(to) > range) return false;
    for (uint32_t w = wallsBetween(from, to); w > 0; --w) range = range * 2 / 3;
    return from.distance(to) <= range;
}

static bool isSecurity(const Person* p)
{
    return p->type == poliisi || p->type == fsb || p->type == sotilas ||
           (p->type == vartija && p->hasRadio);
}

void Level::alertCops(Person* source, uint32_t range)
{
    Person* firstWitness = nullptr;
    for (auto p : persons) {
        if (p == source || p->stats.health == 0 || !isSecurity(p)) continue;
        if (!p->canSeeTarget(source->coord) && !canHear(p->coord, source->coord, range, range > 15)) continue;
        p->chaseTarget = source;
        p->alerted = true;
        p->lastKnownPos = source->coord;
        p->alertCooldown = 20;
        if (!firstWitness) firstWitness = p;
    }
    if (firstWitness) callForHelp(firstWitness, source, true);
}

void Level::callForHelp(Person* caller, Person* target, bool hostile)
{
    const bool radio = caller->hasRadio && caller->type == vartija;
    if (radio) {
        caller->say("Kaikki yksik\xC3\xB6t, apua!");
        audio.addSound(caller->coord, SoundType::WalkieTalkie,
            "Vartija huutaa radioon", "Radiokutsu", "...radion r\xC3\xA4tin\xC3\xA4\xC3\xA4...", 15);
    } else if (hostile) {
        caller->say("Pomogite! T\xC3\xA4nne!");
        audio.addSound(caller->coord, SoundType::Voice,
            "Politsija huutaa apua", "Avunhuuto", "...joku huutaa apua...", 18);
    }

    for (auto p : persons) {
        if (p == caller || p == target || p->stats.health == 0) continue;
        if (p->type != poliisi && !(p->type == vartija && p->hasRadio)) continue;

        const bool sees = p->canSeeTarget(caller->coord);
        const bool hears = canHear(p->coord, caller->coord, 15);
        const bool radioed = radio && p->hasRadio && radioReaches(caller->coord, p->coord);
        if (!sees && !hears && !radioed) continue;

        if (!p->alerted && common::random(40))
            p->say(radioed && !hears ? "Vastaanotettu, tulossa!" : "Tulen avuksi!");
        if (hostile) p->chaseTarget = target;
        p->alerted = true;
        p->lastKnownPos = target->coord;
        p->alertCooldown = 15;
        if (radioed && !hears) {
            // the call comes out of this person's radio
            if (common::random(50)) p->say("*r\xC3\xA4tin\xC3\xA4* Apua tarvitaan, kaikki yksik\xC3\xB6t!");
            audio.addSound(p->coord, SoundType::WalkieTalkie,
                "Radiopuhelin r\xC3\xA4tisee", "Radion r\xC3\xA4tin\xC3\xA4\xC3\xA4", "...r\xC3\xA4tin\xC3\xA4\xC3\xA4...", 8);
        }
    }
}

void Level::witnessPutinDeath(const Coord& where)
{
    for (auto p : persons) {
        if (p->type == pelaaja || p->stats.health == 0) continue;
        if (p->canSeeTarget(where) || canHear(p->coord, where, 15)) p->knowsPutinDead = true;
    }
}

void Level::spreadNews()
{
    for (auto p : persons) {
        if (!p->knowsPutinDead || p->type == pelaaja || p->stats.health == 0) continue;
        if (!common::random(4)) continue;

        const bool loyal = p->type == fsb || p->type == sotilas || p->type == vartija ||
                           p->type == shoigu || p->type == gerasimov || p->type == medvedev;
        p->say(loyal ? "Prezident ubit!" : "VOVA ON KUOLLUT!");
        audio.addSound(p->coord, SoundType::Voice,
            "Joku huutaa jotain Vovasta", "Huutoa Vovasta", "...joku huutaa jotain Vovasta...", 14);

        for (auto q : persons) {
            if (q != p && q->type != pelaaja && !q->knowsPutinDead && canHear(q->coord, p->coord, 12))
                q->knowsPutinDead = true;
        }
    }
}

void Level::cleanDead()
{
    bool restart = true;
    while(restart) {
        restart = false;

        for (auto& p : persons) {
            if ((p->stats.health == 0) && (p->type != pelaaja)) {
                if (p->type == putin) {
                    putinDead = true;
                    witnessPutinDeath(p->coord);
                }
                if (p->type == pankkirosvo && common::random(70)) items.push_back(new Rynkky(p->coord));
                delete p;
                p = nullptr;

                auto it = find(persons.begin(), persons.end(), p);
                persons.erase(it);

                restart = true;
                break;
            }
        }
    }
}

void Level::cleanDiscardedItems()
{
    bool restart = true;
    while(restart) {
        restart = false;

        for (auto& i : items) {
            if (i->discard) {
                delete i;
                i = nullptr;

                auto it = find(items.begin(), items.end(), i);
                items.erase(it);

                restart = true;
                break;
            }
        }
    }
}

Building* Level::buildingAt(const Coord& c) const
{
    if (c.x >= common::SIZEX || c.y >= common::SIZEY) return nullptr;
    if (buildingGridCount != buildings.size()) {
        // a building's footprint never changes once placed, so the grid only follows additions
        buildingGrid.assign(common::SIZEX * common::SIZEY, nullptr);
        for (auto it = buildings.rbegin(); it != buildings.rend(); ++it) {
            const Building* b = *it;
            for (uint32_t y = b->wallTop; y <= b->wallBot && y < common::SIZEY; ++y)
                for (uint32_t x = b->wallLeft; x <= b->wallRight && x < common::SIZEX; ++x)
                    buildingGrid[y * common::SIZEX + x] = *it;   // earlier buildings win, as before
        }
        buildingGridCount = buildings.size();
    }
    return buildingGrid[c.y * common::SIZEX + c.x];
}

bool Level::hitBuilding(const Coord& c) const
{
    return buildingAt(c) != nullptr;
}

bool Level::hitWall(const Coord& c) const
{
    const Building* b = buildingAt(c);
    return b && b->hitWall(c);
}

bool Level::hitWindow(const Coord& c) const
{
    const Building* b = buildingAt(c);
    return b && b->hitWindow(c);
}

bool Level::hitPerson(const Coord& c) const
{
    for (const auto& p : persons)
    {
        if ((p->coord == c) && (p->stats.health > 0))
            return true;
    }
    return false;
}

bool Level::hitItem(const Coord& c) const
{
    for (const auto& i : items)
    {
        if (i->coord == c)
            return true;
    }
    return false;
}

bool Level::hitLOSBlocker(const Coord& c) const
{
    for (const auto& i : items)
    {
        if (i->coord == c && i->blocksLOS)
            return true;
    }
    return false;
}

bool Level::hit(const Coord& c) const
{
    return hitBuilding(c) || hitPerson(c) || hitItem(c);
}

Item* Level::getItem(const Coord& c) const
{
    for (auto& i : items) {
        if (i->coord == c) return i;
    }
    return nullptr;
}

Coord Level::freePosition() const
{
    Coord coord;

    bool recheck;
    do {
        coord.x = std::rand() % sizex;
        coord.y = std::rand() % sizey;
        recheck = hit(coord);
    } while (recheck);

    return coord;
}

Person* Level::checkPerson(const Coord& c)
{
    for (auto& p : persons) {
        if (p->coord == c) {
            return p;
        }
    }
    return nullptr;
}

void Level::addBonas()
{
    for (uint32_t i = 0; i < stage; ++i) {
        Coord c = freePosition();
        auto b = new Bona(c);
        items.push_back(b);
    }
}

void Level::addItem(ItemType item)
{
    Coord c = freePosition();

    Item* i;
    switch(item)
    {
        default:
            printer->printErr("ERROR: addItem\n");
            i = nullptr;
            break;
        case EKalja:
            i = new Kalja(c);
            break;
        case ELonkka:
            i = new Lonkka(c);
            break;
        case ELenkki:
            i = new Lenkki(c);
            break;
        case EPamppu:
            i = new Pamppu(c);
            break;
        case EVeitsi:
            i = new Veitsi(c);
            break;
        case EKetjut:
            i = new Ketjut(c);
            break;
        case EKivi:
            i = new Kivi(c);
            break;
        case EBootsit:
            i = new Bootsit(c);
            break;
        case EKala:
            i = new Kala(c);
            break;
        case EOmppo:
            i = new Omppo(c);
            break;
        case EBanaani:
            i = new Banaani(c);
            break;
        case ERaha:
            i = new Raha(c);
            break;
        case EPaska:
            i = new Paska(c);
            break;
        case EViesti:
            i = new Viesti(c);
            break;
        case EPensas:
            i = new Pensas(c);
            break;
        case ERoskis:
            i = new Roskis(c);
            break;
        case EPistooli:
            i = new Pistooli(c);
            break;
        case EPropusk:
            i = new Propusk(c);
            break;
        case EGranata:
            i = new Granata(c);
            break;
        case EVodka:
            i = new Vodka(c);
            break;
    }
    if (i) {
        addItem(i);
    }
}

bool Level::addBuilding(Building* building)
{
    if (!building) return false;

    building->level = this;
    buildings.push_back(building);
    return true;
}

// base initiative per kind of person (actions per second x10)
static uint32_t baseSpeed(PersonType t)
{
    switch (t) {
        case vanki:        return 30; // runs for it
        case varas:        return 32;
        case skinhead:     return 27;
        case jengi:        return 28;
        case ryostaja:     return 28;
        case fsb:          return 28;
        case poliisi:      return 25;
        case sotilas:      return 25;
        case ilmatorjunta: return 25;
        case vartija:      return 24;
        case veteraani:    return 22;
        case diileri:      return 22;
        case yka:          return 18;
        case putin:        return 18;
        case shoigu:
        case gerasimov:    return 18;
        case juoppo:       return 15; // drunk: slow
        case medvedev:     return 14;
        case mummo:        return 12;
        case avustaja:     return 10; // blind drunk
        case kotiaiti:     return 11;
        case lapsi:        return 34;
        case kansalainen:  return 22;
        case rotta:        return 30;
        case pankkirosvo:  return 26;
        case rekryytti:    return 19;
        case korealainen:  return 23;
        default:           return 20;
    }
}

// how tempted each kind of person is to pick the player's pocket (percent, never 0)
static uint32_t theftMotivation(PersonType t)
{
    switch (t) {
        case varas:        return 60;
        case lapsi:        return 40;
        case ryostaja:     return 40;
        case jengi:        return 35;
        case vanki:        return 30;
        case diileri:      return 25;
        case skinhead:     return 20;
        case juoppo:       return 15;
        case yka:          return 15;
        case kansalainen:  return 3;
        case poliisi:      return 10;
        case sotilas:      return 8;
        case avustaja:     return 8;
        case veteraani:    return 6;
        case fsb:          return 5;
        case vartija:      return 5;
        case mummo:        return 4;
        case kotiaiti:     return 3;
        case medvedev:     return 3;
        case shoigu:
        case gerasimov:    return 2;
        default:           return 1;
    }
}

// the weak run off with the money; the strong just stare you down
static bool isWeak(const Person* p)
{
    if (p->stats.health <= 6) return true;
    switch (p->type) {
        case mummo: case juoppo: case avustaja: case lapsi: case kotiaiti: case yka: return true;
        default: break;
    }
    const Veteraani* v = dynamic_cast<const Veteraani*>(p);
    return v && v->oneLegged;
}

std::string Level::nameOf(const Person* p) const
{
    switch (p->type) {
        case juoppo:       return dynamic_cast<const Juoppo*>(p)->name;
        case mummo:        return "Babuška";
        case poliisi:      return "Politsija";
        case varas:        return "Varas";
        case vanki:        return "Pakovanki";
        case skinhead:     return "Skinhead";
        case yka:          return "Yevgeni";
        case jengi:        return "Gopnik";
        case ryostaja:     return "Ry\xC3\xB6st\xC3\xA4j\xC3\xA4";
        case diileri:      return "Diileri";
        case veteraani:    return dynamic_cast<const Veteraani*>(p)->oneLegged ? "Jalkapuoli veteraani" : "Veteraani";
        case fsb:          return "FSB-agentti";
        case sotilas:      return "Sotilas";
        case ilmatorjunta: return "Ilmatorjuntamies";
        case vartija:      return "Vartija";
        case putin:        return "Vova";
        case medvedev:     return "Medvedev";
        case shoigu:       return "\xC5\xA0oigu";
        case gerasimov:    return "Gerasimov";
        case avustaja:     return "Avustaja";
        case kotiaiti:     return "Koti\xC3\xA4iti";
        case lapsi:        return "Lapsi";
        case kansalainen:  return dynamic_cast<const Kansalainen*>(p)->name;
        case rotta:        return "Rotta";
        case pankkirosvo:  return "Pankkiry\xC3\xB6st\xC3\xA4j\xC3\xA4";
        case rekryytti:    return "Värvätty";
        case korealainen:  return "Pohjoiskorealainen";
        default:           return "Joku";
    }
}

// one action's chance to pick the player's pocket when right next to them
static bool tryPickpocket(Level* level, Person* p)
{
    PlayerNS::Player* player = level->getPlayer();
    if (!player || level->playerDead || player->stats.money == 0 || p->coord.distance(player->coord) > 1) return false;
    if ((uint32_t)(std::rand() % 800) >= theftMotivation(p->type)) return false;

    const uint32_t amount = std::max<uint32_t>(1, player->stats.money * (5 + std::rand() % 26) / 100);
    player->stats.money -= amount;
    p->stolenMoney += amount;
    level->printer->showMessage(level->nameOf(p) + " varasti sinulta " + std::to_string(amount) + " ruplaa!", level, false);

    if (isWeak(p)) {
        p->fleeActions = 20;
        const Veteraani* v = dynamic_cast<const Veteraani*>(p);
        p->say(p->type == lapsi ? "H\xC3\xA4h\xC3\xA4\xC3\xA4! Kiitti rahoista, bomž!"
               : (v && v->oneLegged) ? "*k\xC3\xB6p\xC3\xB6tt\xC3\xA4\xC3\xA4 karkuun*" : "*kipitt\xC3\xA4\xC3\xA4 karkuun*");
    } else {
        p->say("Mit\xC3\xA4 tuijotat? Haluatko lis\xC3\xA4\xC3\xA4?");
    }
    return true;
}

// a weak thief steps away from the player
static void fleeStep(Level* level, Person* p)
{
    const PlayerNS::Player* player = level->getPlayer();
    if (!player) return;
    Coord best = p->coord;
    uint32_t bestDist = p->coord.distance(player->coord);
    const int d[4][2] = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };
    for (const auto& o : d) {
        Coord c { (uint32_t)((int)p->coord.x + o[0]), (uint32_t)((int)p->coord.y + o[1]) };
        if (c.x >= common::SIZEX || c.y >= common::SIZEY) continue;
        if (level->hitWall(c) || level->hitPerson(c)) continue;
        const uint32_t dist = c.distance(player->coord);
        if (dist > bestDist) { best = c; bestDist = dist; }
    }
    p->coord = best;
}

// ---- empty bottles: from five up they draw people, and the poorer they are the harder ----

static constexpr uint32_t BOTTLE_LURE_MIN = 5;
static constexpr uint32_t BOTTLE_SACK_FULL = 25;

static bool isOneLegged(const Person* p)
{
    const Veteraani* v = dynamic_cast<const Veteraani*>(p);
    return v && v->oneLegged;
}

// the poor take bottles by the handful and anything else that comes loose
static bool isPoor(const Person* p)
{
    switch (p->type) {
        case juoppo: case yka: case avustaja: case mummo: case lapsi: case vanki: case varas: return true;
        default: return isOneLegged(p);
    }
}

// how much someone wants the bottles (percent per action with a full sack)
static uint32_t bottleGreed(const Person* p)
{
    if (isOneLegged(p)) return 70;
    switch (p->type) {
        case juoppo: case yka:            return 70;
        case avustaja:                    return dynamic_cast<const Avustaja*>(p)->collapsed ? 0 : 50;
        case mummo: case lapsi:
        case vanki: case varas:           return 45;
        // the feared and the ordinary only want one, now and then
        case jengi: case skinhead: case ryostaja: case diileri: case pankkirosvo:
        case veteraani: case kansalainen: case kotiaiti: case rekryytti:
                                          return 12;
        case poliisi: case sotilas: case ilmatorjunta: case korealainen:
                                          return 5;
        case fsb: case putin: case medvedev: case shoigu: case gerasimov:
                                          return 2;
        default:                          return 0; // guards keep to their door; rats don't drink
    }
}

// a drunk or an escaped convict wants a fight on top of the bottles
static bool picksFights(const Person* p)
{
    return p->type == juoppo || p->type == vanki;
}

// in plain sight within `range`: a clinking sack carries further than a drunk's eyesight
static bool seesPlayer(const Level* level, const Person* p, const Coord& target, uint32_t range)
{
    if (p->blind || p->coord.distance(target) > range) return false;
    if (level->hitLOSBlocker(target) && p->coord.distance(target) > 1) return false;
    return level->raycast(p->coord, target);
}

// one step towards the player, around walls and people
static void approachStep(Level* level, Person* p, const Coord& target)
{
    Coord best = p->coord;
    uint32_t bestDist = p->coord.distance(target);
    const int d[4][2] = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };
    for (const auto& o : d) {
        Coord c { (uint32_t)((int)p->coord.x + o[0]), (uint32_t)((int)p->coord.y + o[1]) };
        if (c.x >= common::SIZEX || c.y >= common::SIZEY) continue;
        if (level->hitBuilding(c) || level->hitPerson(c) || level->hitItem(c)) continue;
        const uint32_t dist = c.distance(target);
        if (dist < bestDist) { best = c; bestDist = dist; }
    }
    p->coord = best;
}

static void stealBottles(Level* level, Person* p, Player* player)
{
    auto& inv = player->inventory;
    const string name = level->nameOf(p);
    if (!isPoor(p)) {
        // someone you would rather not meet takes one bottle and goes; you breathe out
        --inv.pullot;
        static const char* relief[] = {
            " otti tyhj\xC3\xA4n pullon ja k\xC3\xA4veli pois. Huh.",
            " nappasi pullon. Onneksi vain pullon.",
            " vei tyhj\xC3\xA4n pullon. Et aio pyyt\xC3\xA4\xC3\xA4 sit\xC3\xA4 takaisin.",
        };
        level->printer->showMessage(name + relief[std::rand() % 3], level, false);
        p->say(common::random(50) ? "Pantti on pantti." : "T\xC3\xA4m\xC3\xA4 on nyt minun.");
        return;
    }

    const uint32_t bottles = std::min(inv.pullot, std::max<uint32_t>(1, inv.pullot * (10 + std::rand() % 21) / 100));
    inv.pullot -= bottles;
    p->stolenBottles += bottles;
    string msg = name + " kahmaisi sinulta " + std::to_string(bottles) + (bottles == 1 ? " pullon" : " pulloa");

    // anything else that comes loose
    for (int tries = 0; tries < 2; ++tries) {
        if (!common::random(45)) continue;
        const int k = std::rand() % STEALABLE_COUNT;
        if (stealable(inv, k) == 0) continue;
        --stealable(inv, k);
        p->stolenItems.push_back(k);
        msg += string(", ") + stealableName(k);
    }
    if (player->stats.money > 0 && common::random(25)) {
        const uint32_t amount = std::max<uint32_t>(1, player->stats.money / 10);
        player->stats.money -= amount;
        p->stolenMoney += amount;
        msg += " ja " + std::to_string(amount) + " ruplaa";
    }
    level->printer->showMessage(msg + "!", level, false);

    if (picksFights(p)) {
        p->brawlActions = 15;
        static const char* taunts[] = {
            "Mit\xC3\xA4 tuijotat? Haluatko turpaan?",
            "Minun pulloni! Tule hakemaan!",
            "Sinulla oli liikaa, bom\xC5\xBE!",
        };
        p->say(taunts[std::rand() % 3]);
    } else {
        p->fleeActions = 20;
        p->say(p->type == lapsi ? "H\xC3\xA4h\xC3\xA4\xC3\xA4! Pantit minulle!" : "*kipitt\xC3\xA4\xC3\xA4 karkuun pullot kainalossa*");
    }
}

// someone who sees your bottles comes for them; out of sight, out of mind
static bool bottleLure(Level* level, Person* p)
{
    Player* player = level->getPlayer();
    if (!player || level->playerDead || player->inJail) return false;
    const uint32_t bottles = player->inventory.pullot;
    if (bottles < BOTTLE_LURE_MIN) return false;
    const uint32_t greed = bottleGreed(p);
    if (greed == 0) return false;
    // a bigger sack is noticed from further away and wanted more
    const uint32_t reach = 4 + std::min(bottles, BOTTLE_SACK_FULL) / 2;
    if (!seesPlayer(level, p, player->coord, reach)) return false;
    const uint32_t chance = greed * std::min(bottles, BOTTLE_SACK_FULL) / BOTTLE_SACK_FULL;
    if ((uint32_t)(std::rand() % 100) >= std::max<uint32_t>(1, chance)) return false;

    if (p->coord.distance(player->coord) <= 1) {
        stealBottles(level, p, player);
    } else {
        if (common::random(8)) p->say(isPoor(p) ? "Pulloja! Anna yksi, veli!" : "Mit\xC3\xA4 sinulla on siin\xC3\xA4 s\xC3\xA4kiss\xC3\xA4?");
        approachStep(level, p, player->coord);
    }
    return true;
}

// a bottle thief who picked a fight keeps at you while he can see you
static bool brawlAct(Level* level, Person* p)
{
    if (p->brawlActions == 0) return false;
    Player* player = level->getPlayer();
    if (!player || level->playerDead || !seesPlayer(level, p, player->coord, 12)) {
        p->brawlActions = 0; // lost sight of you, forgot all about it
        return false;
    }
    --p->brawlActions;
    if (p->coord.distance(player->coord) <= 1) {
        if (common::random(50)) {
            level->printer->showMessage(level->nameOf(p) + (common::random(50) ? " ly\xC3\xB6 sinua!" : " potkaisee sinua s\xC3\xA4\xC3\xA4reen!"), level, false);
            player->damage(1);
        } else {
            p->say(common::random(50) ? "No? Lyö sitten!" : "Pelk\xC3\xA4\xC3\xA4tk\xC3\xB6?");
        }
    } else {
        approachStep(level, p, player->coord);
    }
    return true;
}

void Level::npcFrame(uint32_t ms)
{
    // the player breathes the exhaust every step, not just on world ticks
    if (PlayerNS::Player* pl = getPlayer()) {
        exhaustMs = exhaustMs > ms ? exhaustMs - ms : 0;
        if (pl->stats.health > 0 && !playerDead && gasAt(pl->coord) == 1 && exhaustMs == 0) {
            exhaustMs = 1200;
            if (common::random(60)) {
                // a sniff of hydrocarbons: a quick buzz, at a price
                pl->updatepromilles(1);
                if (common::random(40)) pl->damage(1);
                static const char* lines[] = {
                    "Vedät pakokaasua henkeen.",
                    "Vedät hiilivetyj\xC3\xA4 keuhkoihin.",
                    "Ladan pakokaasu kirvelee silmi\xC3\xA4. P\xC3\xA4\xC3\xA4 kevenee.",
                    "Hengit\xC3\xA4t lyijyll\xC3\xA4 maustettua ilmaa. *K\xC3\xB6h*",
                };
                printer->showMessage(lines[std::rand() % 4], this, false);
            }
        }
    }

    // index loop: an NPC's action may add people
    for (size_t i = 0; i < persons.size(); ++i) {
        Person* p = persons[i];
        if (p->type == pelaaja || p->stats.health == 0) continue;
        p->energy += p->speed * ms;
        if (p->energy >= 10000) {
            p->energy = std::min<uint32_t>(p->energy - 10000, 10000);
            if (p->fleeActions > 0) {
                --p->fleeActions;
                fleeStep(this, p);
            } else if (brawlAct(this, p) || bottleLure(this, p)) {
                // busy with the player's bottles
            } else if (!tryPickpocket(this, p)) {
                p->npcAct();
            }
        }
    }
}

bool Level::addPerson(Person* person)
{
    if (person->type != pelaaja) {
        // everyone has their own pace
        person->speed = baseSpeed(person->type) * (80 + std::rand() % 41) / 100;
        const Veteraani* v = dynamic_cast<const Veteraani*>(person);
        if (v && v->oneLegged) person->speed /= 2;
        person->energy = std::rand() % 10000;
    }
    // eyesight differs from person to person
    if (person->type != pelaaja && person->visionRange > 0)
        person->visionRange = std::max<uint32_t>(1, person->visionRange * (70 + std::rand() % 61) / 100);

    for (const auto &p : persons)
    {
        if ((p->coord.x == person->coord.x) && (p->coord.y == person->coord.y))
            return false;
    }

    person->setLevel(this);
    persons.push_back(person);
    return true;
}

void Level::addItem(Item* item)
{
    items.push_back(item);
}

void Level::addThrownItem(Item* item)
{
    thrownItem = item;
}

void Level::addVehicle(Vehicle* v)
{
    v->level = this;
    vehicles.push_back(v);
}

void Level::updateVehicles()
{
    for (auto& v : vehicles)
    {
        v->update();
    }
}

Vehicle* Level::getVehicleAt(const Coord& c) const
{
    for (auto& v : vehicles)
    {
        if (v->hitVehicle(c)) return v;
    }
    return nullptr;
}

void Level::addAnimation(Animation* animation, bool play)
{
    if (!common::animsEnabled) {
        delete animation;
        return;
    }

    constexpr size_t MAX_ANIMATIONS = 24;
    if (animations.size() >= MAX_ANIMATIONS) {
        delete animations.front();
        animations.erase(animations.begin());
    }
    animations.push_back(animation);

    if (play) {
        playAnimation();
    }
}

void Level::removeItem(Item* item)
{
    delete item;
    auto it = find(items.begin(), items.end(), item);
    if (it != items.end())
    {
        items.erase(it);
    }
}

// walls block LOS, but windows and doors don't
bool Level::blocksLOS(const Coord& c) const
{
    if (hitLOSBlocker(c)) return true;

    if (const Building* b = buildingAt(c)) {
        const bool seeThroughWindow = b->hitWindow(c) && (!b->walkable || b->lit);
        if (b->hitWall(c) && !seeThroughWindow && !b->hitDoor(c))
            return true;
    }
    return false;
}

// return true if visibility ok
bool Level::raycast(Coord from, const Coord& to) const
{
    // solid buildings always show their walls and plates; a walkable interior
    // (the Kremlin) is seen only through windows, the door or from inside
    bool originBuilding = false;
    if (const Building* b = buildingAt(to)) originBuilding = !b->walkable || b->hitWall(to) || b->isBreach(to);

    uint32_t block = 0;
    uint32_t dx = std::max(from.x, to.x) - std::min(from.x, to.x);
    uint32_t dy = std::max(from.y, to.y) - std::min(from.y, to.y);
    float ang = (dy == 0) ? 10.f : (float)((float)dx / (float)dy);

    if (ang < 1.f) {
        bool ret = (from == to);
        float k = (ang == 0) ? 100 : 1.f / ang;

        while(from != to) {
            uint32_t dx = std::max(from.x, to.x) - std::min(from.x, to.x);
            uint32_t dy = std::max(from.y, to.y) - std::min(from.y, to.y);
            float ang = (dy == 0) ? 999.f : (float)((float)dx / (float)dy);
            k = (ang == 0) ? (float)dy / 2.f : 1.f / ang;
            k = ((int)k == 0) ? 1 : k;

            for (int i = (int)k; i > 0; --i)
            {
                if (from.y < to.y) ++from.y;
                else if (from.y > to.y) --from.y;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
                if (block > 0) {
                    break;
                }
            }
            if (block <= 1)
            {
                if (from.x < to.x) ++from.x;
                else if (from.x > to.x) --from.x;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
            }

            if (from == to)
            {
                ret = true;
                break;
            }
            if ((block <= 1) && hitPerson(from) && (to == from)) {
                ret = true;
                break;
            } else if (block > 0) {
                ret = false;
                break;
            }
        }

        // always show building internals
        if (originBuilding) ret = true;

        return ret;

    } else if (ang >= 1.f) {
        bool ret = (from == to);
        float k = ang;

        while(from != to) {

            uint32_t dx = std::max(from.x, to.x) - std::min(from.x, to.x);
            uint32_t dy = std::max(from.y, to.y) - std::min(from.y, to.y);
            float ang = (dy == 0) ? 100 : (float)((float)dx / (float)dy);
            k = ang;

            for (int i = (int)k; i > 0; --i)
            {
                if (from.x < to.x) ++from.x;
                else if (from.x > to.x) --from.x;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
                if (block > 0) {
                    break;
                }
            }
            if (block <= 1)
            {
                if (from.y < to.y) ++from.y;
                else if (from.y > to.y) --from.y;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
            }

            if (from == to)
            {
                ret = true;
                break;
            }
            if ((block <= 1) && hitPerson(from) && (to == from)) {
                ret = true;
                break;
            } else if (block > 0) {
                ret = false;
                break;
            }
        }
        // always show building internals
        if (originBuilding) ret = true;

        return ret;
    }

    return true;
}

Person* Level::raycast(const Coordinate<int>& from, const Coordinate<int>& vector) const
{
    if (vector == Coordinate<int>(0, 0)) return nullptr;

    Coordinate<int> pos = from;
    pos.x += vector.x;
    pos.y += vector.y;

    while (true) {
        if (pos.x < 0 || pos.y < 0 ||
            pos.x >= (int)sizex || pos.y >= (int)sizey) {
            return nullptr;
        }

        Coord c { (uint32_t)pos.x, (uint32_t)pos.y };

        if (hitBuilding(c) || hitLOSBlocker(c)) return nullptr;

        Person* p = getPerson(c);
        if (p) return p;

        pos.x += vector.x;
        pos.y += vector.y;
    }
}


Building* Level::raycastBuilding(const Coordinate<int>& from, const Coordinate<int>& vector) const
{
    return nullptr;
}
