#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include "coord.hpp"
#include <cstdint>
#include <string>
#include <vector>

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
    virtual std::string name() const { return "Auto"; }
    // glyph and colour of one of the vehicle's cells (index 0 is the front)
    virtual std::string glyphAt(uint32_t index) const;
    virtual const char* colorAt(uint32_t index) const { return "1;93"; }

    // every cell the vehicle covers, front first; the body trails behind the front
    std::vector<Coord> cells() const;
    bool hitVehicle(const Coord& c) const;
    // which cell of the vehicle is at c, or -1
    int cellIndex(const Coord& c) const;

    Coord coord;            // the front of the vehicle
    uint32_t length = 1;
    bool horizontal = true;
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
    virtual std::string name() const { return "Bussi"; }
    virtual std::string glyphAt(uint32_t index) const;
    virtual const char* colorAt(uint32_t index) const;

    uint32_t stopX;
    static constexpr uint32_t STOP_DURATION = 30;
};

// A car driving its lane end to end; honks at people in the way, or hits them.
class Car : public Vehicle {
public:
    enum class Kind { Random, Limousine, Police };
    Car(const Coord& pos, bool horizontal, int direction, Kind kind = Kind::Random);
    virtual void frame(uint32_t ms);
    virtual std::string name() const { return model; }
    virtual std::string glyphAt(uint32_t index) const;
    virtual const char* colorAt(uint32_t index) const;
    std::string model;
    uint32_t speed;   // tiles per second x10
private:
    bool limousine = false;
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
