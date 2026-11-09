#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "coord.hpp"
#include <string>

class Animation {
public:
    Animation(const Coord& pos, std::string txt, uint32_t frames);
    virtual ~Animation() = default;

    virtual bool act();

    Coord coord;
    Coordinate<int> coordMove { 0, 0 };
    std::string text;
    uint32_t ttl;
    uint32_t frames;            // initial ttl, for fading
    const void* owner = nullptr; // a speaker's new bubble replaces the old one
    Coord source;
    uint32_t tick = 0;               // where it happens; drawn only if the player sees that spot
    uint32_t delay = 66;
    bool isBubble = false;
};

#endif