#include "vehicle.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "common.hpp"
#include "input.hpp"
#include "persons.hpp"
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

bool Vehicle::hitVehicle(const Coord& c) const
{
    return (c.x == coord.x && c.y == coord.y);
}

// Bus
Bus::Bus(uint32_t route_y, uint32_t stop_x)
    : Vehicle(Coord{common::SIZEX - 1, route_y}, route_y), stopX(stop_x)
{
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

string Bus::typeToChar() const
{
    return "B";
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
Car::Car(const Coord& pos, bool horizontal, int dir)
    : Vehicle(pos, pos.y), horizontal(horizontal)
{
    direction = dir;
    struct Model { const char* name; const char* paint; uint32_t speed; };
    static const Model models[] = {
        { "Lada",        "1;97", 45 },
        { "Lada",        "1;91", 45 },
        { "Lada",        "1;38;5;180", 40 },
        { "Volga",       "1;38;5;238", 50 },
        { "UAZ",         "1;38;5;64", 40 },
        { "Gelendvagen", "1;38;5;232;48;5;250", 70 },
        { "Politsijan auto", "1;97;44", 60 },
    };
    const Model& m = models[std::rand() % 7];
    model = m.name;
    paint = m.paint;
    speed = m.speed * (80 + std::rand() % 41) / 100;
}

std::string Car::typeToChar() const
{
    return horizontal ? (direction > 0 ? "\xE2\x96\xB6" : "\xE2\x97\x80") : (direction > 0 ? "\xE2\x96\xBC" : "\xE2\x96\xB2");
}

void Car::frame(uint32_t ms)
{
    energy += speed * ms;
    if (energy < 10000) return;
    energy -= 10000;

    Coord next = coord;
    const uint32_t limit = horizontal ? common::SIZEX : common::SIZEY;
    uint32_t& axis = horizontal ? next.x : next.y;
    if (direction > 0) axis = axis + 1 >= limit ? 0 : axis + 1;
    else axis = axis == 0 ? limit - 1 : axis - 1;

    if (level->getVehicleAt(next)) return;

    Person* p = level->getPerson(next);
    if (p) {
        // mostly the driver leans on the horn; sometimes they don't brake at all
        if (waiting < 6 && common::random(80)) {
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
        return;
    }
    waiting = 0;
    coord = next;
}
