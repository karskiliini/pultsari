#include "animation.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"

// How often the open rear is announced again, in world ticks.
static constexpr uint32_t REAR_REMINDER = 25;

void Level::warTick()
{
    if (!isWarLevel || playerDead) return;
    ++warTicks;
    PlayerNS::Player* player = getPlayer();

    if (!rearOpen) {
        if (warTicks == WAR_SHIFT_TICKS / 2 && printer) {
            printer->showMessage("Komppanianp\xC3\xA4\xC3\xA4llikk\xC3\xB6 radiossa: 'Vuorovaihto tulee pian. Pysyk\xC3\xA4\xC3\xA4 kuopissa!'", this, false);
        }
        if (warTicks < WAR_SHIFT_TICKS) return;
        rearOpen = true;
    } else if ((warTicks - WAR_SHIFT_TICKS) % REAR_REMINDER != 0) {
        return;
    }

    if (printer) {
        printer->showMessage("VUOROVAIHTO! Vet\xC3\xA4ydy selustaan: kartan vasempaan reunaan <<", this, false);
    }
    // an arrow at the rear, on the player's row, so it shows on screen
    if (player) {
        addAnimation(new Animation(Coord{ 0, player->coord.y }, "<< SELUSTA", 20));
    }
}

bool Level::reachedRear(const Coord& c) const
{
    return isWarLevel && rearOpen && c.x <= REAR_X;
}
