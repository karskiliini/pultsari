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
// Browser mode (PULTSARI_WEB): the game talks to the web client in JSON messages,
// one per line, each starting with an ASCII record separator (0x1E).
bool webMode();
void webSend(const std::string& json);
std::string jsonString(const std::string& s);

// Ask the browser client to play a synthesized sound effect (volume 0..1).
void sound(const char* name, float volume = 1.0f);

// Visual events for the browser's pixel effects (world coordinates).
void hitEvent(const Coord& at, uint32_t damage);
void boomEvent(const Coord& at, uint32_t radius);
// direction of the next hit (e.g. a bullet), so the blood sprays the right way
void setHitDirection(int dx, int dy);

} // namespace common

#endif