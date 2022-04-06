#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <cstdint>
#include <vector>
#include "persons.hpp"
#include "building.hpp"
#include "item.hpp"

class Level {
public:
    Level() = default;
    Level(uint32_t difficulty);
    ~Level();

    bool hit(uint32_t x, uint32_t y) const;

    void addBonas();
    bool addBuilding(Building& building);
    bool addPerson(Person& person);
    void addItem(Item* item);
    void removeItem(Item* item);

    uint32_t sizex = 80;
    uint32_t sizey = 28;

    std::vector<Building*> buildings;
    std::vector<Person*> persons;
    std::vector<Item*> items;

    uint32_t turn = 0;
    uint32_t stage = 1;
};


#endif
