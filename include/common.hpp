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
// the drunk comes to in the city's bottom-left corner, on the pavement of Leninski prospekt
// (its carriageway is rows 150..152, Leningradskoje shosse runs down columns 15..17)
constexpr uint32_t PLAYER_START_X = 6;
constexpr uint32_t PLAYER_START_Y = 153;

extern bool animsEnabled;
extern bool realtimeEnabled;

bool random(uint32_t pct);
bool checkBounds(Coord& coord);
void sleep(uint32_t ms);
// sleeps until the next frame of `ms` is due
void frameSleep(uint32_t ms);
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
// an anti-aircraft shell streaks from `from` to where it bursts
void shellEvent(const Coord& from, const Coord& to);
// someone throws up at `at`, spraying towards (dx, dy) (0, 0 = all around)
void vomitEvent(const Coord& at, int dx, int dy);
// direction of the next hit (e.g. a bullet), so the blood sprays the right way
void setHitDirection(int dx, int dy);

} // namespace common

#endif