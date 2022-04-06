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
    virtual char typeToChar() const {
        return 'x';
    };

    Level* level;
    uint32_t x = 30;
    uint32_t y = 20;
    PersonType type;
    uint32_t health = 1;

};

#endif