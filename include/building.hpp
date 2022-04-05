#ifndef BUILDING_HPP
#define BUILDING_HPP

#include <cstdint>

enum BuildingType {
    Alko,
    KRauta,
    Vankila,
    Divari
};

class Building {
public:
    Building(BuildingType buildingType) : type(buildingType) { }

    BuildingType type;
    bool open = true;
};

#endif