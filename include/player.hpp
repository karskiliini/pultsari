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
    virtual void updatepromilles(int p);
    void updateTurn();

    void resetPosition();

    virtual void npcAct(Printer* printer) { }

    virtual bool move(DirectionNS::Direction d, Level& level, Printer& printer);
    virtual bool interact(std::string& msg, Person* source, Printer* printer);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);

    virtual bool drink(Printer& printer, Level& level);
    virtual bool eat(Printer& printer, Level& level);
    virtual bool throwItem(Printer& printer);

    virtual std::string typeToChar() const { return "@"; };

    Inventory inventory;
    uint32_t promilles = 2;
    uint32_t money = 20;
    uint32_t turn = 1;
    bool inJail = false;

private:
    bool decrementInventory(uint32_t index, Printer& printer);
};

}

#endif
