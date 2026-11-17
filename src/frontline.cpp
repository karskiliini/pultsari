#include "animation.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "subtitles.hpp"
#include <algorithm>

// How often the open rear is announced again, in world ticks.
static constexpr uint32_t REAR_REMINDER = 25;

void Level::exposureTick()
{
    PlayerNS::Player* p = getPlayer();
    if (!isWarLevel || playerDead || !p) return;
    const Trench t = trenchAt(p->coord);
    const bool down = t == Trench::Own || t == Trench::Enemy;

    if (down) p->exposure = p->exposure > 2 ? p->exposure - 2 : 0;
    else p->exposure = std::min<uint32_t>(100, p->exposure + (t == Trench::Crater ? 2 : 4));

    // the more he is seen, the more is said about it; a spell down in the trench resets it
    static const char* marks[] = {
        "[Sinut on huomattu.]",
        "[Kiikarit kiiltävät idässä.]",
        "[Droonien pilotit puhuvat sinusta radiossa.]",
    };
    const uint32_t mark = p->exposure >= 75 ? 3 : p->exposure >= 50 ? 2 : p->exposure >= 25 ? 1 : 0;
    if (mark > p->exposureMark) Subtitles::say(marks[mark - 1], "exposure");
    if (mark > p->exposureMark || mark == 0) p->exposureMark = mark;

    // sitting tight in one stretch of trench: the pilots work out where he is
    const Coord block { p->coord.x / 5, p->coord.y / 5 };
    if (down && (block == p->pressureBlock || p->pressureBlock.x == UINT32_MAX)) {
        p->trenchPressure = std::min<uint32_t>(100, p->trenchPressure + 3);
        if (p->trenchPressure == 60) Subtitles::say("[Droonit kiertävät juuri tätä kohtaa haudasta.]", "pressure");
    } else {
        p->trenchPressure = p->trenchPressure > 2 ? p->trenchPressure - 2 : 0;
    }
    p->pressureBlock = down ? block : Coord{ UINT32_MAX, UINT32_MAX };

    // the wire bites through trousers and skin
    if (t == Trench::Wire && common::random(30)) {
        HurtBy who(this, "piikkilanka");
        p->damage(1);
        if (printer) printer->showMessage("Piikkilanka repii lahkeesi ja s\xC3\xA4\xC3\xA4resi!", this, false);
    }
}

void Level::warTick()
{
    if (!isWarLevel || playerDead) return;
    ++warTicks;
    exposureTick();
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
