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
    ERefinery,
    EBank,
    EShop,
    EMetro,
    EFlats,
    EGasStation,
    EGasPump
};

class Level;
class Person;

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
    // walking into a wall cell from outside; true if the building handled it (msg set)
    virtual bool bumpWall(PlayerNS::Player* player, const Coord& c, std::string& msg) { return false; }

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
    // people can walk around inside (through the door), like the Kremlin or a bank
    bool walkable = false;
    // wall cells smashed open (e.g. by a car), now passable rubble
    std::vector<Coord> breaches;
    bool isBreach(const Coord& c) const;
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
    virtual bool bumpWall(PlayerNS::Player* player, const Coord& c, std::string& msg);
    // the bottle return hatch on the east wall
    Coord getHatch() const { return Coord{ wallRight, wallBot - 2 }; }
    // bottles handed in, waiting for the receipt
    uint32_t hatchBottles = 0;
    void startFire();
    bool onFire = false;
    uint32_t fireTurn = 0;
    std::vector<Coord> fireSpots;
    std::vector<Coord> smokeSpots;
};

// A Western shop that pulled out of Russia in 2022 and never came back.
class SuljettuLiike : public Building {
public:
    // frontSouth: the shop window faces south (the shop stands north of the street)
    SuljettuLiike(const std::string& name, const std::string& sign, bool frontSouth = false);
    virtual std::string getName() const { return name; }
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
    virtual void npcAct();
    // a stone through the shop window: glass, alarm, maybe loot, police later
    void smashWindow(const Coord& c, Person* source);
    bool smashed = false;
    uint32_t alarmTicks = 0;
    Person* burglar = nullptr;
private:
    std::string name;
    std::string sign;
};

// A Russian shop that is open: a few things for sale, a rude seller and an angry queue.
// Oruzhie is the gun shop: knives, pistols, rounds, grenades and a Kalashnikov over the counter.
class Kauppa : public Building {
public:
    enum class Kind { Produkty, Kioski, Oruzhie };
    Kauppa(const std::string& name, Kind kind);
    virtual std::string getName() const { return name; }
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
private:
    std::string name;
    Kind kind;
};

// A metro station entrance. The fare is 60 roubles, or you jump the turnstile and hope
// nobody saw. Every trip to the next station is a mini-game (metro.hpp): this only takes
// the fare and leaves the trip for the main loop to play.
class Metro : public Building {
public:
    explicit Metro(const std::string& station);
    virtual std::string getName() const { return "\xD0\x9C \xD0\x9C\xD0\x95\xD0\xA2\xD0\xA0\xD0\x9E"; } // "М МЕТРО"
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
    // the next station on the line, which runs through every station on the level and loops
    Metro* nextOnLine() const;
    std::string station;
};

// A petrol station kiosk: hot dogs and beer, and outside people complain about the prices.
class Bensa : public Building {
public:
    Bensa();
    virtual std::string getName() const { return "\xD0\x90\xD0\x97\xD0\xA1 \xC2\xB7 95"; } // "АЗС · 95"
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual void interact(PlayerNS::Player* player, std::string& msg);
    virtual std::string getWalkMsg() const;
    virtual void npcAct();
    uint32_t grumbleTicks = 0;
    bool outOfFuel = false;   // then the queue of cars stretches down the road
};

// A petrol pump in front of the kiosk.
class Pumppu : public Building {
public:
    Pumppu();
    virtual std::string typeToChar(const Coord& c) const;
    virtual std::string getWalkMsg() const;
};

// A grey block of flats filling up the city: no way in, graffiti on the walls.
class Kerrostalo : public Building {
public:
    Kerrostalo(const std::string& name, uint32_t w, uint32_t h);
    virtual std::string getName() const { return name; }
    virtual std::string getWalkMsg() const;
private:
    std::string name;
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

// Sberbank on Tverskaja: open to walk in, and to rob.
class Pankki : public Building {
public:
    Pankki();
    virtual std::string getName() const { return "\xD0\xA1\xD0\x91\xD0\x95\xD0\xA0\xD0\x91\xD0\x90\xD0\x9D\xD0\x9A"; }
    virtual bool getEnterMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual bool getInteractMsg(PlayerNS::Player* player, std::string& msg) const;
    virtual std::string getWalkMsg() const;
    Coord robberyWindow() const { return Coord{ wallLeft + 4, wallBot }; }
};

#endif