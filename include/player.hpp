#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "persons.hpp"
#include "level.hpp"
#include "inventory.hpp"
#include "direction.hpp"
#include "printer.hpp"
#include "scoreboard.hpp"
#include <cstdint>
#include <string>

namespace PlayerNS {

constexpr uint32_t INITIAL_HP = 10;

class Player : public Person {
public:
    Player();
    virtual ~Player() = default;
    virtual void updatepromilles(int p);
    virtual void updateMoney(int money);
    void updateTurn();

    void resetPosition();

    virtual void npcAct() { }

    bool stagger(Level& level);
    virtual bool move(DirectionNS::Direction d, Level& level);
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);

    virtual bool drink(Level& level);
    virtual bool eat(Level& level);
    virtual bool puke(DirectionNS::Direction d, Level& level);
    virtual bool throwItem();

    virtual std::string typeToChar() const { return "@"; };

    Inventory inventory;
    uint32_t promilles = 2;
    uint32_t money = 20;
    uint32_t turn = 1;
    bool inJail = false;

    ScoreBoard scoreBoard;

private:
    bool decrementInventory(uint32_t index);
};

}

#endif
