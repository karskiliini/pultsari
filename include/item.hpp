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
class Printer;

class Item {
public:
    Item(const Coord& c, ItemType t) : coord(c), type(t) { };
    virtual ~Item() = default;

    virtual std::string typeToChar() const { return " "; };
    virtual std::string getMsg() const { return "error"; };
    virtual bool interact(PlayerNS::Player* player) { return true; };

    virtual void actThrow(Printer* printer, Level* level);
    static Item* createItem(uint32_t index, Coord coord);

    Coord coord;
    ItemType type;
    bool discard = false;

    bool thrown = false;
    Coordinate<int> throwVec { 0, 0 };
};

class Bona : public Item {
public:
    Bona(const Coord& c);
    virtual ~Bona() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
};

class Kalja : public Item {
public:
    Kalja(const Coord& c);
    virtual ~Kalja() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Lonkka : public Item {
public:
    Lonkka(const Coord& c);
    virtual ~Lonkka() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Lenkki : public Item {
public:
    Lenkki(const Coord& c);
    virtual ~Lenkki() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Pamppu : public Item {
public:
    Pamppu(const Coord& c);
    virtual ~Pamppu() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Veitsi : public Item {
public:
    Veitsi(const Coord& c);
    virtual ~Veitsi() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Ketjut : public Item {
public:
    Ketjut(const Coord& c);
    virtual ~Ketjut() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Kivi : public Item {
public:
    Kivi(const Coord& c);
    virtual ~Kivi() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Bootsit : public Item {
public:
    Bootsit(const Coord& c);
    virtual ~Bootsit() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Kala : public Item {
public:
    Kala(const Coord& c);
    virtual ~Kala() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Omppo : public Item {
public:
    Omppo(const Coord& c);
    virtual ~Omppo() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Banaani : public Item {
public:
    Banaani(const Coord& c);
    virtual ~Banaani() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Raha : public Item {
public:
    Raha(const Coord& c);
    Raha(const Coord& c, uint32_t value);
    virtual ~Raha() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
    uint32_t value = 1;
};

class Paska : public Item {
public:
    Paska(const Coord& c);
    virtual ~Paska() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Viesti : public Item {
public:
    Viesti(const Coord& c);
    virtual ~Viesti() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class NextLevel : public Item {
public:
    NextLevel(const Coord& c, Level* level);
    virtual ~NextLevel() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);

    void setLevel(Level* l) { level = l; }
    Level* level;
};
#endif