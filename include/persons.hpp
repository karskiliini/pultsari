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

    virtual DirectionNS::Direction getMoveDirection(const Coord& target) const;
    virtual bool move(DirectionNS::Direction d);
    virtual void npcAct(std::string& msg) { msg = ""; }
    virtual bool interact(std::string& message, Person* source) { return false; };
    virtual void interactThrow(Item* item, Person* source, std::string& msg) { msg = "Osuit henkilöön, joka kupsahti."; health = 0; };
    virtual void damage(uint32_t damage);
    virtual std::string typeToChar() const { return "x"; };
    virtual Item* dropItem() { return nullptr; };
    virtual bool throwItem(Printer& printer) { return false; };
    void setLevel(Level* level) { Person::level = level; }

    Level* level;
    PersonType type = invalidPerson;
    Coord coord = { 30, 20 };
    uint32_t health = 1;
};

class Mummo : public Person {
public:
    Mummo(const Coord& pos);
    virtual ~Mummo() = default;
    virtual bool interact(std::string& message, Person* source);
    virtual void interactThrow(Item* item, Person* source, std::string& msg);
    Item* dropItem();
    virtual std::string typeToChar() const { return "M"; };
};

class Cop : public Person {
public:
    Cop(const Coord& pos);
    virtual ~Cop() = default;

    virtual bool move(DirectionNS::Direction d);
    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct(std::string& msg);
    virtual bool interact(std::string& message, Person* source);
    void interactThrow(Item* item, Person* source, std::string& msg);
    Item* dropItem();
    virtual std::string typeToChar() const { return "C"; };
};

class Varas : public Person {
public:
    Varas(const Coord& pos);
    virtual ~Varas() = default;

    virtual bool move(DirectionNS::Direction d);
    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct(std::string& msg);
    virtual bool interact(std::string& message, Person* source);
    virtual void interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "V"; };
    Coord target;
};

class Vanki : public Person {
public:
    Vanki(const Coord& pos);
    virtual ~Vanki() = default;

    virtual bool move(DirectionNS::Direction d);
    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct(std::string& msg);
    virtual bool interact(std::string& message, Person* source);
    virtual void interactThrow(Item* item, Person* source, std::string& msg);
    virtual std::string typeToChar() const { return "E"; };
    Coord target;
    bool kivi = false;
    bool firstMove = true;
};



#endif