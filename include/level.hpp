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

    bool AddBuilding(Building& building);
    bool AddPerson(Person& person);

private:
    uint32_t sizex = 80;
    uint32_t sizey = 40;

    std::vector<Building> buildings;
    std::vector<Person> persons;
};


#endif
