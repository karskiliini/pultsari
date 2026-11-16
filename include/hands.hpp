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

// the throw numbers of Player::decrementInventory / Item::createItem: 1 Kalja, 2 Lonkka, 3 Lenkki,
// 4 Ketjut, 5 Veitsi, 6 Kivi, 7 Pamppu, 8 Bootsit, 9 Kala, 10 Omppo, 11 Bansku, 13 Vodka,
// 14 Granata, 15 Tyhja (an empty bottle); anything else is Thing::None
Thing throwNumToThing(uint32_t n);
uint32_t thingToThrowNum(Thing t); // 0 if it cannot be thrown

#endif
