#ifndef BUILDING_HPP
#define BUILDING_HPP

#include "direction.hpp"
#include "coord.hpp"
#include <cstdint>
#include <string>
#include <vector>

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
    ESeina,
    EKremlin,
    EHypermarket,
    EClosedShop,
    ERefinery
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
    bool hitWindow(const Coord& c) const;

    Coord getDoor() const;
    Coord getSpawn() const;
    void setOffset(int32_t dx, int32_t dy);

    BuildingType type;
    DirectionNS::Direction door = DirectionNS::Direction::up;
    bool hasDoor = true;
    // a lit interior can be seen whole from inside and from further away outside
    bool lit = false;
    uint32_t wallRight;
    uint32_t wallTop;
    uint32_t wallLeft;
    uint32_t wallBot;

    Level* level;
    std::vector<Coord> windows;
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
    uint32_t turn = 0;
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
    uint32_t turn = 0;
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
    bool emitted = false;
    uint32_t turn = 0;
};

class Seina : public Building {
public:
    Seina();
    virtual ~Seina() = default;
    virtual std::string getName() const;
    virtual std::string getWalkMsg() const;
};

class Kremlin : public Building {
public:
    Kremlin();
    virtual ~Kremlin() = default;
    virtual std::string getName() const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
};

class Hypermarket : public Building {
public:
    Hypermarket();
    virtual ~Hypermarket() = default;
    void npcAct();
    virtual std::string getName() const;
    virtual std::string typeToChar(const Coord& c) const;
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
    void startFire();
    bool onFire = false;
    uint32_t fireTurn = 0;
    std::vector<Coord> fireSpots;
    std::vector<Coord> smokeSpots;
};

// A Western shop that pulled out of Russia in 2022 and never came back.
class SuljettuLiike : public Building {
public:
    SuljettuLiike(const std::string& name, const std::string& sign);
    virtual std::string getName() const { return name; }
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual std::string getWalkMsg() const;
private:
    std::string name;
    std::string sign;
};

// An oil refinery: drones love it. Once hit it burns for a long time and
// pours out toxic smoke.
class Jalostamo : public Building {
public:
    Jalostamo();
    virtual std::string getName() const { return "\xD0\x9D\xD0\x9F\xD0\x97"; }
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual std::string getWalkMsg() const;
    uint32_t burning = 0;   // world ticks left to burn
};

#endif