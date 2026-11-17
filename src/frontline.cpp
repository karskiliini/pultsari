#include "animation.hpp"
#include "common.hpp"
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

void Level::shellBurst(const Coord& at, uint32_t radius, uint32_t maxDamage, const char* who)
{
    HurtBy hurt(this, who);
    common::boomEvent(at, radius);
    addAnimation(new Animation(Coord{ at.x > 3 ? at.x - 3 : 0, at.y }, "*KRUMP*", 6));
    audio.addSound(at, SoundType::Explosion,
        "KRUMP! Kranaatti iskee aivan vieress\xC3\xA4!", "Kranaatin r\xC3\xA4j\xC3\xA4hdys!", "...tykist\xC3\xB6n jyrin\xC3\xA4\xC3\xA4...", 60);
    const PlayerNS::Player* player = getPlayer();
    for (auto p : persons) {
        if (p->stats.health == 0) continue;
        const uint32_t d = p->coord.distance(at);
        if (d > radius) continue;
        // the trench walls take the shrapnel unless it lands right on top of you
        if (inCover(p->coord) && d > 1) continue;
        p->damage(std::rand() % maxDamage + 1);
        if (p == player && printer) printer->showMessage("Kranaatin sirpale osuu!", this, false);
    }
    // the ground is torn up; a trench is a trench still, just a deeper one
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if ((dx || dy) && std::rand() % 2) continue;
            const int x = (int)at.x + dx, y = (int)at.y + dy;
            if (x < 0 || y < 0) continue;
            const Coord c { (uint32_t)x, (uint32_t)y };
            const Trench t = trenchAt(c);
            if (t != Trench::Own && t != Trench::Enemy && c.x < common::SIZEX && c.y < common::SIZEY) setTrench(c, Trench::Crater);
        }
    }
}

void Level::fireSalvo(uint32_t shells)
{
    const PlayerNS::Player* player = getPlayer();
    const Coord around = player ? player->coord : Coord{ TrenchNS::OWN_X, common::PLAYER_START_Y };
    for (uint32_t i = 0; i < shells; ++i) {
        const int y = std::max(0, std::min((int)common::SIZEY - 1, (int)around.y + std::rand() % 41 - 20));
        const int line = ownTrenchEastX(*this, (uint32_t)y);
        const int x = std::max(0, (line < 0 ? (int)TrenchNS::OWN_X : line - 1) + std::rand() % 9 - 4);
        shellBurst(Coord{ (uint32_t)x, (uint32_t)y }, 3, 8, "tykist\xC3\xB6");
    }
}

void Level::artilleryTick()
{
    if (!isWarLevel || warStage < 2 || playerDead) return;
    if (salvoIn) {
        if (--salvoIn == 0) fireSalvo(3 + std::rand() % 4);
        return;
    }
    if (artilleryCooldown && --artilleryCooldown) return;
    artilleryCooldown = 25 + std::rand() % 26;   // ten to twenty seconds between salvoes
    salvoIn = 2;
    if (printer) printer->showMessage("Ulvontaa id\xC3\xA4st\xC3\xA4! TYKIST\xC3\x96\xC3\x84! Maahan!", this, false);
    Subtitles::say("[Kranaattien ulvonta l\xC3\xA4hestyy. Haudan pohjalle!]", "artillery");
}

void Level::warTick()
{
    if (!isWarLevel || playerDead) return;
    ++warTicks;
    exposureTick();
    artilleryTick();
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
