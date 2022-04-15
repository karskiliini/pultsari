#ifndef PERSONS_HPP
#define PERSONS_HPP

#include "persontype.hpp"
#include "coord.hpp"
#include "direction.hpp"
#include "item.hpp"
#include "printer.hpp"
#include <cstdint>
#include <string>

class Level;

class Person {
public:
    Person(PersonType personType, const Coord& pos) : type(personType), coord(pos) { }
    virtual ~Person() = default;

    virtual void powerup(uint32_t damage);
    virtual void damage(uint32_t damage);

    virtual DirectionNS::Direction getMoveDirection(const Coord& target) const;
    virtual void npcAct() = 0;
    virtual bool interact(std::string& msg, Person* source) { return false; };
    virtual bool interactThrow(Item* item, Person* source, std::string& msg) = 0;
    virtual std::string typeToChar() const { return "x"; };
    virtual Item* dropItem() { return nullptr; };
    virtual bool throwItem(Printer& printer) { return false; };
    void setLevel(Level* level) { Person::level = level; }

    Level* level;
    Printer* printer;
    PersonType type = invalidPerson;
    Coord coord = { 30, 20 };
    uint32_t health = 1;
};

class Mummo : public Person {
public:
    Mummo(const Coord& pos);
    virtual ~Mummo() = default;
    virtual void npcAct() {};
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    Item* dropItem();
    virtual std::string typeToChar() const { return "M"; };
};

class Cop : public Person {
public:
    Cop(const Coord& pos);
    virtual ~Cop() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    Item* dropItem();
    virtual std::string typeToChar() const { return "C"; };
};

class Varas : public Person {
public:
    Varas(const Coord& pos);
    virtual ~Varas() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "V"; };
    Coord target;
};

class Vanki : public Person {
public:
    Vanki(const Coord& pos);
    virtual ~Vanki() = default;

    bool move(DirectionNS::Direction d, std::string& msg);
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

#endif