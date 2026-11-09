#include "vehicle.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "common.hpp"
#include "input.hpp"
#include "persons.hpp"
#include "animation.hpp"
#include "building.hpp"
#include <cmath>
#include <iostream>

using std::string;
using std::cout;

Vehicle::Vehicle(const Coord& pos, uint32_t route_y)
    : coord(pos), routeY(route_y)
{
}

void Vehicle::update()
{
}

std::vector<Coord> Vehicle::cells() const
{
    std::vector<Coord> out;
    const uint32_t limit = horizontal ? common::SIZEX : common::SIZEY;
    for (uint32_t i = 0; i < length; ++i) {
        Coord c = coord;
        uint32_t& axis = horizontal ? c.x : c.y;
        // body cells trail behind the front, wrapping around the map edge
        axis = (uint32_t)(((int64_t)axis - (int64_t)direction * i + limit) % limit);
        out.push_back(c);
    }
    return out;
}

int Vehicle::cellIndex(const Coord& c) const
{
    const auto body = cells();
    for (size_t i = 0; i < body.size(); ++i) {
        if (body[i] == c) return (int)i;
    }
    return -1;
}

bool Vehicle::hitVehicle(const Coord& c) const
{
    return cellIndex(c) >= 0;
}

// a boxy car: arrow at the front, solid body, half block at the back
std::string Vehicle::glyphAt(uint32_t i) const
{
    if (i == 0) {
        if (horizontal) return direction > 0 ? "\xE2\x96\xB6" : "\xE2\x97\x80";
        return direction > 0 ? "\xE2\x96\xBC" : "\xE2\x96\xB2";
    }
    if (i + 1 == length && horizontal) return direction > 0 ? "\xE2\x96\x90" : "\xE2\x96\x8C";
    return "\xE2\x96\x88";
}

// Bus
Bus::Bus(uint32_t route_y, uint32_t stop_x)
    : Vehicle(Coord{common::SIZEX - 8, route_y}, route_y), stopX(stop_x)
{
    length = 7;
}

// a long yellow bus with a row of windows
std::string Bus::glyphAt(uint32_t i) const
{
    if (i == 0 || i + 1 == length) return Vehicle::glyphAt(i);
    return (i % 2) ? "\xE2\x96\x88" : "\xE2\x96\x92";
}

const char* Bus::colorAt(uint32_t i) const
{
    return (i % 2 || i == 0 || i + 1 == length) ? "1;93" : "1;96";
}

void Bus::update()
{
    if (atStop) {
        if (stopTimer > 0) {
            --stopTimer;
        } else {
            atStop = false;
        }
        return;
    }

    coord.x += direction;

    if (coord.x == stopX) {
        atStop = true;
        stopTimer = STOP_DURATION;
    }

    if (coord.x == 0) {
        coord.x = common::SIZEX - 1;
    }
}


// Bus mini-game
BusGameResult runBusMinigame(PlayerNS::Player* player, Level* level)
{
    BusGameResult result;

    Printer* printer = level->printer;
    printer->showMessage("Nouset bussiin...", level, false);
    printer->print(level);
    common::sleep(500);

    InputNS::Input::raw();

    uint32_t ticks = 0;
    uint32_t stopsReached = 0;
    constexpr uint32_t TICKS_PER_STOP = 40;
    constexpr uint32_t MAX_STOPS = 5;

    float balance = 0.0f;
    bool riding = true;
    bool fell_asleep = false;

    while (riding && stopsReached < MAX_STOPS) {
        char c = InputNS::Input::getRawCharNonBlocking();

        float sway = sinf((float)ticks * 0.12f) * (float)player->stats.promilles / 80.0f;
        balance += sway * 0.08f;

        if (c == 'a' || c == 'j') balance -= 0.3f;
        if (c == 'd' || c == 'l') balance += 0.3f;

        if (common::random(3) && player->stats.promilles > 20) {
            balance += ((float)(std::rand() % 100) / 100.0f - 0.5f) * 0.4f;
        }

        if (player->stats.promilles > 25 && common::random(2)) {
            if (!fell_asleep && common::random(5)) {
                fell_asleep = true;
                printer->showMessage("Silmäsi painuvat kiinni...", level, false);
                printer->print(level);
                common::sleep(1500);

                stopsReached += std::rand() % 3 + 1;
                if (stopsReached >= MAX_STOPS) stopsReached = MAX_STOPS;
                fell_asleep = false;
                printer->showMessage("Heräät! Missä ollaan?", level, false);
                printer->print(level);
                common::sleep(500);
                continue;
            }
        }

        if (fabsf(balance) > 3.0f) {
            printer->showMessage("Kaadut bussin lattialle!", level, false);
            printer->print(level);
            common::sleep(800);
            player->damage(1);
            balance = 0.0f;

            if (common::random(40)) {
                printer->showMessage("Kuski heittää sinut ulos!", level, false);
                printer->print(level);
                common::sleep(1000);
                result.ejected = true;
                riding = false;
                break;
            }
        }

        if (player->stats.promilles > 28 && common::random(3)) {
            printer->showMessage("Oksennat bussin lattialle!", level, false);
            printer->print(level);
            common::sleep(800);
            player->damage(1);

            if (common::random(60)) {
                printer->showMessage("Kuski heittää sinut ulos oksentamisen takia!", level, false);
                printer->print(level);
                common::sleep(1000);
                result.ejected = true;
                riding = false;
                break;
            }
        }

        ++ticks;
        if (ticks % TICKS_PER_STOP == 0) {
            ++stopsReached;
            char buf[80];
            snprintf(buf, sizeof(buf), "Pysäkki %d/%d. A/D=tasapaino, Z=poistu", stopsReached, MAX_STOPS);
            printer->showMessage(string(buf), level, false);
            printer->print(level);

            for (int wait = 0; wait < 15; ++wait) {
                char ch = InputNS::Input::getRawCharNonBlocking();
                if (ch == 'z') {
                    riding = false;
                    break;
                }
                common::sleep(50);
            }
        }

        // render balance meter
        {
            int meter = (int)(balance * 5.0f);
            if (meter < -10) meter = -10;
            if (meter > 10) meter = 10;
            char buf[80];
            char bar[25];
            for (int i = 0; i < 21; ++i) bar[i] = ' ';
            bar[21] = '\0';
            bar[10] = '|';
            int pos = 10 + meter;
            if (pos < 0) pos = 0;
            if (pos > 20) pos = 20;
            bar[pos] = '@';
            snprintf(buf, sizeof(buf), "TASAPAINO [%s] Pysäkit:%d/%d", bar, stopsReached, MAX_STOPS);
            printer->showMessage(string(buf), level, false);
        }

        printer->print(level);
        common::sleep(50);
    }

    InputNS::Input::noraw();

    if (result.ejected) {
        if (stopsReached == 0) {
            result.levelChange = -(std::rand() % 2 + 1);
        } else {
            result.levelChange = (int)stopsReached - 1;
        }
    } else {
        result.levelChange = (int)stopsReached;
    }

    if (result.levelChange > 0) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Matkustit %d pysäkin verran eteenpäin!", result.levelChange);
        printer->showMessage(string(buf), level, false);
    } else if (result.levelChange < 0) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Lennit %d kenttää taaksepäin!", -result.levelChange);
        printer->showMessage(string(buf), level, false);
    } else {
        printer->showMessage("Et päässyt yhtään eteenpäin.", level, false);
    }
    printer->print(level);
    common::sleep(1500);

    return result;
}

// Car
Car::Car(const Coord& pos, bool horiz, int dir, Kind kind)
    : Vehicle(pos, pos.y)
{
    horizontal = horiz;
    direction = dir;
    struct Model { const char* name; const char* paint; uint32_t speed; uint32_t length; };
    // Moscow traffic is mostly Ladas: small and boxy
    static const Model models[] = {
        { "Lada", "1;97", 42, 3 }, { "Lada", "1;91", 42, 3 }, { "Lada", "1;38;5;180", 38, 3 },
        { "Lada", "1;38;5;109", 40, 3 }, { "Lada", "1;38;5;143", 40, 3 }, { "Lada", "1;97", 44, 3 },
        { "Lada", "1;38;5;167", 40, 3 },
        { "Volga", "1;38;5;245", 48, 4 },
        { "UAZ", "1;38;5;64", 38, 3 },
        { "Gelendvagen", "1;38;5;240", 70, 4 },
    };
    Model m = models[std::rand() % 10];
    if (kind == Kind::Police) m = { "Politsijan auto", "1;94", 55, 4 };
    if (kind == Kind::Getaway) m = { "Ry\xC3\xB6st\xC3\xA4jien Lada", "1;38;5;130", 60, 3 };
    loot = kind == Kind::Getaway;
    if (kind == Kind::Limousine) m = { "Vovan limusiini", "38;5;16", 55, 10 };
    limousine = kind == Kind::Limousine;
    armoured = limousine;
    police = kind == Kind::Police;
    keysIn = true;      // whoever drives it has the key in the ignition
    model = m.name;
    paint = m.paint;
    length = horizontal ? m.length : 2;
    speed = kind == Kind::Random ? m.speed * (80 + std::rand() % 41) / 100 : m.speed;
}

void Car::frame(uint32_t ms)
{
    if (freeMode) { drive(ms); return; }
    msSinceMoved += ms;
    // a car with nobody alive at the wheel goes nowhere
    if (!driverInside || driverDead) return;
    if (script == Script::Parked || gone) return;
    energy += speed * ms;
    if (energy < 10000) return;
    energy -= 10000;

    // the getaway car turns off the lane and drives up through the bank window
    if (script == Script::ToBank && bank && coord.x == bank->wallLeft + 4) { // Pankki::robberyWindow()
        horizontal = false;
        direction = -1;
        length = 2;
        script = Script::Smash;
    }
    if (script == Script::Smash && bank && coord.y + 1 == bank->wallBot) {
        script = Script::Parked;
        return;
    }
    // backing out of the bank to the lane, then away at full speed
    if (script == Script::Reverse && coord.y >= laneY) {
        coord.y = laneY;
        horizontal = true;
        direction = -1;
        length = 3;
        script = Script::Flee;
        reckless = true;
        speed = 90;
    }
    // fleeing and chasing cars leave the map instead of driving round again
    if ((script == Script::Flee || script == Script::Chase) && horizontal && coord.x == 0) {
        gone = true;
        return;
    }

    Coord next = coord;
    const uint32_t limit = horizontal ? common::SIZEX : common::SIZEY;
    uint32_t& axis = horizontal ? next.x : next.y;
    if (direction > 0) axis = axis + 1 >= limit ? 0 : axis + 1;
    else axis = axis == 0 ? limit - 1 : axis - 1;

    if (Vehicle* blocker = level->getVehicleAt(next)) {
        // the police ram the getaway car to a stop
        if (script == Script::Chase && blocker == chasing) {
            chasing->script = Script::Parked;
            script = Script::Parked;
            level->audio.addSound(coord, SoundType::Fight, "RYS\xC3\x84YS! Politsija rammaa pakoauton!", "Kolarin ryskett\xC3\xA4", "...kolarin rysk\xC3\xA4ys...", 40);
            common::boomEvent(next, 1);
        }
        return;
    }

    if (script == Script::Smash && bank && bank->hitWall(next)) {
        // through the window: glass everywhere
        bank->breaches.push_back(next);
        Coord side { next.x + 1, next.y };
        if (bank->hitWall(side)) bank->breaches.push_back(side);
        level->audio.addSound(next, SoundType::Glass, "KILINÄ! Auto ajaa pankin ikkunasta sisään!", "Lasin s\xC3\xA4rkymist\xC3\xA4", "...lasin helin\xC3\xA4\xC3\xA4...", 40);
        common::boomEvent(next, 1);
    }

    Person* p = level->getPerson(next);
    if (p && p->type == pelaaja && level->playerDead) {
        common::hitEvent(next, 2);
        p = nullptr;
    }
    if (p) {
        // mostly the driver leans on the horn; sometimes they don't brake at all
        if (!reckless && waiting < 6 && common::random(80)) {
            ++waiting;
            if (waiting == 1) {
                level->audio.addSound(coord, SoundType::Engine,
                    "T\xC3\x96\xC3\x96T! " + model + " t\xC3\xB6\xC3\xB6tt\xC3\xA4\xC3\xA4", "Autontorvi", "...t\xC3\xB6\xC3\xB6ttely\xC3\xA4...", 15);
            }
            return;
        }
        waiting = 0;
        p->damage(std::rand() % 2 + 2);
        if (p->type == pelaaja) {
            level->printer->showMessage(model + " t\xC3\xB6ytt\xC3\xA4\xC3\xA4 sinut kumoon! Kuski ei edes hidasta.", level, false);
        } else {
            p->report(model + " ajaa jalankulkijan yli. Kukaan ei pys\xC3\xA4hdy.");
        }
        level->audio.addSound(coord, SoundType::Fight, "KOLAHDUS! Auto osuu ihmiseen", "Kolahdus", "...kolahdus...", 18);
        // the victim is thrown to the side of the lane
        Coord side = p->coord;
        if (horizontal) side.y += (side.y % 2) ? 1 : -1; else side.x += (side.x % 2) ? 1 : -1;
        if (!level->hit(side)) p->coord = side;
        if (!reckless) return;
    }
    waiting = 0;
    coord = next;
    msSinceMoved = 0;
    checkNearMiss();

    // old Ladas leave a cloud right behind them
    if ((model == "Lada" || model == "UAZ") && common::random(40)) {
        Coord behind = cells().back();
        uint32_t& axis = horizontal ? behind.x : behind.y;
        if (direction > 0 ? axis > 0 : axis + 1 < (horizontal ? common::SIZEX : common::SIZEY)) {
            axis = direction > 0 ? axis - 1 : axis + 1;
            level->addGas(behind, false);
        }
    }
}

// A car that passes right beside the player leans on the horn and the driver
// yells abuse out of the window, once per pass.
void Car::checkNearMiss()
{
    const PlayerNS::Player* p = level->getPlayer();
    if (!p || level->playerDead) return;

    bool beside = false, near = false;
    for (const auto& c : cells()) {
        const int dx = (int)p->coord.x - (int)c.x;
        const int dy = (int)p->coord.y - (int)c.y;
        const int along = horizontal ? dx : dy;
        const int across = horizontal ? dy : dx;
        if (along == 0 && (across == 1 || across == -1)) beside = true;
        if (std::abs(dx) <= 4 && std::abs(dy) <= 4) near = true;
    }
    if (!near) nearMissed = false;
    if (!beside || nearMissed) return;
    nearMissed = true;

    static const char* insults[] = {
        "Pois tielt\xC3\xA4, saatanan pultsari!",
        "Idi nahui, juoppo!",
        "Blyat! Seuraavalla kerralla ajan yli!",
        "Katso eteesi, sika!",
        "Durak! Tie on autoille!",
        "Suka! Opettele k\xC3\xA4velem\xC3\xA4\xC3\xA4n!",
        "Mene kotiin, alkoholik!",
        "Haista paska, rappioalkkis!",
        "Pjanitsa! Melkein naarmutit maalit!",
        "Jop tvoju mat! Sokea vai humalassa?!",
        "Mudak! Kadulla ei nukuta!",
        "Kuole pois, laitapuolen kulkija!",
    };
    const string insult = insults[std::rand() % (sizeof(insults) / sizeof(insults[0]))];

    level->audio.addSound(coord, SoundType::Engine,
        "T\xC3\x96\xC3\x96T T\xC3\x96\xC3\x96\xC3\x96\xC3\x96\xC3\x96T! " + model + " t\xC3\xB6\xC3\xB6tt\xC3\xA4\xC3\xA4 vimmatusti ja kuski huutaa: \"" + insult + "\"",
        "Autontorvi ja huutoa", "...t\xC3\xB6\xC3\xB6ttely\xC3\xA4...", 18);

    // the driver's yell hangs over the car like anybody's speech bubble
    uint32_t cols = 0;
    for (unsigned char ch : insult) if ((ch & 0xC0) != 0x80) ++cols;
    const uint32_t half = cols / 2;
    const Coord at { coord.x > half ? coord.x - half : 0, coord.y > 1 ? coord.y - 1 : 0 };
    Animation* anim = new Animation(at, insult, common::realtimeEnabled ? 50 : 4);
    anim->isBubble = true;
    anim->owner = this;
    anim->source = coord;
    for (auto it = level->animations.begin(); it != level->animations.end();) {
        if ((*it)->owner == this) { delete *it; it = level->animations.erase(it); }
        else ++it;
    }
    level->addAnimation(anim, false);
}
