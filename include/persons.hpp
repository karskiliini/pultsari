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
#include <vector>

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
    // initiative: actions per second x10; energy builds up each frame until it is enough to act
    uint32_t speed = 20;
    uint32_t energy = 0;
    // pickpocketing: money taken from the player, and how many more actions to run away
    uint32_t stolenMoney = 0;
    uint32_t fleeActions = 0;
    // bottle thieves: what they took (stealable() indexes), and how long a brawler keeps at you
    uint32_t stolenBottles = 0;
    std::vector<int> stolenItems;
    uint32_t brawlActions = 0;
    bool hasLoot() const { return stolenMoney > 0 || stolenBottles > 0 || !stolenItems.empty(); }
    bool armor = false; // a bulletproof vest takes the sting out of most hits
    // individual clothing colour (xterm-256 SGR), overrides the per-type colour
    const char* clothing = nullptr;
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
    bool oneLegged = false;
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

// Foreign recruits a long way from home. The joke is on whoever brought them here.
class Varvatty : public Person {
public:
    Varvatty(const Coord& pos, PersonType type);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return type == korealainen ? "n" : "h"; }
private:
    void step(DirectionNS::Direction d);
    uint32_t talkCooldown = 0;
    DirectionNS::Direction march = DirectionNS::right;
    uint32_t marchLeft = 0;
    bool gaveRation = false;
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
    // a door guard can be lured off: he chases you only so far, and goes to look at noises
    bool chasing = false;
    uint32_t lostTicks = 0;
    Coord investigate { 0, 0 };
    uint32_t investigateTicks = 0;
    void hearNoise(const Coord& c);
    void provoke(Person* by);
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

// A housewife so drunk she understands nothing, and her foul-mouthed child.
class Kotiaiti : public Person {
public:
    Kotiaiti(const Coord& pos);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "K"; };
};

class Lapsi : public Person {
public:
    Lapsi(const Coord& pos);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "c"; };
    Kotiaiti* mother = nullptr;
private:
    void stepTowards(const Coord& target, bool away);
};

// An ordinary Muscovite with a name, a job, their own clothes and their own errands.
class Kansalainen : public Person {
public:
    Kansalainen(const Coord& pos);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return glyph; };
    std::string name;
    std::string job;
private:
    void pickDestination();
    std::string glyph;
    std::string jobLine;
    Coord destination;
    uint32_t panic = 0;
};

// A rat that lives in a trash pile and defends its patch.
class Rotta : public Person {
public:
    Rotta(const Coord& pos);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "r"; };
    Coord home;
};

// A bank robber in a balaclava with an assault rifle, drunk and hiccuping:
// sprays everyone inside the bank, rarely hits anything.
class Pankkirosvo : public Person {
public:
    enum class Phase { Raid, Return };
    Pankkirosvo(const Coord& pos, const Coord& goal);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "R"; };
    Phase phase = Phase::Raid;
    Coord goal;             // where to go: a spot in the bank, then the car
    bool boarded = false;   // back in the getaway car
private:
    void stepTowards(const Coord& target);
    void sprayFire();
};

#endif