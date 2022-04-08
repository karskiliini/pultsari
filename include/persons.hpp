#ifndef PERSONS_HPP
#define PERSONS_HPP

#include "direction.hpp"
#include "item.hpp"
#include <cstdint>
#include <string>

enum PersonType {
    invalidPerson,
    pelaaja,
    poliisi,
    Kake,
    mummo,
    juppi,
    hippi,
    vanki,
    varas
};

class Level;

class Person {
public:
    Person(PersonType personType) : type(personType) { }
    virtual ~Person() = default;

    virtual bool move(DirectionNS::Direction d) { return false; };
    virtual void npcAct(std::string& msg) { msg = ""; }
    virtual bool interact(std::string& message, Person* source) { return false; };
    virtual void damage(uint32_t damage) { };
    virtual std::string typeToChar() const { return "x"; };
    virtual Item* dropItem() { return nullptr; };

    void checkBounds(uint32_t sizex, uint32_t sizey) {
        x = (x == sizex) ? sizex-1 : x;
        x = (x > sizex) ? 0 : x;
        y = (y == sizey) ? sizey-1 : y;
        y = (y > sizey) ? 0 : y;
    }
    void setLevel(Level* level) { Person::level = level; }

    Level* level;
    uint32_t x = 30;
    uint32_t y = 20;
    PersonType type = invalidPerson;
    uint32_t health = 1;
};

class Mummo : public Person {
public:
    Mummo(uint32_t x, uint32_t y);
    virtual ~Mummo() = default;
    virtual bool interact(std::string& message, Person* source);
    Item* dropItem();
    virtual std::string typeToChar() const { return "M"; };
};

class Cop : public Person {
public:
    Cop(uint32_t x, uint32_t y);
    virtual ~Cop() = default;

    virtual bool move(DirectionNS::Direction d);
    virtual void npcAct(std::string& msg);
    virtual bool interact(std::string& message, Person* source);
    Item* dropItem();
    virtual std::string typeToChar() const { return "C"; };
    bool attack = false;
};

#endif