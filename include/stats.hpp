#ifndef STATS_HPP
#define STATS_HPP

#include <cstdint>
#include <cstdio>
#include <string>

struct Stats {
    uint32_t money = 0;
    // bottle deposits are paid in kopecks; every 100 turn into a rouble
    uint32_t kopecks = 0;
    uint32_t health = 1;
    uint32_t promilles = 0;
    uint32_t turn = 0;
    uint32_t hunger = 100;

    void addKopecks(uint32_t k)
    {
        kopecks += k;
        money += kopecks / 100;
        kopecks %= 100;
    }
};

// "21,40" when there are kopecks, plain "21" otherwise
inline std::string moneyText(const Stats& s)
{
    if (s.kopecks == 0) return std::to_string(s.money);
    char buf[24];
    snprintf(buf, sizeof(buf), "%u,%02u", s.money, s.kopecks);
    return buf;
}

#endif
