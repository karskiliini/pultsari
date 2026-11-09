#ifndef INTROART_HPP
#define INTROART_HPP

#include <cstdint>

// The game ad as colour ASCII art for the terminal intro's background
// (generated from art/ by art/tools/gen_cpp.py).
namespace IntroArt {

struct Picture {
    int cols;
    int rows;
    const char* const* chars;   // rows strings of cols characters
    const uint8_t* fg;          // rows * cols xterm-256 colours
    const uint8_t* bg;

    // The chimney fire, its smoke and the horizon fires: a looping animation
    // that redraws only the cells listed in animCells (index = row * cols + col).
    int animFrames;
    int animCellCount;
    const uint16_t* animCells;
    const char* const* animChars;  // animFrames strings of animCellCount characters
    const uint8_t* animFg;         // animFrames * animCellCount
    const uint8_t* animBg;
};

constexpr int ANIM_FPS = 12;

extern const Picture PICTURES[];   // largest first
extern const int PICTURE_COUNT;

} // namespace IntroArt

#endif
