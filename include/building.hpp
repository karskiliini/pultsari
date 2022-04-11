#ifndef BUILDING_HPP
#define BUILDING_HPP

#include "direction.hpp"
#include "coord.hpp"
#include <cstdint>
#include <string>

namespace PlayerNS {
    class Player;
};

enum BuildingType {
    EAlko,
    EIsku,
    EKRauta,
    EVankila,
    EDivari,
    EPoliisiAsema,
    EAsema,
    ESeina
};

class Level;

class Building {
public:
    Building(BuildingType buildingType);
    virtual ~Building() = default;

    virtual void npcAct();

    virtual std::string typeToChar(const Coord& c) const;
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);

    virtual std::string getWalkMsg() const;

    bool hitBuilding(const Coord& c) const;
    bool hitTopWall(const Coord& c) const;
    bool hitBotWall(const Coord& c) const;
    bool hitLeftWall(const Coord& c) const;
    bool hitRightWall(const Coord& c) const;
    bool hitWall(const Coord& c) const;
    bool hitDoor(const Coord& c) const;
    std::string printChar(const Coord& c) const;

    Coord getDoor() const;
    Coord getSpawn() const;

    BuildingType type;
    DirectionNS::Direction door = DirectionNS::Direction::up;
    bool hasDoor = true;
    uint32_t wallRight;
    uint32_t wallTop;
    uint32_t wallLeft;
    uint32_t wallBot;

    uint32_t turn = 0;
    Level* level;
};

class Alko : public Building {
public:
    Alko();
    virtual ~Alko() = default;
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
};

class Isku : public Building {
public:
    Isku();
    virtual ~Isku() = default;
    virtual std::string getName() const;
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
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
};

class KRauta : public Building {
public:
    KRauta();
    virtual ~KRauta() = default;
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
};

class Vankila : public Building {
public:
    Vankila();
    virtual ~Vankila() = default;
    void npcAct();
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
    bool emitted = false;
};

class PoliisiAsema : public Building {
public:
    PoliisiAsema();
    virtual ~PoliisiAsema() = default;
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
};

class Asema : public Building {
public:
    Asema();
    virtual ~Asema() = default;
    void npcAct();
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
};

class Seina : public Building {
public:
    Seina();
    virtual ~Seina() = default;
    virtual std::string getName() const;
    virtual std::string getWalkMsg() const;
};

#endif