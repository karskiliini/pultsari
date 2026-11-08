#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include "coord.hpp"
#include <cstdint>
#include <string>

class Level;

namespace PlayerNS {
    class Player;
}

class Vehicle {
public:
    Vehicle(const Coord& pos, uint32_t route_y);
    virtual ~Vehicle() = default;

    virtual void update();
    virtual std::string typeToChar() const { return "B"; }

    bool hitVehicle(const Coord& c) const;

    Coord coord;
    uint32_t routeY;
    int direction = -1;
    bool atStop = false;
    uint32_t stopTimer = 0;
    Level* level = nullptr;
};

class Bus : public Vehicle {
public:
    Bus(uint32_t route_y, uint32_t stop_x);
    virtual ~Bus() = default;

    virtual void update();
    virtual std::string typeToChar() const;

    uint32_t stopX;
    static constexpr uint32_t STOP_DURATION = 30;
};

struct BusGameResult {
    int levelChange = 0;
    bool ejected = false;
};

BusGameResult runBusMinigame(PlayerNS::Player* player, Level* level);

#endif
