#ifndef PERSONS_HPP
#define PERSONS_HPP

#include "direction.hpp"
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
    virtual void npcAct() { }
    virtual bool interact(std::string& message, Person* source) { return false; };
    virtual void damage(uint32_t damage) { };
    virtual std::string typeToChar() const { return "x"; };

    uint32_t x = 30;
    uint32_t y = 20;
    PersonType type = invalidPerson;
    uint32_t health = 1;
};

class Mummo : public Person {
public:
    Mummo();
    virtual ~Mummo() = default;

    virtual void npcAct();
    virtual bool interact(std::string& message, Person* source);
    virtual std::string typeToChar() const { return "M"; };
};

#endif