#ifndef ARMART_HPP
#define ARMART_HPP

#include "hands.hpp"
#include <cstdint>
#include <string>
#include <vector>

// the player's forearms as plain-ASCII pictures shown beside the map: the sleeve hangs from the
// top (rows 0-8, torn by damage), the hand below it (rows 9-21) shows what the hand holds
constexpr int ARM_W = 14, ARM_H = 22;

int sleeveStage(uint32_t health, bool usedCoat);   // 0 whole jacket .. 3 sleeve gone, bloody
std::vector<std::string> armPicture(bool leftArm, const Held& held, int stage);  // ARM_H rows of ARM_W
std::string mirrorRow(const std::string& row);     // reverse + swap / \ ( ) < > [ ] { } ` '
bool armRowBloody(int stage, int row);             // rows the web colours red at stage 3

#endif
