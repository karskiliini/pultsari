#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <cstdint>

enum Building {
    Alko,
    KRauta,
    Vankila,
    Divari
};

class Level {
public:
    Level() = default;
    Level(uint32_t x, uint32_t y) : sizex(x), sizey(y) {};
    ~Level() = default;

    void AddBuilding(Building building);


private:
    uint32_t sizex = 80;
    uint32_t sizey = 40;

};


#endif
