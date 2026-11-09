#ifndef LEVEL_HPP
#define LEVEL_HPP

#include "item.hpp"
#include "coord.hpp"
#include "common.hpp"
#include "persontype.hpp"
#include "audio.hpp"
#include <cstdint>
#include <vector>

class Vehicle;
class EnvironmentEvents;

class Person;
class Building;
class Item;
class Printer;
class Animation;

namespace PlayerNS{
    class Player;
};

// A three-tile-wide street across the whole map; the middle row/column is the centre line.
struct Road {
    bool horizontal;
    uint32_t pos;       // top row (horizontal) or left column (vertical)
    std::string name;
    bool contains(const Coord& c) const {
        const uint32_t v = horizontal ? c.y : c.x;
        return v >= pos && v <= pos + 2;
    }
};

// Something flying over the city. Harmless flyovers just buzz; a drone with a
// target dives into that building.
struct Drone {
    float x, y;
    float vx, vy;       // tiles per second
    Building* target = nullptr;
    float turn = 0;     // flyovers: radians per second of slow bank
    float age = 0;      // seconds in the air; a homing drone turns harder the longer it flies
};

// A puff of gas drifting on the wind: Lada exhaust or toxic refinery smoke.
struct GasPuff {
    float x, y;
    uint32_t ttl;       // world ticks
    bool toxic;
    bool smoke = false; // dark smoke from a fire, carried by the wind
};

class Car;

// A bank robbery in progress: the getaway car, the police and how far along it is.
struct Robbery {
    enum class Stage { Idle, Arriving, Robbing, Returning, Fleeing };
    Stage stage = Stage::Idle;
    Car* car = nullptr;
    Car* police = nullptr;
    Building* bank = nullptr;
    uint32_t timer = 0;
    uint32_t cooldown = 300;  // world ticks until the next robbery (about two minutes)
    uint32_t boarded = 0;
    bool policeParked = false;
};

class Level {
public:
    Level() = default;
    Level(uint32_t difficulty);
    ~Level();

    void buildingTurn();
    void npcTurn();
    // real time: every NPC acts when its own initiative has built up enough energy
    void npcFrame(uint32_t ms);
    uint32_t exhaustMs = 0;   // cooldown between exhaust sniffs
    // a name for messages, e.g. "Gopnik" or a drunk's own name
    std::string nameOf(const Person* p) const;
    void actThrow(Person* source);
    void playAnimation();

    Person* createPerson(const Coord& coord, PersonType type);

    Person* getPerson(const Coord& coord) const;
    PlayerNS::Player* getPlayer() const;
    bool PersonExists(PersonType type) const;
    void removePerson(Person* person);
    // --- perception: NPCs act only on what they see, hear or get over the radio ---
    bool isAlive(const Person* p) const;
    bool isLit(const Coord& c) const;
    // wall crossings on the straight line between two points; Kremlin walls count double
    uint32_t wallsBetween(const Coord& a, const Coord& b) const;
    // speech does not carry through walls; loud noises do, a third as far per wall
    bool canHear(const Coord& listener, const Coord& source, uint32_t range, bool loud = false) const;
    // radios carry further and through walls better, but not through everything
    bool radioReaches(const Coord& from, const Coord& to) const;
    // security forces who see or hear `source` (within `range`) start chasing it
    void alertCops(Person* source, uint32_t range = 12);
    // `caller` shouts (or radios) for help with `target`; hostile makes responders attack it
    void callForHelp(Person* caller, Person* target, bool hostile);
    // people who know Putin is dead shout it; whoever hears them learns it too
    void spreadNews();
    void witnessPutinDeath(const Coord& where);
    void cleanDead();
    void cleanDiscardedItems();

    bool hitBuilding(const Coord& c) const;
    // the building covering a cell, from a grid rebuilt whenever buildings are added
    Building* buildingAt(const Coord& c) const;
    mutable std::vector<Building*> buildingGrid;
    mutable size_t buildingGridCount = (size_t)-1;
    bool hitWall(const Coord& c) const;
    bool hitWindow(const Coord& c) const;
    bool hitPerson(const Coord& c) const;
    bool hitItem(const Coord& c) const;
    bool hitLOSBlocker(const Coord& c) const;
    bool blocksLOS(const Coord& c) const;
    bool hit(const Coord& coord) const;

    Item* getItem(const Coord& c) const;
    Coord freePosition() const;
    Person* checkPerson(const Coord& c);

    void addBonas();
    bool addBuilding(Building* building);
    bool addPerson(Person* person);

    void addItem(ItemType item);
    void addItem(Item* item);
    // used syringes by dealers, drunks, metro entrances and bushes
    void scatterNeedles();
    void addThrownItem(Item* item);
    void addAnimation(Animation* animation, bool play = true);
    void removeItem(Item* item);

    void addVehicle(Vehicle* v);
    void updateVehicles();
    Vehicle* getVehicleAt(const Coord& c) const;

    bool raycast(Coord from, const Coord& to) const;
    Person* raycast(const Coordinate<int>& from, const Coordinate<int>& vector) const;
    Building* raycastBuilding(const Coordinate<int>& from, const Coordinate<int>& vector) const;

    uint32_t sizex = common::WORLD_W;
    uint32_t sizey = common::WORLD_H;
    int32_t cameraX = 0;
    int32_t cameraY = 0;
    void updateCamera();

    std::vector<Building*> buildings;
    std::vector<Person*> persons;
    std::vector<Item*> items;
    std::vector<Vehicle*> vehicles;
    std::vector<Road> roads;
    std::vector<Drone> drones;
    std::vector<GasPuff> gas;
    // the wind: never shown, but it carries the smoke; it veers slowly and now and then swings round
    float windAngle = 0.3f;
    float windSpeed = 0.35f;     // cells per world tick
    uint32_t nearStrikeCooldown = 120;
    void addSmoke(const Coord& c);
    // a drone over the city: a flyover, or a strike on `target`
    void launchDrone(Building* target);
    // world tick: spawn flyovers, drift and burn gas, let gas affect people
    void skyTick();
    void addGas(const Coord& c, bool toxic);
    // 0 none, 1 exhaust, 2 toxic
    int gasAt(const Coord& c) const;
    void droneImpact(const Coord& c, Building* target);
    Robbery robbery;
    // world tick: start, run and finish bank robberies
    void robberyTick();
    // A bullet from `shooter` along `angle` (radians): walls stop it; each person in
    // the way is hit with `hitPercent` chance, else it flies on. Returns who was hit.
    Person* fireBullet(Person* shooter, float angle, uint32_t range, uint32_t damage, uint32_t hitPercent);
    std::vector<uint8_t> gasGrid;   // per-cell gas kind, rebuilt every world tick
    // the horizontal and vertical road at a cell (either may be null)
    void roadsAt(const Coord& c, const Road*& h, const Road*& v) const;
    // where a cell is, for the title bar: the building in or next to, else the street; "" if nowhere named
    std::string locationAt(const Coord& c) const;
    // cars move on their own clock, every frame
    void vehicleFrame(uint32_t ms);
    Item* thrownItem = nullptr;
    std::vector<Animation*> animations;
    void stepAnimations();
    void explode(const Coord& c, uint32_t radius, uint32_t maxDamage, Person* source);

    Printer* printer = nullptr;
    EnvironmentEvents* events = nullptr;

    uint32_t stage = 1;
    bool pathFinding = false;
    bool isWarLevel = false;
    bool isBolshoi = false;
    bool putinDead = false;
    bool celebrating = false;
    // the player is dead and the world just carries on: nobody notices the body
    bool playerDead = false;

    AudioSystem audio;
};


#endif
