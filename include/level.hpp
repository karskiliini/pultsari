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

namespace PlayerNS{
    class Player;
};

class Level {
public:
    Level() = default;
    Level(uint32_t difficulty);
    ~Level();

    void buildingTurn();
    void npcTurn(Printer* printer);

    Person* getPerson(const Coord& coord) const;
    bool PersonExists(PersonType type) const;
    void removePerson(Person* person);
    void alertCops();
    void cleanDead();
    void cleanDiscardedItems();
    PlayerNS::Player* findPlayer();
    bool hit(const Coord& coord) const;
    Item* getItem(const Coord& c) const;
    Coord freePosition() const;
    Person* checkPerson(const Coord& c);

    void addBonas();
    bool addBuilding(Building* building);
    bool addPerson(Person* person);

    void addItem(ItemType item);
    void addItem(Item* item);
    void removeItem(Item* item);

    Person* raycast(const Coordinate<int>& from, const Coordinate<int>& vector) const;

    uint32_t sizex = common::SIZEX;
    uint32_t sizey = common::SIZEY;

    std::vector<Building*> buildings;
    std::vector<Person*> persons;
    std::vector<Item*> items;

    uint32_t stage = 1;
};


#endif
