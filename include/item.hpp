#ifndef ITEM_HPP
#define ITEM_HPP

#include <string>

namespace PlayerNS {
    class Player;
}

class Item {
public:
    Item(uint32_t x, uint32_t y) : x(x), y(y) { };
    virtual ~Item() = default;

    virtual std::string typeToChar() const { return " "; };
    virtual std::string getMsg() const { return "error"; };
    virtual bool interact(PlayerNS::Player* player) { return true; };

    uint32_t x;
    uint32_t y;
};

class Bona : public Item {
public:
    Bona(uint32_t x, uint32_t y);
    virtual ~Bona() = default;

    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
};

class Paska : public Item {
public:
    Paska(uint32_t x, uint32_t y);
    virtual ~Paska() = default;

    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

#endif