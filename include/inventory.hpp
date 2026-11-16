#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include <cstdint>
#include <string>
#include "hands.hpp"

struct Inventory {
    // the Kremlin pass is a paper in the inner pocket: it takes no slot and never falls out
    uint32_t propusk = 0;

    // two hands, one pocket, the belt (pistol) and the back (AK)
    Held left = heldOf(Thing::Kalja);
    Held right = heldOf(Thing::Lenkki);
    Held pocket, belt, back;
    bool hasBelt = false;
    // the rifle is off the back and up at the shoulder: it fills both hands until lowered
    bool akRaised = false;

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
    int freeHands() const { return akRaised ? 0 : (left.empty() ? 1 : 0) + (right.empty() ? 1 : 0); }
    // the hand holding it, if either does
    Held* inHand(Thing t)
    {
        if (t == Thing::None) return nullptr;
        if (left.what == t) return &left;
        if (right.what == t) return &right;
        return nullptr;
    }
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
    // a new one, as found or bought: a full bottle, a loaded gun
    static Held fresh(Thing t, const std::string& pissSeller = "")
    {
        Held h;
        h.what = t;
        if (t == Thing::Vodka) h.fill = VODKA_SIPS;
        if (t == Thing::Pistooli) h.rounds = MAKAROV_ROUNDS;
        if (t == Thing::Rynkky) h.rounds = AK_ROUNDS;
        h.pissSeller = pissSeller;
        return h;
    }
    bool give(Thing t, const std::string& pissSeller = "")
    {
        return give(fresh(t, pissSeller));
    }
    bool give(const Held& h)
    {
        const Thing t = h.what;
        if (t == Thing::None) return false;
        auto put = [&](Held& slot) {
            if (!slot.empty()) return false;
            if (akRaised && (&slot == &left || &slot == &right)) return false;
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
    // would give() take it? nothing changes
    bool fits(Thing t) const
    {
        Inventory copy = *this;
        return copy.give(t);
    }
    // a drink in hand is drunk: a bottle stays in the hand empty, a Jaguar can goes
    bool drain(Thing t)
    {
        Held* s = find(t);
        if (!s) return false;
        *s = t == Thing::Lonkka ? Held() : heldOf(Thing::Tyhja);
        return true;
    }
    // bottles in the hands, empty or not: what a bottle thief is after
    uint32_t bottlesInHands() const
    {
        uint32_t n = 0;
        for (const Held* s : { &left, &right })
            if (s->what == Thing::Tyhja || isDrink(s->what)) ++n;
        return n;
    }
};

// what a bottle thief can grab besides bottles, by index
constexpr int STEALABLE_COUNT = 7;
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
inline const char* stealableName(Thing t)
{
    for (int i = 0; i < STEALABLE_COUNT; ++i)
        if (stealableThing(i) == t) return stealableName(i);
    return t == Thing::Tyhja ? "pullon" : "";
}

#endif