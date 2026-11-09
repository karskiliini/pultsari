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
    virtual void frame(uint32_t ms) { }
    virtual std::string typeToChar() const { return "B"; }
    virtual const char* color() const { return "1;93;44"; }

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

// A car driving its lane end to end; honks at people in the way, or hits them.
class Car : public Vehicle {
public:
    Car(const Coord& pos, bool horizontal, int direction);
    virtual void frame(uint32_t ms);
    virtual std::string typeToChar() const;
    virtual const char* color() const { return paint; }
    std::string model;
private:
    bool horizontal;
    uint32_t speed;   // tiles per second x10
    uint32_t energy = 0;
    uint32_t waiting = 0;
    const char* paint;
};

struct BusGameResult {
    int levelChange = 0;
    bool ejected = false;
};

BusGameResult runBusMinigame(PlayerNS::Player* player, Level* level);

#endif
