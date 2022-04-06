#ifndef ITEM_HPP
#define ITEM_HPP

#include <string>

class Item {
public:
    Item(uint32_t x, uint32_t y) : x(x), y(y) { };
    virtual ~Item() = default;

    virtual char typeToChar() const { return ' '; };
    virtual std::string getMsg() const { return "error"; };

    uint32_t x;
    uint32_t y;
};

class Bona : public Item {
public:
    Bona(uint32_t x, uint32_t y);
    virtual ~Bona();

    virtual char typeToChar() const;
    virtual std::string getMsg() const;
};

#endif