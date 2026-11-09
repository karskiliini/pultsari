#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "coord.hpp"
#include <cstdint>
#include <vector>
#include <string>

class Level;
class Printer;

struct Fire {
    Coord coord;
    uint32_t ttl;
    uint32_t spreadChance;
};

class EnvironmentEvents {
public:
    EnvironmentEvents(Level* level, bool calm = false);
    ~EnvironmentEvents() = default;

    void update();

    bool hasFire(const Coord& c) const;
    void addFiresAround(const Coord& c);
    // a whole building catches fire: flames over its walls and inside, burning for a long while
    void setAblaze(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
    const std::vector<Fire>& getFires() const { return fires; }

private:
    void droneStrike();
    void aaFire();
    void updateFires();
    void spreadFire();
    void fireDamage();
    void droneSound();
    void gasAttack();
    void defenestration();
    void novichok();
    void randomEvent();
    void artilleryBarrage();
    void meatAssault();
    void militaryParade();
    void hypermarketFire();

    Level* level;
    bool calm;  // early Moscow: only fires burn, no random disasters
    std::vector<Fire> fires;
    uint32_t droneCooldown = 0;
    uint32_t aaCooldown = 0;
    uint32_t droneNoiseCooldown = 0;
    uint32_t eventCooldown = 0;
    uint32_t paradeCooldown = 0;
    bool gasActive = false;
    uint32_t gasTimer = 0;
    Coord gasCenter = { 0, 0 };
};

#endif
