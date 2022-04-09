#ifndef PERSONS_HPP
#define PERSONS_HPP

#include "persontype.hpp"
#include "coord.hpp"
#include "direction.hpp"
#include "item.hpp"
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
    virtual void damage(uint32_t damage) { };
    virtual std::string typeToChar() const { return "x"; };
    virtual Item* dropItem() { return nullptr; };

    bool checkBounds(uint32_t sizex, uint32_t sizey) {
        auto ox = coord.x;
        auto oy = coord.y;

        coord.x = (coord.x == sizex) ? sizex-1 : coord.x;
        coord.x = (coord.x > sizex) ? 0 : coord.x;
        coord.y = (coord.y == sizey) ? sizey-1 : coord.y;
        coord.y = (coord.y > sizey) ? 0 : coord.y;

        return !((ox == coord.x) && (oy == coord.y));
    }
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
    Item* dropItem();
    virtual std::string typeToChar() const { return "C"; };
    bool attack = false;
    Coord target;
};

class Varas : public Person {
public:
    Varas(const Coord& pos);
    virtual ~Varas() = default;

    virtual bool move(DirectionNS::Direction d);
    bool move(DirectionNS::Direction d, std::string& msg);
    virtual void npcAct(std::string& msg);
    virtual bool interact(std::string& message, Person* source);
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
    virtual std::string typeToChar() const { return "E"; };
    Coord target;
    bool kivi = false;
    bool firstMove = true;
};



#endif