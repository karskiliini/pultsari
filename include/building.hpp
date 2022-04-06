#ifndef BUILDING_HPP
#define BUILDING_HPP

#include "direction.hpp"
#include <cstdint>
#include <string>

namespace PlayerNS {
    class Player;
};

enum BuildingType {
    EAlko,
    EKRauta,
    EVankila,
    EDivari
};

class Building {
public:
    Building(BuildingType buildingType);

    virtual char typeToChar(uint32_t x, uint32_t y) const;
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);

    virtual std::string getWalkMsg() const;

    bool hitTopWall(uint32_t x, uint32_t y) const;
    bool hitBotWall(uint32_t x, uint32_t y) const;
    bool hitLeftWall(uint32_t x, uint32_t y) const;
    bool hitRightWall(uint32_t x, uint32_t y) const;
    bool hitWall(uint32_t x, uint32_t y) const;
    bool hitDoor(uint32_t x, uint32_t y) const;
    char printChar(uint32_t x, uint32_t y) const;

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
    Alko();
    virtual ~Alko() = default;
    virtual std::string getName() const;
    virtual char typeToChar(uint32_t x, uint32_t y) const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
};


class Divari : public Building {
public:
    Divari();
    virtual ~Divari() = default;
    virtual std::string getName() const;
    virtual char typeToChar(uint32_t x, uint32_t y) const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual std::string getWalkMsg() const;
};

#endif