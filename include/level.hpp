#ifndef LEVEL_HPP
#define LEVEL_HPP

#include "item.hpp"
#include <cstdint>
#include <vector>

class Person;
class Building;
class Item;

class Level {
public:
    Level() = default;
    Level(uint32_t difficulty);
    ~Level();

    void npcTurn();

    void cleanDead();
    bool hit(uint32_t x, uint32_t y) const;
    void freePosition(uint32_t& x, uint32_t& y) const;
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
