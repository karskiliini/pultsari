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

namespace PlayerNS {
    class Player;
}

namespace PathNS {
    class PathMask;
}

// how tall a character is, for line of sight: low ones are seen over
constexpr uint32_t HEIGHT_FLAT = 0;      // a rat, someone lying on the ground
constexpr uint32_t HEIGHT_LOW = 1;       // a one-legged veteran, a crawling drunk
constexpr uint32_t HEIGHT_STANDING = 2;

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
    // not drawn at all: a rifleman lying low in his trench far across no man's land
    virtual bool hiddenFromView() const { return false; }
    void setLevel(Level* level) { Person::level = level; }

    bool canSeeTarget(const Coord& target) const;
    // the dead lie flat; the eyes are at the top of the head
    virtual uint32_t height() const { return stats.health > 0 ? HEIGHT_STANDING : HEIGHT_FLAT; }
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
    // bottle thieves: what they took (as it was in your hands), and how long a brawler keeps at you
    uint32_t stolenBottles = 0;
    std::vector<Held> stolenItems;
    uint32_t brawlActions = 0;
    bool hasLoot() const { return stolenMoney > 0 || stolenBottles > 0 || !stolenItems.empty(); }
    std::string lastHurtBy;     // the player: who or what hurt you last
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
    // sent by a burglar alarm: grabs the burglar when next to him
    bool arrests = false;
    // radios in a patrol if the chase drags on past COP_BACKUP_TURNS (once)
    bool callsBackup = false;
    bool backupCalled = false;
    uint32_t chaseTurns = 0;
    // which way he is walking his beat
    DirectionNS::Direction beat = DirectionNS::none;
};

// turns a radio cop chases without laying hands on the target before calling for backup
constexpr uint32_t COP_BACKUP_TURNS = 6;
// how likely an alarm cop next to the burglar gets him in cuffs this turn (percent)
constexpr int COP_ARREST_PERCENT = 40;

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
    // sitting on the pavement with one leg, he hides only what's right behind him
    virtual uint32_t height() const { return (oneLegged && stats.health > 0) ? HEIGHT_LOW : Person::height(); }
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
    virtual uint32_t height() const { return collapsed ? HEIGHT_FLAT : Person::height(); }
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

class Car;

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
    // jumped out of a black car to grab the player for the front; gives up after `patience` acts
    bool recruiter = false;
    uint32_t patience = 0;
    bool holding = false;         // has the player by the collar and drags him to the car
    bool gaveUp = false;          // lost him: waits to get back in the car
    uint32_t stun = 0;            // acts lost after a punch or a bullet
    uint32_t lastHealth = 0;      // to notice being hurt between acts
    void recruitAct();
    // pull the held player one step towards the car's door; in through it once he is there
    void drag(Car* car);
    // the player fights back against a man come to take him to the front
    bool recruiterHit(std::string& msg, Person* source);
    bool hasTarget = false;
    Coord defenTarget = { 0, 0 };
    uint32_t defenCooldown = 0;
};

class Sotilas : public Person {
public:
    // an officer (S) is a soldier (s) with more to lose: tougher, a pistol on his belt
    Sotilas(const Coord& pos, bool officer = false);
    virtual ~Sotilas() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return officer ? "S" : "s"; };
    std::string who() const { return officer ? "Upseeri" : "Sotilas"; }
    // the corpse's belt (sometimes a coat; an officer's pistol), once
    Item* dropItem();
    // a rank-and-file soldier is drunk, high and has nothing to lose; an officer keeps his head
    virtual uint32_t height() const { return passedOut ? HEIGHT_FLAT : Person::height(); }
    // a burst of 2-3 badly aimed rounds (an officer: one aimed shot); misses may find a bystander
    void fire(Person* target, std::string& msg);
    bool looted = false;
    bool firstMove = true;
    bool officer = false;
    uint32_t ammo = 12;
    uint32_t rage = 0;        // hit, pushed or stared at: hits and shoots for a while
    uint32_t passedOut = 0;   // actions left lying in a stupor
    uint32_t satisfied = 0;   // actions left busy with the player's bottle
    bool demanded = false;    // has asked for the bottle
private:
    bool wantsBottle(PlayerNS::Player* p, std::string& msg);
    bool drunkAct(std::string& msg);
    void calmedByOfficer();
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

// A Ukrainian soldier across no man's land. Lying low in his trench (Lurk) he is never seen,
// only his muzzle flash; he takes his time over a shot at anyone up on the open ground, and
// the longer the man has been up there the readier he is. In the assault he comes over the
// top with his AK, jumps into our trench and rolls it up.
class Ukrainalainen : public Person {
public:
    enum class Mode { Lurk, Assault };
    Ukrainalainen(const Coord& pos, Mode mode = Mode::Lurk);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "u"; }
    virtual bool hiddenFromView() const { return mode == Mode::Lurk && flash == 0 && stats.health > 0; }
    // one aimed rifle shot at `target` from the trench; `msg` for the player
    void snipe(Person* target, std::string& msg);
    // a short AK burst at close range
    void burst(Person* target, std::string& msg);
    Mode mode;
    uint32_t flash = 0;         // NPC turns the muzzle flash still shows
    bool firstMove = true;
    uint32_t ammo = 60;
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

// Drunk, raving like a mad dog. Every rant about nukes winds him up further,
// until he foams at the mouth, posts it all to Telegram and passes out.
class Medvedev : public Person {
public:
    Medvedev(const Coord& pos, const KremlinArea& area);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "D"; };

    static constexpr uint32_t MAX_FROTH = 10;
    uint32_t froth = 0;        // how wound up he is; foams from FOAM_AT up
    uint32_t passedOut = 0;    // turns left lying in his own foam after a Telegram post
    static constexpr uint32_t FOAM_AT = 5;

    // the latest post on his Telegram channel; people on the street read it out
    static std::string latestPost;
    static std::string randomPost();
private:
    void rant();
    void foam();
    KremlinArea area;
    uint32_t talkCooldown = 0;
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
    // run away from `danger` for a while, panic or not
    void fleeFrom(const Coord& danger);
    std::string name;
    std::string job;
private:
    void pickDestination();
    uint32_t running = 0;
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
    virtual uint32_t height() const { return HEIGHT_FLAT; }
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

// what a shopkeeper grabs when he sees you steal: a stapler mostly, a knife now and
// then, rarely the pistol from under the counter
enum class SellerWeapon { Nitoja, Veitsi, Pistooli };
// 75 / 20 / 5 %
SellerWeapon rollSellerWeapon();

// the fanatical shopkeeper: he runs after a thief down the street until one of them
// is dead (or the level changes), and uses whatever he has in his hand without mercy
class Myyja : public Person {
public:
    Myyja(const Coord& pos, SellerWeapon weapon);
    virtual ~Myyja() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "m"; };
    // the knife or the pistol stays on the street; a stapler is not worth picking up
    Item* dropItem();
    SellerWeapon weapon;
    uint32_t ammo = 8;
    bool looted = false;
};

#endif