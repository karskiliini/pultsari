#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "persons.hpp"
#include "inventory.hpp"
#include "direction.hpp"
#include <cstdint>

namespace PlayerNS {

class Player : public Person {
public:
    Player();
    virtual ~Player() = default;

    virtual bool move(DirectionNS::Direction d);
    virtual bool drink();

    Inventory inventory;
    uint32_t promillet = 0.2;
};

}

#endif
