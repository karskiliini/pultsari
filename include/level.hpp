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

class Level {
public:
    Level() = default;
    Level(uint32_t difficulty);
    ~Level();

    void buildingTurn();
    void npcTurn();
    // real time: every NPC acts when its own initiative has built up enough energy
    void npcFrame(uint32_t ms);
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
    // the horizontal and vertical road at a cell (either may be null)
    void roadsAt(const Coord& c, const Road*& h, const Road*& v) const;
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

    AudioSystem audio;
};


#endif
