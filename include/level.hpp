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

class Level {
public:
    Level() = default;
    Level(uint32_t difficulty);
    ~Level();

    void buildingTurn();
    void npcTurn();
    void actThrow(Person* source);
    void playAnimation();

    Person* createPerson(const Coord& coord, PersonType type);

    Person* getPerson(const Coord& coord) const;
    PlayerNS::Player* getPlayer() const;
    bool PersonExists(PersonType type) const;
    void removePerson(Person* person);
    void alertCops(Person* source);
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
    Item* thrownItem = nullptr;
    std::vector<Animation*> animations;
    void stepAnimations();
    void explode(const Coord& c, uint32_t radius, uint32_t maxDamage, Person* source);

    Person* attack = nullptr;
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
