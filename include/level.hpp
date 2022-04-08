#ifndef LEVEL_HPP
#define LEVEL_HPP

#include "item.hpp"
#include "coord.hpp"
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

    void npcTurn(Printer* printer);

    Person* getPerson(uint32_t x, uint32_t y) const;
    Person* getPerson(const Coord& coord) const;
    void removePerson(Person* person);
    void alertCops();
    void cleanDead();
    PlayerNS::Player* findPlayer();
    bool hit(uint32_t x, uint32_t y) const;
    bool hit(const Coord& coord) const;
    void freePosition(uint32_t& x, uint32_t& y) const;
    Coord freePosition() const;
    Person* checkPerson(uint32_t checkx, uint32_t checky);

    void addBonas();
    bool addBuilding(Building* building);
    bool addPerson(Person* person);

    void addItem(ItemType item);
    void addItem(Item* item);
    void removeItem(Item* item);

    uint32_t sizex = 80;
    uint32_t sizey = 28;

    std::vector<Building*> buildings;
    std::vector<Person*> persons;
    std::vector<Item*> items;

    uint32_t stage = 1;
};


#endif
