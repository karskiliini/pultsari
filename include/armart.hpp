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
// ARM_H rows of ARM_W; `coat` is whose sleeve it is: anyone else's is whole at stage 0
std::vector<std::string> armPicture(bool leftArm, const Held& held, int stage, Coat coat = Coat::Oma);
std::string mirrorRow(const std::string& row);     // reverse + swap / \ ( ) < > [ ] { } ` '
bool armRowBloody(int stage, int row);             // rows with blood on them at stage 3
// the picture's colours, one letter per cell, composed and mirrored like armPicture: J own jacket,
// B police blue, O soldier olive, D black leather, N civilian grey (the coat's cloth), S skin, G bottle glass, V vodka, A beer, L label text, C can metal, M gun metal, W wood,
// R blood, Y banana yellow, K grenade green, space none
std::vector<std::string> armColours(bool leftArm, const Held& held, int stage, Coat coat = Coat::Oma);

#endif
