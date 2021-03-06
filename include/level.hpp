#ifndef LEVEL_HPP
#define LEVEL_HPP

#include "item.hpp"
#include "coord.hpp"
#include "common.hpp"
#include "persontype.hpp"
#include <cstdint>
#include <vector>

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
    bool hitPerson(const Coord& c) const;
    bool hitItem(const Coord& c) const;
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

    bool raycast(Coord from, const Coord& to) const;
    Person* raycast(const Coordinate<int>& from, const Coordinate<int>& vector) const;
    Building* raycastBuilding(const Coordinate<int>& from, const Coordinate<int>& vector) const;

    uint32_t sizex = common::SIZEX;
    uint32_t sizey = common::SIZEY;

    std::vector<Building*> buildings;
    std::vector<Person*> persons;
    std::vector<Item*> items;
    Item* thrownItem = nullptr;
    Animation* animation = nullptr;

    Person* attack = nullptr;
    Printer* printer = nullptr;

    uint32_t stage = 1;
    bool pathFinding = false;
};


#endif
