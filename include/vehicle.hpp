#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include "coord.hpp"
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

class Level;
class Building;
class Person;

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
    virtual std::vector<Coord> cells() const;
    bool hitVehicle(const Coord& c) const;
    // which cell of the vehicle is at c, or -1
    int cellIndex(const Coord& c) const;

    Coord coord;            // the front of the vehicle
    uint32_t length = 1;
    bool horizontal = true;
    uint32_t routeY;
    int direction = -1;
    bool atStop = false;
    // blown up: a burnt-out shell that no longer moves
    bool wrecked = false;
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

// What the person at the wheel does this frame.
struct DriveControls {
    uint32_t throttle = 0;   // W presses since the last frame
    int steer = 0;           // A (-1) and D (+1) presses since the last frame
    bool brake = false;      // S held
    bool clutch = false;     // Shift held: the clutch pedal on the floor
    int shift = 0;           // +1 up a gear, -1 down a gear
};

// A car driving its lane end to end; honks at people in the way, or hits them.
// A car someone takes over turns free: it drives in any of 72 directions.
class Car : public Vehicle {
public:
    enum class Kind { Random, Limousine, Police, Getaway };
    static constexpr int HEADINGS = 72;     // 5 degrees each; 0 = east, clockwise
    // scripted driving for a bank robbery and the police chase
    enum class Script { None, ToBank, Smash, Parked, Reverse, Flee, Chase };
    Car(const Coord& pos, bool horizontal, int direction, Kind kind = Kind::Random);
    virtual void frame(uint32_t ms);
    virtual std::string name() const { return model; }
    virtual std::string glyphAt(uint32_t index) const;
    virtual const char* colorAt(uint32_t index) const;
    std::string model;
    uint32_t speed;   // tiles per second x10
    Script script = Script::None;
    Building* bank = nullptr;     // the bank to smash into
    uint32_t laneY = 0;           // the lane to return to
    Car* chasing = nullptr;       // the police after the getaway car
    bool reckless = false;        // runs people over instead of honking
    bool gone = false;            // drove off the map
    bool loot = false;            // the getaway car's glovebox, until searched
    bool nearMissed = false;      // already honked at the player on this pass

    // who sits in it and whether it goes
    bool driverInside = true;
    bool driverDead = false;
    bool keysIn = false;
    bool engineOn = true;
    bool police = false;
    bool armoured = false;        // Vova's limousine: the doors do not open

    // --- free driving, once someone takes the car over ---
    bool freeMode = false;
    bool playerAtWheel = false;
    float px = 0, py = 0;         // centre of the car: x in cells, y in half rows (cells are twice as tall as wide)
    float headingF = 0;           // radians, clockwise from east
    int heading = 0;              // headingF snapped to one of the 72 directions
    float velocity = 0;           // half rows (= cell widths) per second, negative in reverse
    float rpm = 0;
    int gear = 0;                 // -1 reverse, 0 neutral, 1..4
    float steerPos = 0;           // the wheel: -1 full left .. +1 full right
    float throttlePos = 0;        // the pedal: 0..1
    bool clutchDown = false;
    float clutchBite = 1.0f;      // a let-go clutch bites in over a moment: 0 slipping .. 1 locked
    uint32_t crankMs = 0;         // the starter needs a breather between tries
    uint32_t grindMs = 0;         // the gearbox still crunching
    DriveControls controls;       // filled by the driver every frame

    // after being shot at: the driver floors it at the shooter, then gets away
    enum class Ai { None, Ram, Flee, Stop };
    Ai ai = Ai::None;
    Person* aiTarget = nullptr;
    Coord aiFrom { 0, 0 };
    uint32_t aiMs = 0;
    uint32_t aiStuckMs = 0;
    bool shotAt = false;          // the driver has already reacted to gunfire
    // a bullet hits the car: the driver dies, runs or tries to run the shooter over.
    // Returns whether the bodywork stopped the bullet.
    bool takeShot(Person* shooter, const Coord& at);

    // take the car over: the lane car becomes a free car at the same spot
    void makeFree();
    // one frame of physics for a free car, with or without anyone at the wheel
    void drive(uint32_t ms);
    // the cell the driver sits in
    Coord driverCell() const;
    // the cell next to the driver's door, or the nearest free one around the car
    bool exitCell(Coord& out) const;
    // the shift into `newGear` and its unavoidable gearbox crunch
    void changeGear(int newGear);
    // the engine cranks; may or may not start
    bool crank(std::string& msg);
    float kmh() const { return std::fabs(velocity) * 6.0f; }
    // standing still: nobody can jump into a moving car
    bool standing() const;
    uint32_t msSinceMoved = 0;    // lane cars: time since the last step along the lane
    // compass bearing of the nose in degrees, north 0, east 90
    int bearing() const { return (heading * (360 / HEADINGS) + 90) % 360; }
    static const char* gearName(int gear);

    virtual std::vector<Coord> cells() const;
private:
    // free car body as quadrant-block cells, rebuilt whenever it moves
    void rasterize();
    // what a panicking driver does with the pedals and the wheel this frame
    DriveControls aiControls(uint32_t ms);
    // the driver jumps out and runs from `danger`
    void bailOut(const Coord& danger, bool seen);
    // the body at a given position and heading, cells and their 2x2 sub-cell masks
    void rasterizeAt(float x, float y, int headingIdx, std::vector<Coord>& out, std::vector<uint8_t>* masks, std::vector<bool>* glass) const;
    // whether the body would fit at a position (no walls, buildings, other vehicles)
    bool fits(const std::vector<Coord>& body) const;
    std::vector<Coord> freeCells;
    std::vector<uint8_t> freeMasks;
    std::vector<bool> freeGlass;
    uint32_t hitCooldownMs = 0;
private:
    void checkNearMiss();
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

// walking into a car: get in, dragging out whoever is at the wheel if need be.
// Only a car standing still can be taken.
bool enterCar(PlayerNS::Player* player, Level* level, Car* car);
// the same for any car right next to the player
bool enterNearbyCar(PlayerNS::Player* player, Level* level);
// E at the wheel: get out (at speed, you jump)
bool leaveCar(PlayerNS::Player* player, Level* level);
// a drunk driver looks mostly ahead, but the eyes wander
void updateDriverGaze(PlayerNS::Player* player, uint32_t ms);
// whether the driver sees `to` from `from` (the cone of the gaze)
bool gazeSees(const PlayerNS::Player* player, const Coord& from, const Coord& to, uint32_t range);
// one line of gauges for the terminal: gear, revs, speed, wheel, clutch
std::string drivingHud(const Car* car);

#endif
