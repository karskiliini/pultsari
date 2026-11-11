#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "hands.hpp"

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

    // two-hands model (replaces the counters above in a later task)
    Held left = heldOf(Thing::Kalja);
    Held right = heldOf(Thing::Lenkki);
    Held pocket, belt, back;
    bool hasBelt = false;

    static Held heldOf(Thing t)
    {
        Held h;
        h.what = t;
        return h;
    }
    uint32_t count(Thing t) const
    {
        uint32_t n = 0;
        for (const Held* s : { &left, &right, &pocket, &belt, &back })
            if (s->what == t) ++n;
        return t == Thing::None ? 0 : n;
    }
    int freeHands() const { return (left.empty() ? 1 : 0) + (right.empty() ? 1 : 0); }
    Held* find(Thing t)
    {
        if (t == Thing::None) return nullptr;
        for (Held* s : { &left, &right, &pocket, &belt, &back })
            if (s->what == t) return s;
        return nullptr;
    }
    bool take(Thing t)
    {
        Held* s = find(t);
        if (!s) return false;
        *s = Held();
        return true;
    }
    bool give(Thing t, const std::string& pissSeller = "")
    {
        Held h;
        h.what = t;
        if (t == Thing::Vodka) h.fill = VODKA_SIPS;
        if (t == Thing::Pistooli) h.rounds = MAKAROV_ROUNDS;
        if (t == Thing::Rynkky) h.rounds = AK_ROUNDS;
        h.pissSeller = pissSeller;
        return give(h);
    }
    bool give(const Held& h)
    {
        const Thing t = h.what;
        if (t == Thing::None) return false;
        auto put = [&](Held& slot) {
            if (!slot.empty()) return false;
            slot = h;
            return true;
        };
        if (t == Thing::Rynkky) return put(back);
        if (t == Thing::Pistooli && hasBelt && put(belt)) return true;
        if (t == Thing::Lipas && put(pocket)) return true;
        if (isWeapon(t)) {
            if (put(right) || put(left)) return true;
        } else if (put(left) || put(right)) {
            return true;
        }
        return isSmall(t) && put(pocket);
    }
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
inline Thing stealableThing(int i)
{
    switch (i) {
        case 0: return Thing::Kalja;
        case 1: return Thing::Lonkka;
        case 2: return Thing::Vodka;
        case 3: return Thing::Lenkki;
        case 4: return Thing::Kala;
        case 5: return Thing::Omppo;
        default: return Thing::Bansku;
    }
}
inline const char* stealableName(int i)
{
    static const char* names[] = { "Baltikan", "Jaguarin", "vodkan", "makkaran", "voblan", "omenan", "banaanin" };
    return names[i < 0 || i >= STEALABLE_COUNT ? STEALABLE_COUNT - 1 : i];
}

#endif