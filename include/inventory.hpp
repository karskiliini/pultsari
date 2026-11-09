#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include <cstdint>
#include <string>
#include <vector>

struct Inventory {
    uint32_t kalja = 1;
    uint32_t lonkka = 0;
    uint32_t lenkki = 1;
    uint32_t ketjut = 0;
    uint32_t veitset = 0;
    uint32_t kivet = 0;
    uint32_t pamput = 0;
    uint32_t bootsit = 0;
    uint32_t kalat = 0;
    uint32_t omppo = 0;
    uint32_t bansku = 0;
    uint32_t pistooli = 0;
    uint32_t ammukset = 0;
    uint32_t propusk = 0;
    uint32_t granata = 0;
    uint32_t vodka = 0;
    uint32_t rynkky = 0;
    // empty bottles: 20 kopecks each at a hypermarket's return hatch
    uint32_t pullot = 0;
    // sellers of bottles counted in `vodka` that are actually piss
    std::vector<std::string> pissVodka;
};

// what a bottle thief can grab besides bottles, by index (see stolenItems in Person)
constexpr int STEALABLE_COUNT = 7;
inline uint32_t& stealable(Inventory& inv, int i)
{
    switch (i) {
        case 0: return inv.kalja;
        case 1: return inv.lonkka;
        case 2: return inv.vodka;
        case 3: return inv.lenkki;
        case 4: return inv.kalat;
        case 5: return inv.omppo;
        default: return inv.bansku;
    }
}
inline const char* stealableName(int i)
{
    static const char* names[] = { "Baltikan", "Jaguarin", "vodkan", "makkaran", "voblan", "omenan", "banaanin" };
    return names[i < 0 || i >= STEALABLE_COUNT ? STEALABLE_COUNT - 1 : i];
}

#endif