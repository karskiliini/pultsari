#ifndef HANDS_HPP
#define HANDS_HPP

#include <cstdint>
#include <string>

enum class Thing : uint8_t { None, Kalja, Lonkka, Vodka, Tyhja, Lenkki, Kala, Omppo, Bansku,
    Ketjut, Veitsi, Kivi, Pamppu, Bootsit, Granata, Pistooli, Rynkky, Lipas };

constexpr uint8_t VODKA_SIPS = 6, MAKAROV_ROUNDS = 8, AK_ROUNDS = 30;

struct Held {
    Thing what = Thing::None;
    uint8_t fill = 0;
    uint8_t rounds = 0;
    std::string pissSeller;
    bool empty() const { return what == Thing::None; }
};

bool isSmall(Thing t);  // Lipas, Granata, Veitsi
bool isDrink(Thing t);  // Kalja, Lonkka, Vodka
bool isWeapon(Thing t); // Pistooli, Rynkky
bool isFood(Thing t);   // Lenkki, Kala, Omppo, Bansku
const char* thingName(Thing t);
// a glass bottle, full or empty (a Jaguar is a can)
bool isBottle(Thing t);
// what it adds to a punch when you walk into someone with it in hand: 0 for nothing useful
uint32_t meleeDamage(Thing t);
// a bottle swung at a head breaks this often (percent), for this much more on top
constexpr uint32_t BOTTLE_BREAK_PERCENT = 60, BOTTLE_BREAK_DAMAGE = 3;

// whose coat the player wears; it colours the sleeves of the arm pictures. Oma is the poster's
// own ragged work jacket; the others come off corpses, the ground or the pawnshop and start whole.
enum class Coat : uint8_t { Oma, Poliisi, Sotilas, Musta, Siviili };
char coatColour(Coat c);       // the sleeve's colour mask letter: J, B, O, D, N
const char* coatName(Coat c);  // genitive, for messages: "poliisin takki"

// the throw numbers of Player::decrementInventory / Item::createItem: 1 Kalja, 2 Lonkka, 3 Lenkki,
// 4 Ketjut, 5 Veitsi, 6 Kivi, 7 Pamppu, 8 Bootsit, 9 Kala, 10 Omppo, 11 Bansku, 13 Vodka,
// 14 Granata, 15 Tyhja (an empty bottle); anything else is Thing::None
Thing throwNumToThing(uint32_t n);
uint32_t thingToThrowNum(Thing t); // 0 if it cannot be thrown

#endif
