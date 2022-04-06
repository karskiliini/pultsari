#ifndef BUILDING_HPP
#define BUILDING_HPP

#include "direction.hpp"
#include <cstdint>
#include <string>

enum BuildingType {
    EAlko,
    EKRauta,
    EVankila,
    EDivari
};

class Building {
public:
    Building(BuildingType buildingType) : type(buildingType) { }

    virtual char typeToChar(uint32_t x, uint32_t y) const { return ' '; };
    virtual std::string getName() const { return ""; }

    virtual std::string getWalkMsg() const { return ""; }

    bool hitTopWall(uint32_t x, uint32_t y) const {
        return ((x >= wallLeft) && (x <= wallRight) && (y == wallTop));
    }

    bool hitBotWall(uint32_t x, uint32_t y) const {
        return ((x >= wallLeft) && (x <= wallRight) && (y == wallBot));
    }

    bool hitLeftWall(uint32_t x, uint32_t y) const {
        return ((x == wallLeft) && (y >= wallTop) && (y <= wallBot));
    }

    bool hitRightWall(uint32_t x, uint32_t y) const {
        return ((x == wallRight) && (y >= wallTop) && (y <= wallBot));
    }

    bool hitWall(uint32_t x, uint32_t y) const {
        if (hitTopWall(x, y) ||
            hitRightWall(x, y) ||
            hitBotWall(x, y) ||
            hitLeftWall(x, y))
        {
            return true;
        }
        return false;
    }

    bool hitDoor(uint32_t x, uint32_t y) const {
        if (hitWall(x, y))
        {
            if ((x >= wallLeft) && (x <= wallRight) && ((y == wallTop) || (y == wallBot))) {
                if (door == DirectionNS::Direction::up)
                {
                    // door top
                    const uint32_t mid = ((wallRight + wallLeft) / 2);
                    if ((x >= mid-1) && (x <= mid+1) && (y == wallTop)) return true;
                } else if (door == DirectionNS::Direction::down)
                {
                    // door bot
                    const uint32_t mid = ((wallRight + wallLeft) / 2);
                    if ((x >= mid-1) && (x <= mid+1) && (y == wallBot)) return true;
                }
            }
            else if (((x == wallLeft) || (x == wallRight)) && (y >= wallTop) && (y <= wallBot)) {
                if ((door == DirectionNS::Direction::left) || (door == DirectionNS::Direction::right))
                {
                    if (door == DirectionNS::Direction::left)
                    {
                        // special case
                        const uint32_t mid = ((wallTop + wallBot) / 2);
                        if ((y >= mid-1) && (y <= mid+1) && (x == wallLeft)) return true;
                    }
                    else if (door == DirectionNS::Direction::right)
                    {
                        // special case
                        const uint32_t mid = ((wallTop + wallBot) / 2);
                        if ((y >= mid-1) && (y <= mid+1) && (x == wallRight)) return true;
                    }

                }
            }
        }
        return false;
    }

    char printChar(uint32_t x, uint32_t y) const
    {
        if (hitDoor(x, y)) return '#';
        if (hitWall(x, y)) return '=';

        if (y == (wallTop + wallBot) / 2)
        {
            const auto name = getName();
            const auto len = name.length();
            const uint32_t mid = ((wallRight + wallLeft) / 2);
            const uint32_t left = mid - len/2;

            if ((x >= left) && (x < (left + len))) {
                const auto index = x - left;
                return name[index];
            }
        }

        return ' ';
    }

    BuildingType type;
    bool open = true;
    DirectionNS::Direction door = DirectionNS::Direction::up;
    uint32_t wallRight;
    uint32_t wallTop;
    uint32_t wallLeft;
    uint32_t wallBot;
};

class Alko : public Building {
public:
    Alko() : Building(BuildingType::EAlko) {
        wallRight = 20;
        wallTop = 20;
        wallLeft = 0;
        wallBot = 27;

        door = DirectionNS::up;
    }

    virtual ~Alko() = default;
    virtual std::string getName() const { return "ALKO"; }
    virtual char typeToChar(uint32_t x, uint32_t y) const
    {

        return printChar(x, y);
    }
    virtual std::string getWalkMsg() const { return "Isket paasi alkon seinaan. Mielenkiintoista."; }
};


class Divari : public Building {
public:
    Divari() : Building(BuildingType::EDivari) {
        wallRight = 20;
        wallTop = 0;
        wallLeft = 0;
        wallBot = 8;

        door = DirectionNS::down;
    }
    virtual ~Divari() = default;
    virtual std::string getName() const { return "DIVARI"; }
    virtual char typeToChar(uint32_t x, uint32_t y) const
    {
        return printChar(x, y);
    }
    virtual std::string getWalkMsg() const { return "Divarin seinä on vankkumaton,vaikka kuinka yrität kävellä sitä päin."; }

};

#endif