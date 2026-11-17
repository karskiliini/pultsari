#ifndef HANDSINTRO_HPP
#define HANDSINTRO_HPP

// The hands walk-through (InputNS::HANDS_INTRO_KEY): an animated show over the map that explains
// the forearms beside it. A demo drunk's arms stand in for the player's while it runs: the hands
// as the inventory, picking something up, the sleeve tearing as the health runs out, the
// captions at the bottom of the screen, and the rifle handed out at the front.

#include "player.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class Level;
class Printer;

namespace HandsIntroNS {

using Rows = std::vector<std::vector<std::pair<std::string, std::string>>>;

enum class Chapter { Inventory, Pickup, Damage, Captions, Rifle, End };
constexpr uint32_t CHAPTER_START[] = { 0, 7000, 14000, 22000, 29000, 36000 };
constexpr uint32_t TOTAL_MS = 40000;

class Show {
public:
    Show();
    // time passes: the demo's hands and sleeves follow, and the captions come on cue
    void tick(uint32_t ms);
    // straight to the start of the next part (the end of the show from the last one)
    void skip();
    bool over() const { return ms >= TOTAL_MS; }
    Chapter chapter() const;
    // the picture drawn where the map usually is
    Rows render() const;
    std::string title() const;
    std::string message() const;

    PlayerNS::Player demo;   // whose arms are drawn beside the map while the show runs
    uint32_t ms = 0;
private:
    void apply();            // the demo's state at `ms`
    void cue(uint32_t from, uint32_t to);   // the captions due in (from, to]
    int walkerX() const;     // the demo drunk on the little street of the pick-up part
};

// Runs the show over the map until it ends or is closed (Esc, Q or the intro key again); any
// other key skips to the next part. One key is read per frame. The printer's player, scene and
// message are put back afterwards.
void run(Printer* printer, Level* level);

}

#endif
