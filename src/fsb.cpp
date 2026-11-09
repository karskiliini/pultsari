// The FSB collects drunks who walk down the middle of the road: the longer the
// stroll, the likelier two black Auruses come for a new "volunteer".
#include "fsb.hpp"
#include "level.hpp"
#include "vehicle.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "common.hpp"
#include <algorithm>
#include <cstdlib>
#include <string>

using std::string;

void RoadRun::step(bool onRoad)
{
    if (onRoad) ++tiles;
    else tiles = tiles > DECAY ? tiles - DECAY : 0;
}

float RoadRun::dispatchChance() const
{
    if (tiles <= SAFE) return 0.0f;
    // slow at first, then steep: rarely before the warnings, almost surely by CERTAIN
    const float x = (float)(tiles - SAFE) / (float)(CERTAIN - SAFE);
    return std::min(0.5f, 0.15f * x * x);
}

bool Level::hasVehicle(const Vehicle* v) const
{
    return v && std::find(vehicles.begin(), vehicles.end(), v) != vehicles.end();
}

namespace {

int alongOf(const Road& r, const Coord& c) { return (int)(r.horizontal ? c.x : c.y); }

// a black car in the road's centre line, where ordinary traffic never drives
Car* spawnFsbCar(Level& level, const Road& r, int along, int dir)
{
    const int limit = (int)(r.horizontal ? common::SIZEX : common::SIZEY);
    for (int k = 0; k < 6; ++k) {
        const int a = ((along - dir * k * 3) % limit + limit) % limit;
        const Coord c = r.horizontal ? Coord{ (uint32_t)a, r.pos + 1 } : Coord{ r.pos + 1, (uint32_t)a };
        Car* car = new Car(c, r.horizontal, dir, Car::Kind::Fsb);
        bool free = true;
        for (const auto& cell : car->cells()) free = free && !level.getVehicleAt(cell);
        if (free) {
            level.addVehicle(car);
            return car;
        }
        delete car;
    }
    return nullptr;
}

const Road* roadUnder(const Level& level, const Coord& c)
{
    const Road* h = nullptr;
    const Road* v = nullptr;
    level.roadsAt(c, h, v);
    return h ? h : v;
}

// the cars drive off and the player is left alone for a while
void standDown(Level& level, FsbPatrol& f, uint32_t cooldown)
{
    for (auto car : f.cars) {
        if (!level.hasVehicle(car)) continue;
        car->script = Car::Script::Leave;
        car->reckless = false;
        car->arrived = false;
        car->speed = 80;
    }
    f.cars.clear();
    // the men who are still running get back in the cars
    for (size_t i = 0; i < level.persons.size();) {
        FSBAgent* a = dynamic_cast<FSBAgent*>(level.persons[i]);
        if (a && a->recruiter) {
            delete a;
            level.persons.erase(level.persons.begin() + i);
            continue;
        }
        ++i;
    }
    f.stage = FsbPatrol::Stage::Idle;
    f.run.tiles = 0;
    f.radioHeard = false;
    f.cooldown = cooldown;
}

void jumpOut(Level& level, Car* car)
{
    int placed = 0;
    for (const auto& cell : car->cells()) {
        for (int side : { -1, 1 }) {
            if (placed >= 2) return;
            Coord c = cell;
            if (car->horizontal) c.y = (uint32_t)((int)c.y + side);
            else c.x = (uint32_t)((int)c.x + side);
            if (level.hit(c) || level.getVehicleAt(c)) continue;
            FSBAgent* a = dynamic_cast<FSBAgent*>(level.createPerson(c, fsb));
            if (!a) continue;
            a->recruiter = true;
            a->firstMove = false;
            a->patience = 45;
            a->speed = 60 + std::rand() % 15;   // they sprint; a drunk can't outrun them for long
            ++placed;
        }
    }
}

} // namespace

void Level::fsbTick()
{
    FsbPatrol& f = fsbPatrol;
    f.cars.erase(std::remove_if(f.cars.begin(), f.cars.end(),
                                [&](Car* c) { return !hasVehicle(c); }), f.cars.end());
    PlayerNS::Player* player = getPlayer();
    if (!player || playerDead || isWarLevel || roads.empty() || player->conscripted || player->inJail) return;
    if (f.cooldown > 0) --f.cooldown;
    // at the wheel the road is where he belongs: only a drunk on foot is counted
    if (player->driving) {
        f.havePos = false;
        return;
    }

    // how far the player got since the last tick, and where he is now
    const Coord pos = player->coord;
    uint32_t moved = 0;
    int travel = 0;
    const Road* road = roadUnder(*this, pos);
    if (f.havePos) {
        const int dx = (int)pos.x - (int)f.lastPos.x, dy = (int)pos.y - (int)f.lastPos.y;
        moved = (uint32_t)std::max(std::abs(dx), std::abs(dy));
        if (moved > 6) moved = 0;  // a new level or a bus ride, not a walk
        if (road) travel = road->horizontal ? dx : dy;
    }
    f.lastPos = pos;
    f.havePos = true;

    bool dispatch = false;
    for (uint32_t i = 0; i < moved; ++i) {
        f.run.step(road != nullptr);
        if (road && f.cooldown == 0 && (f.stage == FsbPatrol::Stage::Idle || f.stage == FsbPatrol::Stage::Tailing) &&
            (float)(std::rand() % 10000) < f.run.dispatchChance() * 10000.0f)
            dispatch = true;
    }
    const int dir = travel > 0 ? 1 : travel < 0 ? -1 : (std::rand() % 2 ? 1 : -1);

    switch (f.stage) {
    case FsbPatrol::Stage::Idle:
        if (!road || f.cooldown > 0 || (!dispatch && f.run.tiles < RoadRun::WARN)) break;
        // first warning: a black car comes crawling up behind
        if (Car* tail = spawnFsbCar(*this, *road, alongOf(*road, pos) - dir * 10, dir)) {
            tail->script = Car::Script::Tail;
            tail->speed = 60;
            f.cars.push_back(tail);
        }
        f.stage = FsbPatrol::Stage::Tailing;
        printer->showMessage("Musta Aurus hidastaa viereesi... Tummennetun lasin takaa joku katsoo sinua.", this, false);
        if (!dispatch) break;
        // fall through: they had already decided
    case FsbPatrol::Stage::Tailing: {
        Car* tail = f.cars.empty() ? nullptr : f.cars.front();
        if (!dispatch) {
            if (f.run.tiles < RoadRun::SAFE) {
                printer->showMessage("Musta Aurus kiihdytt\xC3\xA4\xC3\xA4 pois. T\xC3\xA4ll\xC3\xA4 kertaa.", this, false);
                standDown(*this, f, 40);
                break;
            }
            // the tail keeps pace a few steps behind, or waits when the player turns back
            if (tail) {
                const int gap = tail->horizontal ? ((int)pos.x - (int)tail->coord.x) * tail->direction
                                                 : ((int)pos.y - (int)tail->coord.y) * tail->direction;
                tail->speed = gap > 3 ? (uint32_t)std::min(140, 20 * gap) : gap >= 0 ? 8 : 0;
            }
            if (!f.radioHeard && f.run.tiles >= (RoadRun::WARN + RoadRun::CERTAIN) / 2) {
                f.radioHeard = true;
                printer->showMessage("Mustasta Auruksesta kuuluu radiopuhelimen r\xC3\xA4tin\xC3\xA4\xC3\xA4. Joku sanoo \"rintama\" ja nauraa.", this, false);
                audio.addSound(tail ? tail->coord : pos, SoundType::WalkieTalkie,
                    "Radiopuhelin r\xC3\xA4tisee mustassa autossa", "Radiopuhelimen r\xC3\xA4tin\xC3\xA4\xC3\xA4", "...r\xC3\xA4tin\xC3\xA4\xC3\xA4...", 15);
            }
            break;
        }
        // the FSB has decided: the tail closes in from behind, a second car from ahead
        const Road* r = road ? road : roadUnder(*this, tail ? tail->coord : pos);
        if (!r) { standDown(*this, f, 40); break; }
        if (tail) {
            tail->script = Car::Script::Hunt;
            tail->reckless = true;
            tail->speed = 90;
        } else if (Car* c = spawnFsbCar(*this, *r, alongOf(*r, pos) - dir * 35, dir)) {
            c->script = Car::Script::Hunt;
            c->reckless = true;
            f.cars.push_back(c);
        }
        if (Car* c = spawnFsbCar(*this, *r, alongOf(*r, pos) + dir * 35, -dir)) {
            c->script = Car::Script::Hunt;
            c->reckless = true;
            f.cars.push_back(c);
        }
        f.stage = FsbPatrol::Stage::Dispatched;
        f.timer = 60;
        printer->showMessage("Radiopuhelin r\xC3\xA4tisee: \"Jeshtsho odin dobrovolets.\" (Viel\xC3\xA4 yksi vapaaehtoinen.) "
                             "Mustat Aurukset kaasuttavat sinua kohti sinivalot vilkkuen!", this, false);
        audio.addSound(pos, SoundType::Siren, "Sireenit ulvovat! Mustat autot tulevat!", "Sireenej\xC3\xA4", "...sireenej\xC3\xA4 kaukaa...", 40);
        break;
    }
    case FsbPatrol::Stage::Dispatched: {
        bool any = false;
        for (auto car : f.cars) {
            if (!car->arrived) continue;
            car->arrived = false;
            jumpOut(*this, car);
            any = true;
        }
        if (any) {
            for (auto car : f.cars) if (car->script == Car::Script::Hunt) car->script = Car::Script::Parked;
            printer->showMessage("Musta Aurus jarruttaa kirskuen viereesi. Miehet mustissa takeissa hypp\xC3\xA4\xC3\xA4v\xC3\xA4t ulos: \"Stoi! FSB!\"", this, false);
            f.stage = FsbPatrol::Stage::Grabbing;
            f.timer = 50;
            break;
        }
        if (f.timer == 0 || --f.timer == 0) {
            printer->showMessage("Mustat Aurukset kaartavat pois. T\xC3\xA4ll\xC3\xA4 kertaa ne eiv\xC3\xA4t l\xC3\xB6yt\xC3\xA4neet sinua.", this, false);
            standDown(*this, f, 60);
        }
        break;
    }
    case FsbPatrol::Stage::Grabbing: {
        bool chasing = false;
        for (auto p : persons) {
            const FSBAgent* a = dynamic_cast<const FSBAgent*>(p);
            if (a && a->recruiter && a->stats.health > 0) chasing = true;
        }
        if (!chasing || f.timer == 0 || --f.timer == 0) {
            printer->showMessage("FSB:n miehet palaavat autoihin kiroillen. Aurukset kaasuttavat pois.", this, false);
            standDown(*this, f, 120);
        }
        break;
    }
    }
}

void FSBAgent::recruitAct()
{
    PlayerNS::Player* p = level->getPlayer();
    if (!p || level->playerDead || p->conscripted) return;
    if (coord.distance(p->coord) <= 1) {
        say("Rintamalle, svinya!");
        p->conscripted = true;
        return;
    }
    if (patience == 0) {
        recruiter = false;
        say("Kuda on delsja? (Minne se katosi?)");
        return;
    }
    --patience;
    if (common::random(12)) {
        static const char* shouts[] = {
            "Stoi! FSB!",
            "Seis, kansalainen! Is\xC3\xA4nmaa kutsuu!",
            "Sinulla on kunnia palvella!",
            "Juokse vain, rintamalla juostaan lis\xC3\xA4\xC3\xA4!",
            "Allekirjoita t\xC3\xA4m\xC3\xA4, ja saat Ladan!",
        };
        say(shouts[std::rand() % 5]);
    }
    string msg;
    move(getMoveDirection(p->coord), msg);
    report(msg);
}
