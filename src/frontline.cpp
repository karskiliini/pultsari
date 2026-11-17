#include "animation.hpp"
#include "bradley.hpp"
#include "fpv.hpp"
#include "persons.hpp"
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

void Level::prepareHold()
{
    const PlayerNS::Player* player = getPlayer();
    const uint32_t y = player ? player->coord.y : common::PLAYER_START_Y;
    // the officer: on the eastern lip of the main line (not out in a listening post), on the new man's row
    int lip = -1;
    for (int x = (int)TrenchNS::OWN_X + 4; x >= 0 && lip < 0; --x) if (inOwnTrench(Coord{ (uint32_t)x, y })) lip = x;
    const Coord want { (uint32_t)std::max(0, lip), y };
    const Coord at = nearestTrench(*this, want, Trench::Own);
    Sotilas* officer = new Sotilas(at, true);
    officer->printer = printer;
    officer->pathFinding = pathFinding;
    if (inOwnTrench(at) && addPerson(officer)) holdOfficer = officer;
    else delete officer;

    // the machine guns: at the mouth of every communication trench on the rear edge, and between
    mgNests.clear();
    for (uint32_t row = 0; row < common::SIZEY; ++row) {
        if (!inOwnTrench(Coord{ 0, row })) continue;
        if (mgNests.empty() || row > mgNests.back().y + 3) mgNests.push_back(Coord{ 0, row });
    }
    const size_t mouths = mgNests.size();
    for (size_t i = 0; i + 1 < mouths; ++i)
        mgNests.push_back(Coord{ 0, (mgNests[i].y + mgNests[i + 1].y) / 2 });
    if (mgNests.size() < 2) {
        mgNests.push_back(Coord{ 0, y > 20 ? y - 20 : 0 });
        mgNests.push_back(Coord{ 0, std::min(common::SIZEY - 1, y + 20) });
    }
}

bool Level::isMgNest(const Coord& c) const
{
    for (const auto& n : mgNests) if (n.y == c.y && c.x <= n.x + 1) return true;
    return false;
}

void Level::barrierFireTick()
{
    if (!holdOfficer || !isAlive(holdOfficer) || mgNests.empty()) return;
    if (barrierShoutCooldown) --barrierShoutCooldown;
    if (barrierCooldown && --barrierCooldown) return;
    barrierCooldown = 2;
    const uint32_t line = holdOfficer->coord.x;
    const PlayerNS::Player* player = getPlayer();
    for (Person* p : persons) {
        if (p == holdOfficer || p->stats.health == 0 || !isRussianSoldier(*this, p)) continue;
        if (p->coord.x + DESERTER_GAP >= line) continue;
        const bool isPlayer = p == player;
        if (isPlayer && !barrierShoutCooldown) {
            barrierShoutCooldown = 10;
            holdOfficer->say("Ei askeltakaan taaksep\xC3\xA4in!");
            if (printer) printer->showMessage("Upseeri radiossa: 'Ei askeltakaan taaksep\xC3\xA4in! Sulkuosasto, tulta karkureihin!'", this, false);
        }
        // the nearest gun opens up; the trench is no help, the fire comes along it from behind
        const Coord* nest = &mgNests.front();
        for (const auto& n : mgNests) if (n.distance(p->coord) < nest->distance(p->coord)) nest = &n;
        addAnimation(new Animation(Coord{ nest->x + 2, nest->y }, "*", 2));
        audio.addSound(*nest, SoundType::Gunshot,
            "TATATATA! Omat konekiv\xC3\xA4\xC3\xA4rit ampuvat!", "Konekiv\xC3\xA4\xC3\xA4rin sarja!", "...konekiv\xC3\xA4\xC3\xA4rin tulta...", 60);
        HurtBy who(this, "oma konekiv\xC3\xA4\xC3\xA4ri (sulkuosasto)");
        const bool hit = common::random(40);
        if (hit) p->damage(2 + std::rand() % 3);
        if (isPlayer && printer)
            printer->showMessage(hit ? "Omat konekiv\xC3\xA4\xC3\xA4rit ampuvat sinua! Sulkuosasto ei kysele."
                                     : "Omien konekiv\xC3\xA4\xC3\xA4rien luodit p\xC3\xB6lytt\xC3\xA4v\xC3\xA4t maata vieress\xC3\xA4si!", this, false);
        else if (hit) p->say("Omat ampuvat!");
    }
}

void Level::assaultTick()
{
    if (warStage < 3) return;
    if (warTicks == BARRAGE_TICK) {
        assaultPhase = AssaultPhase::Barrage;
        if (printer) printer->showMessage("TYKIST\xC3\x96KESKITYS! Koko linja on tulen alla! Haudan pohjalle!", this, false);
        Subtitles::say("[Maa j\xC3\xA4risee. Taivas on t\xC3\xA4ynn\xC3\xA4 ulvontaa.]", "barrage");
    }
    if (assaultPhase == AssaultPhase::Barrage) {
        fireSalvo(2 + std::rand() % 3);
        if (warTicks + 1 >= ASSAULT_TICK) {
            assaultPhase = AssaultPhase::Assault;
            if (printer) printer->showMessage("Keskitys taukoaa... TELAKETJUJEN KOLINAA! Bradleyt tulevat!", this, false);
            Subtitles::say("[Keskityksen j\xC3\xA4lkeen hiljaisuus. Sitten moottorit.]", "assault");
            const PlayerNS::Player* player = getPlayer();
            const uint32_t y = player ? player->coord.y : common::PLAYER_START_Y;
            const uint32_t n = 2 + std::rand() % 2;
            for (uint32_t i = 0; i < n; ++i) {
                const int row = std::max(1, std::min((int)common::SIZEY - 3, (int)y - 12 + (int)i * 12 + std::rand() % 5 - 2));
                addVehicle(new Bradley(Coord{ TrenchNS::FRONT_W - 5, (uint32_t)row }, bradleyStopX(*this, (uint32_t)row)));
            }
        }
    }
}

void Level::warTick()
{
    if (!isWarLevel || playerDead) return;
    ++warTicks;
    exposureTick();
    artilleryTick();
    assaultTick();
    PlayerNS::Player* player = getPlayer();

    // the third level: nobody falls back while the officer holds the line
    if (warStage >= 3 && !rearOpen) {
        if (holdOfficer && !isAlive(holdOfficer)) { holdOfficer = nullptr; officerFell = true; }
        if (holdOfficer) {
            barrierFireTick();
            if (warTicks == WAR_SHIFT_TICKS && printer)
                printer->showMessage("Radio: 'Vuorovaihto peruttu. Pit\xC3\xA4k\xC3\xA4\xC3\xA4 asemat, kunnes upseeri toisin k\xC3\xA4skee.'", this, false);
            return;
        }
        if (officerFell) {
            if (printer) printer->showMessage("Radio: 'Upseeri kaatui! Vet\xC3\xA4ytyk\xC3\xA4\xC3\xA4! Kaikki selustaan!'", this, false);
            rearOpen = true;
            if (warTicks < WAR_SHIFT_TICKS) warTicks = WAR_SHIFT_TICKS;
        }
    }

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
