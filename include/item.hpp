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
    EOksennus,
    ENextLevel,
    EPensas,
    ERoskis,
    EBusStop,
    EPistooli,
    EPropusk,
    EGranata,
    EVodka,
    EViinaKellari,
    EPutinPoyta,
    EKartta,
    ERoska,
    ERynkky
};

class Level;

class Item {
public:
    Item(const Coord& c, ItemType t) : coord(c), type(t) { };
    virtual ~Item() = default;

    virtual std::string typeToChar() const { return " "; };
    virtual std::string getMsg() const { return "error"; };
    virtual bool interact(PlayerNS::Player* player) { return true; };

    virtual void actThrow(Level* level);
    static Item* createItem(uint32_t index, Coord coord);

    Coord coord;
    ItemType type;
    bool discard = false;

    bool thrown = false;
    Coordinate<int> throwVec { 0, 0 };
    // a throw at any angle: exact position and direction, and how many cells it has flown
    bool angled = false;
    float fx = 0, fy = 0, fdx = 0, fdy = 0;
    uint32_t flight = 0;
    bool blocksLOS = false;
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

class Oksennus : public Item {
public:
    Oksennus(const Coord& c);
    virtual ~Oksennus() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Pistooli : public Item {
public:
    Pistooli(const Coord& c);
    virtual ~Pistooli() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class BusStop : public Item {
public:
    BusStop(const Coord& c);
    virtual ~BusStop() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Pensas : public Item {
public:
    Pensas(const Coord& c);
    virtual ~Pensas() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Roskis : public Item {
public:
    Roskis(const Coord& c);
    virtual ~Roskis() = default;
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

class Propusk : public Item {
public:
    Propusk(const Coord& c);
    virtual ~Propusk() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Granata : public Item {
public:
    Granata(const Coord& c);
    virtual ~Granata() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class Vodka : public Item {
public:
    Vodka(const Coord& c);
    virtual ~Vodka() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
};

class ViinaKellari : public Item {
public:
    ViinaKellari(const Coord& c, Level* level);
    virtual ~ViinaKellari() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
    Level* level;
};

class PutinPoyta : public Item {
public:
    // putinsDesk: Putin's own table with his dolls; otherwise the generals' map table
    PutinPoyta(const Coord& c, const char* glyph, bool putinsDesk = true);
    virtual ~PutinPoyta() = default;
    virtual std::string typeToChar() const;
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player);
private:
    const char* glyph;
    bool putinsDesk;
};

// the generals' war map, spread on a table
class Kartta : public Item {
public:
    Kartta(const Coord& c, const char* glyph);
    virtual std::string typeToChar() const { return glyph; }
    virtual std::string getMsg() const;
    virtual bool interact(PlayerNS::Player* player) { return false; }
private:
    const char* glyph;
};

// Litter on the ground around a trash bin: search it and see what you get.
class Roska : public Item {
public:
    Roska(const Coord& c);
    virtual std::string typeToChar() const { return glyph; }
    virtual std::string getMsg() const { return msg; }
    virtual bool interact(PlayerNS::Player* player);
private:
    const char* glyph;
    std::string msg;
    int outcome;
};

// An assault rifle, with a full magazine.
class Rynkky : public Item {
public:
    Rynkky(const Coord& c) : Item(c, ERynkky) { }
    virtual std::string typeToChar() const { return "\xC2\xAC"; }
    virtual std::string getMsg() const { return "Rynn\xC3\xA4kk\xC3\xB6kiv\xC3\xA4\xC3\xA4ri! Kalashnikov, tietysti. Ja t\xC3\xA4ysi lipas."; }
    virtual bool interact(PlayerNS::Player* player);
};

#endif