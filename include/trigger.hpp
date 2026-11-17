#ifndef TRIGGER_HPP
#define TRIGGER_HPP

#include <cstdint>

// Space is the trigger. A terminal sends no key-up: a held key just keeps repeating, so a Space
// that comes hard on the heels of the last one is the key still held, not a new pull.
// The rifle fires on every Space (full auto while held); a pistol is self-loading single fire:
// one shot per pull, however long the key is held.
struct Trigger {
    // longer than a key repeat, shorter than two quick taps
    static constexpr uint32_t REPEAT_GAP_MS = 150;

    uint32_t msSinceSpace = 1000;

    void tick(uint32_t ms) { if (msSinceSpace < 100000) msSinceSpace += ms; }
    // a Space arrived: true if it is a new pull of the trigger, false if the key is held
    bool pull()
    {
        const bool fresh = msSinceSpace >= REPEAT_GAP_MS;
        msSinceSpace = 0;
        return fresh;
    }
};

#endif
