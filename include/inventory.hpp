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
    // sellers of bottles counted in `vodka` that are actually piss
    std::vector<std::string> pissVodka;
};

#endif