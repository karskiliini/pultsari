#ifndef PERSONS_HPP
#define PERSONS_HPP

#include "persontype.hpp"
#include "coord.hpp"
#include "direction.hpp"
#include "item.hpp"
#include "printer.hpp"
#include "stats.hpp"
#include <cstdint>
#include <string>

class Level;

namespace PathNS {
    class PathMask;
}

class Person {
public:
    Person(PersonType personType, const Coord& pos) : type(personType), coord(pos) { }
    virtual ~Person() = default;

    virtual void powerup(uint32_t damage);
    virtual void damage(uint32_t damage);

    virtual DirectionNS::Direction getMoveDirection(const Coord& target) const;
    virtual void pathmaskHook(PathNS::PathMask* pathmask) const { };
    virtual void npcAct() = 0;
    virtual bool interact(std::string& msg, Person* source) { return false; };
    virtual bool interactThrow(Item* item, Person* source, std::string& msg) = 0;
    virtual bool interactpuke(Person* source) { return false; };
    virtual std::string typeToChar() const { return "x"; };
    virtual Item* dropItem() { return nullptr; };
    virtual bool throwItem() { return false; };
    void setLevel(Level* level) { Person::level = level; }

    bool canSeeTarget(const Coord& target) const;
    DirectionNS::Direction randomDirection() const;
    void say(const std::string& text);

    Level* level;
    Printer* printer;
    PersonType type = invalidPerson;
    Coord coord = { 30, 20 };
    Stats stats;
    bool pathFinding = false;

    uint32_t visionRange = 0;
    bool blind = false;
    bool hasRadio = false;
    // show an NPC's message only if the player can see or hear this person
    void report(const std::string& msg);
    // what this NPC knows from its own eyes, ears or radio
    Person* chaseTarget = nullptr;
    bool knowsPutinDead = false;
    Person* currentTarget();
    bool alerted = false;
    Coord lastKnownPos = { 0, 0 };
    uint32_t alertCooldown = 0;
};

class Mummo : public Person {
public:
    Mummo(const Coord& pos);
    virtual ~Mummo() = default;
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    bool move(DirectionNS::Direction d, std::string& msg);
    bool reactedToUniform = false;
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual bool interactpuke(Person* source);
    Item* dropItem();
    virtual std::string typeToChar() const { return "M"; };
};

class Cop : public Person {
public:
    Cop(const Coord& pos);
    virtual ~Cop() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void pathmaskHook(PathNS::PathMask* pathmask) const;
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual bool interactpuke(Person* source);
    Item* dropItem();
    virtual std::string typeToChar() const { return "C"; };
};

class Varas : public Person {
public:
    Varas(const Coord& pos);
    virtual ~Varas() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void pathmaskHook(PathNS::PathMask* pathmask) const;
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "v"; };
    Coord target;
};

class Vanki : public Person {
public:
    Vanki(const Coord& pos);
    virtual ~Vanki() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void pathmaskHook(PathNS::PathMask* pathmask) const;
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "E"; };
    Coord target;
    bool kivi = false;
    bool firstMove = true;
};

class Skinhead : public Person {
public:
    Skinhead(const Coord& pos);
    virtual ~Skinhead() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "O"; };
    bool firstMove = true;
};

class Yka : public Person {
public:
    Yka(const Coord& pos);
    virtual ~Yka() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "Y"; };

    bool puhallus = false;
    uint32_t kaljaKesken = 0;
};

class Jengi : public Person {
public:
    Jengi(const Coord& pos);
    virtual ~Jengi() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "g"; };
    bool firstMove = true;
};

class Ryostaja : public Person {
public:
    Ryostaja(const Coord& pos);
    virtual ~Ryostaja() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    Item* dropItem();
    virtual std::string typeToChar() const { return "R"; };
    bool armed = true;
};

class Diileri : public Person {
public:
    Diileri(const Coord& pos);
    virtual ~Diileri() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "D"; };
    bool offered = false;
};

class Avustaja;

class Veteraani : public Person {
public:
    Veteraani(const Coord& pos);
    virtual ~Veteraani() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const;
    Item* dropItem();
    bool firstMove = true;
    bool aggressive = false;
    uint32_t rageCooldown = 0;
    uint32_t grenades = 2;
    uint32_t ammo = 6;
    // a state-funded personal assistant, permanently blind drunk
    Avustaja* assistant = nullptr;
private:
    void lookAfterAssistant(std::string& msg);
};

class Avustaja : public Person {
public:
    Avustaja(const Coord& pos);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return collapsed ? "_" : "a"; };
    bool collapsed = false;
};

class Juoppo : public Person {
public:
    Juoppo(const Coord& pos);
    virtual ~Juoppo() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "J"; };
    uint32_t stumbleCount = 0;
    std::string name;
};

class FSBAgent : public Person {
public:
    FSBAgent(const Coord& pos);
    virtual ~FSBAgent() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "F"; };
    bool firstMove = true;
    bool hasTarget = false;
    Coord defenTarget = { 0, 0 };
    uint32_t defenCooldown = 0;
};

class Sotilas : public Person {
public:
    Sotilas(const Coord& pos);
    virtual ~Sotilas() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "S"; };
    bool firstMove = true;
    uint32_t ammo = 4;
};

class ITYksikko : public Person {
public:
    ITYksikko(const Coord& pos);
    virtual ~ITYksikko() = default;

    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "A"; };
    uint32_t fireCooldown = 0;
};

class Vartija : public Person {
public:
    Vartija(const Coord& pos);
    virtual ~Vartija() = default;

    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "W"; };
    Coord homePos;
    bool fleeing = false;
private:
    // Kremlin guards carry radios (hasRadio) and react to what they see, hear or are told
    void radioGuardAct();
    void stepTo(const Coord& target);
};

class Putin : public Person {
public:
    Putin(const Coord& pos);
    virtual ~Putin() = default;

    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "P"; };
    bool playingWithDolls = false;
};

// Rectangle inside the Kremlin that the inner circle never leaves.
struct KremlinArea {
    Coord min;
    Coord max;
    bool contains(const Coord& c) const {
        return c.x >= min.x && c.x <= max.x && c.y >= min.y && c.y <= max.y;
    }
};

// Drunk, raving like a mad dog.
class Medvedev : public Person {
public:
    Medvedev(const Coord& pos, const KremlinArea& area);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "D"; };
private:
    KremlinArea area;
};

// Shoigu and Gerasimov: plan at the map table, curse at the map, and go to
// Putin to beg and lie that everything is going to plan.
class Kenraali : public Person {
public:
    Kenraali(const Coord& pos, PersonType type, const KremlinArea& area);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return type == shoigu ? "\xC5\xA0" : "G"; };
    std::string name() const { return type == shoigu ? "\xC5\xA0oigu" : "Gerasimov"; }
private:
    enum class State { AtMap, ToPutin, Begging, Back };
    bool stepTowards(const Coord& target);
    KremlinArea area;
    Coord home;
    State state = State::AtMap;
    uint32_t timer = 0;
};

#endif