#ifndef ITEM_HPP
#define ITEM_HPP

#include "coord.hpp"
#include <string>

namespace PlayerNS {
    class Player;
}

enum ItemType {
    EBona,
    EKalja,
    ELonkka,
    ELenkki,
    EPamppu,
    EVeitsi,
    EKetjut,
    EKivi,
    EBootsit,
    EKala,
    EOmppo,
    EBanaani,
    ERaha,
    EPaska,
    EViesti,
    ENextLevel
};

class Level;

class Item {
public:
    Item(uint32_t x, uint32_t y, ItemType t) : coord({x, y}), type(t) { };
    virtual ~Item() = default;

    virtual std::string typeToChar() const { return " "; };
    virtual std::string getMsg() const { return "error"; };
    virtual bool interact(PlayerNS::Player* player) { return true; };

    Coord coord;
    ItemType type;
    bool discard = false;
};

class Bona : public Item {
public:
    Bona(uint32_t x, uint32_t y);
    virtual ~Bona() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
};

class Kalja : public Item {
public:
    Kalja(uint32_t x, uint32_t y);
    virtual ~Kalja() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Lonkka : public Item {
public:
    Lonkka(uint32_t x, uint32_t y);
    virtual ~Lonkka() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Lenkki : public Item {
public:
    Lenkki(uint32_t x, uint32_t y);
    virtual ~Lenkki() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Pamppu : public Item {
public:
    Pamppu(uint32_t x, uint32_t y);
    virtual ~Pamppu() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Veitsi : public Item {
public:
    Veitsi(uint32_t x, uint32_t y);
    virtual ~Veitsi() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Ketjut : public Item {
public:
    Ketjut(uint32_t x, uint32_t y);
    virtual ~Ketjut() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Kivi : public Item {
public:
    Kivi(uint32_t x, uint32_t y);
    virtual ~Kivi() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Bootsit : public Item {
public:
    Bootsit(uint32_t x, uint32_t y);
    virtual ~Bootsit() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Kala : public Item {
public:
    Kala(uint32_t x, uint32_t y);
    virtual ~Kala() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Omppo : public Item {
public:
    Omppo(uint32_t x, uint32_t y);
    virtual ~Omppo() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Banaani : public Item {
public:
    Banaani(uint32_t x, uint32_t y);
    virtual ~Banaani() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Raha : public Item {
public:
    Raha(uint32_t x, uint32_t y);
    Raha(uint32_t x, uint32_t y, uint32_t value);
    virtual ~Raha() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
    uint32_t value = 1;
};

class Paska : public Item {
public:
    Paska(uint32_t x, uint32_t y);
    virtual ~Paska() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Viesti : public Item {
public:
    Viesti(uint32_t x, uint32_t y);
    virtual ~Viesti() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class NextLevel : public Item {
public:
    NextLevel(uint32_t x, uint32_t y, Level* level);
    virtual ~NextLevel() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);

    void setLevel(Level* l) { level = l; }
    Level* level;
};
#endif