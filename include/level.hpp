#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <cstdint>
#include <vector>
#include "persons.hpp"
#include "building.hpp"

class Level {
public:
    Level() = default;
    Level(uint32_t x, uint32_t y) : sizex(x), sizey(y) {};
    ~Level() = default;

    bool addBuilding(Building& building);
    bool addPerson(Person& person);

    uint32_t sizex = 80;
    uint32_t sizey = 28;

    std::vector<Building*> buildings;
    std::vector<Person*> persons;

    uint32_t turn = 0;
};


#endif
