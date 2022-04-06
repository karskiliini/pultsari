#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "persons.hpp"
#include "level.hpp"
#include "inventory.hpp"
#include "direction.hpp"
#include "printer.hpp"
#include <cstdint>

namespace PlayerNS {

class Player : public Person {
public:
    Player();
    virtual ~Player() = default;

    virtual bool move(DirectionNS::Direction d, Level& level, Printer& printer);
    virtual bool drink(Printer& printer);
    virtual char typeToChar() const { return '@'; };

    Inventory inventory;
    uint32_t promillet = 0.2;
};

}

#endif
