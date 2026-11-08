#include "vehicle.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "common.hpp"
#include "input.hpp"
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
    : Vehicle(Coord{79, route_y}, route_y), stopX(stop_x)
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
        coord.x = 79;
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
            sprintf(buf, "Pysäkki %d/%d. A/D=tasapaino, Z=poistu", stopsReached, MAX_STOPS);
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
            sprintf(buf, "TASAPAINO [%s] Pysäkit:%d/%d", bar, stopsReached, MAX_STOPS);
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
        sprintf(buf, "Matkustit %d pysäkin verran eteenpäin!", result.levelChange);
        printer->showMessage(string(buf), level, false);
    } else if (result.levelChange < 0) {
        char buf[80];
        sprintf(buf, "Lennit %d kenttää taaksepäin!", -result.levelChange);
        printer->showMessage(string(buf), level, false);
    } else {
        printer->showMessage("Et päässyt yhtään eteenpäin.", level, false);
    }
    printer->print(level);
    common::sleep(1500);

    return result;
}
