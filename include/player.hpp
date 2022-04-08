#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "persons.hpp"
#include "level.hpp"
#include "inventory.hpp"
#include "direction.hpp"
#include "printer.hpp"
#include <cstdint>
#include <string>

namespace PlayerNS {

class Player : public Person {
public:
    Player();
    virtual ~Player() = default;
    void resetPosition();

    virtual bool move(DirectionNS::Direction d, Level& level, Printer& printer);
    virtual void npcAct();
    virtual bool interact(std::string& message, Person* source);
    virtual void damage(uint32_t damage);

    virtual bool drink(Printer& printer, Level& level);
    virtual bool eat(Printer& printer, Level& level);
    virtual std::string typeToChar() const { return "@"; };

    Inventory inventory;
    uint32_t promilles = 0.2;
    uint32_t money = 20;
    uint32_t turn = 1;
    bool inJail = false;
};

}

#endif
