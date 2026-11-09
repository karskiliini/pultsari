#ifndef COMMON_HPP
#define COMMON_HPP

#include "coord.hpp"
#include <cstdint>
#include <string>

// enable features by defining these flags
#define ANIMATIONS_ENABLED
#define LOS_ENABLED

namespace common {

constexpr uint32_t VIEWPORT_W = 80;
constexpr uint32_t VIEWPORT_H = 28;
constexpr uint32_t WORLD_W = 400;
constexpr uint32_t WORLD_H = 200;
constexpr uint32_t SIZEX = WORLD_W;
constexpr uint32_t SIZEY = WORLD_H;
constexpr uint32_t PLAYER_START_X = 50;
constexpr uint32_t PLAYER_START_Y = WORLD_H / 2;

extern bool animsEnabled;
extern bool realtimeEnabled;

bool random(uint32_t pct);
bool checkBounds(Coord& coord);
void sleep(uint32_t ms);
// Ask the browser client to play a synthesized sound effect (volume 0..1).
// Sent as an OSC escape the web terminal intercepts; only when PULTSARI_WEB is set.
void sound(const char* name, float volume = 1.0f);

} // namespace common

#endif