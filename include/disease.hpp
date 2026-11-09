#ifndef DISEASE_HPP
#define DISEASE_HPP

#include <cstdint>

// HIV from a used needle. One in two sticks infects, and the player is not told:
// a passing fever after a while, then symptoms that worsen in stages until AIDS,
// which has no cure in this Moscow and ends in death.
namespace Disease {

constexpr uint32_t INFECTION_CHANCE = 50;   // percent per needle stick
constexpr uint32_t FLU_TURNS = 200;         // acute fever after infection (ambiguous hint)
constexpr uint32_t INCUBATION_TURNS = 750;  // ~5 min in real-time (2.5 turns/s)
constexpr uint32_t STAGE_TURNS = 450;       // each later stage ~3 min after the previous
constexpr uint32_t AIDS_STAGE = 5;
constexpr uint32_t WASTING_TURNS = 150;     // with AIDS, VOIMA drops by one this often

struct Hiv {
    bool infected = false;
    uint32_t infectedTurn = 0;
    uint32_t sticks = 0;          // needles the player has stepped on
    uint32_t announcedStage = 0;  // last stage the player has been told about
    bool fluShown = false;

    // a needle stick on `turn`; `roll` is 0-99. Returns true if this stick infected.
    bool needleStick(uint32_t turn, uint32_t roll);
    // 0 = healthy or still incubating, 1-4 = HIV symptoms, 5 = AIDS
    uint32_t stage(uint32_t turn) const;
    // turn on which `stage` begins (meaningless when not infected)
    uint32_t stageStart(uint32_t s) const;
    // VOIMA lost to wasting by `turn` (only with AIDS)
    uint32_t wasted(uint32_t turn) const;
};

// highest VOIMA the body holds up at this stage (0 = no limit)
uint32_t maxHealth(uint32_t stage);
// how far the player sees, from the healthy distance
uint32_t visionDistance(uint32_t base, uint32_t stage);
// percentage of the normal hearing range left
uint32_t hearingPercent(uint32_t stage);
// short status for the HUD ("" while the player does not know)
const char* hudLabel(uint32_t stage);
// what the player notices when a stage begins
const char* stageMessage(uint32_t stage);

}

#endif
