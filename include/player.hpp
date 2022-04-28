#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "persons.hpp"
#include "inventory.hpp"
#include "direction.hpp"
#include "printer.hpp"
#include "scoreboard.hpp"
#include "stats.hpp"
#include <cstdint>
#include <string>

class Level;

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

    bool stagger(Level* level);
    virtual bool move(DirectionNS::Direction d, Level* level);
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);

    virtual bool drink(Level* level);
    virtual bool eat(Level* level);
    virtual bool puke(DirectionNS::Direction d, Level* level);
    virtual bool throwItem();

    virtual std::string typeToChar() const { return "@"; };

    Inventory inventory;
    bool inJail = false;

    ScoreBoard scoreBoard;

private:
    bool decrementInventory(uint32_t index);
};

}

#endif
