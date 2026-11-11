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
const char* thingName(Thing t);

#endif
