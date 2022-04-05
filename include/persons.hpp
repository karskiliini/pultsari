#ifndef PERSONS_HPP
#define PERSONS_HPP

#include "direction.hpp"
#include <cstdint>

enum PersonType {
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
    Person(const Person &obj);  // copy constructor

    virtual ~Person() = default;

    virtual bool move(DirectionNS::Direction d);

    void setLevel(Level* l);

    Level* level;
    uint32_t x = 0;
    uint32_t y = 0;
    PersonType type;
    uint32_t health = 1;
};

#endif