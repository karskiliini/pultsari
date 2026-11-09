// Unit tests for game logic: shops, break-ins, drones, exhaust, vision.
// Build and run with `make test`.
#include "animation.hpp"
#include "building.hpp"
#include "input.hpp"
#include "common.hpp"
#include "item.hpp"
#include "level.hpp"
#include "mask.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "printerweb.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using PlayerNS::Player;
using std::string;

// ---- tiny test framework ----
static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; \
    std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << ": " #cond "\n"; } } while (0)

struct Test { const char* name; std::function<void()> fn; };
static std::vector<Test>& tests() { static std::vector<Test> t; return t; }
struct Reg { Reg(const char* n, std::function<void()> f) { tests().push_back({ n, f }); } };
#define TEST(name) static void name(); static Reg reg_##name(#name, name); static void name()

static bool contains(const string& s, const string& part) { return s.find(part) != string::npos; }

// ---- fixtures ----

// a printer that draws nothing and remembers every message shown
class RecordingPrinter : public PrinterWeb {
public:
    std::vector<string> log;
    void printMessage() override { log.push_back(msg); }
    void print(Level*) override { }
    bool saw(const string& part) const {
        for (const auto& m : log) if (contains(m, part)) return true;
        return false;
    }
};

// menu keys come from stdin: feed them through a pipe
static void feedKeys(const string& keys)
{
    int fds[2];
    if (pipe(fds) != 0) return;
    if (write(fds[1], keys.data(), keys.size()) < 0) { }
    close(fds[1]);
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    clearerr(stdin);
}

struct World {
    RecordingPrinter printer;
    Level level{ 1 };
    Player player;
    VisionNS::Mask mask{ &level };

    explicit World(Coord at = Coord{ 50, 50 }) {
        common::realtimeEnabled = true;
        common::animsEnabled = true;
        level.printer = &printer;
        printer.mask = &mask;
        printer.player = &player;
        player.printer = &printer;
        player.coord = at;
        level.addPerson(&player);
    }
    template <class B> B* add(B* b, uint32_t x, uint32_t y) { b->setOffset(x, y); level.addBuilding(b); return b; }
    template <class P> P* spawn(P* p) { p->printer = &printer; level.addPerson(p); return p; }
    size_t count(PersonType t) const {
        size_t n = 0;
        for (auto p : level.persons) if (p->type == t && p->stats.health > 0) ++n;
        return n;
    }
    bool said(const Person* who, const string& part) const {
        for (auto a : level.animations) if (a->isBubble && a->owner == who && contains(a->text, part)) return true;
        return false;
    }
};

// ---- Russian shops ----

TEST(shop_sells_food_for_money)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.stats.money = 20;
    const uint32_t had = w.player.inventory.lenkki;
    feedKeys("1");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 14);
    CHECK(w.player.inventory.lenkki == had + 1);
}

TEST(shop_kiosk_sells_vodka)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Kioski), 10, 10);
    w.player.stats.money = 30;
    const uint32_t had = w.player.inventory.vodka;
    feedKeys("4");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 5);
    CHECK(w.player.inventory.vodka == had + 1);
}

TEST(shop_refuses_the_broke)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.stats.money = 1;
    const uint32_t had = w.player.inventory.lenkki;
    feedKeys("1");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 1);
    CHECK(w.player.inventory.lenkki == had);
    CHECK(contains(msg, "Myyj"));
}

TEST(shop_leaving_buys_nothing)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.stats.money = 20;
    feedKeys("x");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 20);
    CHECK(!msg.empty());
}

TEST(shop_queue_rage_comes_from_a_real_customer)
{
    World w(Coord{ 15, 12 });
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    auto* drunk = w.spawn(new Juoppo(Coord{ 16, 12 }));
    bool hit = false;
    for (int i = 0; i < 300 && !hit; ++i) {
        w.player.stats.health = 1000;
        feedKeys("x");
        string msg;
        k->interact(&w.player, msg);
        hit = contains(msg, w.level.nameOf(drunk)) && w.player.stats.health < 1000;
    }
    CHECK(hit);
    CHECK(w.level.animations.size() > 0);
}

TEST(shop_without_customers_has_no_rage)
{
    World w(Coord{ 15, 12 });
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    for (int i = 0; i < 100; ++i) {
        w.player.stats.health = 1000;
        feedKeys("x");
        string msg;
        k->interact(&w.player, msg);
    }
    CHECK(w.player.stats.health == 1000);
}

// ---- Alko ----

TEST(alko_lets_everyone_in)
{
    World w;
    auto* a = w.add(new Alko(), 10, 10);
    string msg;
    w.player.stats.promilles = 30;
    w.player.stats.money = 0;
    CHECK(a->getEnterMsg(&w.player, msg));
}

TEST(alko_counter_refuses_drunks_without_reading_keys)
{
    World w;
    auto* a = w.add(new Alko(), 10, 10);
    w.player.stats.promilles = 20;
    w.player.stats.money = 50;
    string msg;
    a->getInteractMsg(&w.player, msg);
    CHECK(contains(msg, "liian"));
    const uint32_t had = w.player.inventory.kalja;
    feedKeys("1");
    a->interact(&w.player, msg);
    CHECK(w.player.inventory.kalja == had);
    CHECK(InputNS::Input::getMenuKey() == '1'); // the key was left alone
}

TEST(alko_sells_beer)
{
    World w;
    auto* a = w.add(new Alko(), 10, 10);
    w.player.stats.promilles = 5;
    w.player.stats.money = 20;
    const uint32_t had = w.player.inventory.kalja;
    feedKeys("1");
    string msg;
    a->interact(&w.player, msg);
    CHECK(w.player.inventory.kalja == had + 1);
    CHECK(w.player.stats.money == 10);
}

TEST(alko_jaguar_needs_fifteen)
{
    World w;
    auto* a = w.add(new Alko(), 10, 10);
    w.player.stats.promilles = 5;
    w.player.stats.money = 12;
    const uint32_t had = w.player.inventory.lonkka;
    feedKeys("2");
    string msg;
    a->interact(&w.player, msg);
    CHECK(w.player.inventory.lonkka == had);
    CHECK(w.player.stats.money == 12);
}

// ---- closed Western shops: back room and break-ins ----

TEST(back_room_sells_vodka)
{
    World w;
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 10, 10);
    string msg;
    CHECK(s->walkable);
    CHECK(s->getEnterMsg(&w.player, msg));
    w.player.stats.money = 30;
    const uint32_t had = w.player.inventory.vodka;
    feedKeys("1");
    s->interact(&w.player, msg);
    CHECK(w.player.inventory.vodka == had + 1);
    CHECK(w.player.stats.money == 10);
}

TEST(front_door_is_locked)
{
    World w;
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 10, 10);
    CHECK(contains(s->getWalkMsg(), "lukossa"));
    CHECK(s->door == DirectionNS::down);
}

TEST(stone_smashes_shop_window_and_alerts_police)
{
    World w(Coord{ 23, 15 });
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20); // front windows at (23,20) and (31,20)
    auto* cop = w.spawn(new Cop(Coord{ 60, 15 }));
    w.player.inventory.kivet = 1;
    w.player.executeThrow(6, DirectionNS::down);
    CHECK(s->smashed);
    CHECK(s->isBreach(Coord{ 23, 20 }));
    CHECK(!s->hitWall(Coord{ 23, 20 }));     // the hole can be walked through
    CHECK(s->alarmTicks > 0);
    CHECK(s->burglar == &w.player);
    CHECK(cop->chaseTarget == &w.player);    // heard the glass and the alarm
    CHECK(w.printer.saw("Ikkuna hajoaa"));
}

TEST(light_throw_does_not_break_window)
{
    World w(Coord{ 23, 15 });
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
    w.player.executeThrow(3, DirectionNS::down); // a sausage
    CHECK(!s->smashed);
}

TEST(alarm_brings_a_patrol_when_burglar_stays)
{
    World w(Coord{ 23, 26 });
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
    s->smashWindow(Coord{ 23, 20 }, &w.player);
    const size_t before = w.count(poliisi);
    for (int i = 0; i < 20; ++i) s->npcAct();
    CHECK(w.count(poliisi) >= before + 1);
    bool chasing = false;
    for (auto p : w.level.persons) if (p->type == poliisi && p->chaseTarget == &w.player) chasing = true;
    CHECK(chasing);
}

TEST(no_patrol_if_burglar_ran_away)
{
    World w(Coord{ 23, 26 });
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
    s->smashWindow(Coord{ 23, 20 }, &w.player);
    w.player.coord = Coord{ 150, 150 };
    for (int i = 0; i < 40; ++i) s->npcAct();
    CHECK(w.count(poliisi) == 0);
    CHECK(s->alarmTicks == 0);
}

TEST(break_in_loot_is_rare)
{
    int loot = 0;
    for (int i = 0; i < 200; ++i) {
        World w(Coord{ 23, 26 });
        auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
        s->smashWindow(Coord{ 23, 20 }, nullptr);
        if (!w.level.items.empty()) ++loot;
    }
    CHECK(loot > 20);
    CHECK(loot < 110); // usually there is nothing but dust
}

// ---- drones ----

static float heading(const Drone& d) { return std::atan2(d.vy, d.vx); }

TEST(homing_drone_curves_and_hits_a_shop)
{
    for (int run = 0; run < 10; ++run) {
        World w(Coord{ 5, 5 });
        auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 200, 100);
        w.level.launchDrone(k);
        CHECK(w.level.drones.size() == 1);
        const float h0 = heading(w.level.drones[0]);
        float maxTurn = 0;
        int frames = 0;
        while (!w.level.drones.empty() && frames < 20 * 120) {
            w.level.vehicleFrame(50);
            if (!w.level.drones.empty()) {
                float d = std::fabs(heading(w.level.drones[0]) - h0);
                if (d > 3.14159f) d = 6.28318f - d;
                maxTurn = std::max(maxTurn, d);
            }
            ++frames;
        }
        CHECK(w.level.drones.empty());   // it reached the target
        CHECK(maxTurn > 0.3f);           // and did not fly in a straight line
        CHECK(!k->breaches.empty());     // the blast holed the shop
    }
}

TEST(some_flyovers_bank)
{
    World w;
    int banking = 0;
    for (int i = 0; i < 100; ++i) {
        w.level.drones.clear();
        w.level.launchDrone(nullptr);
        if (w.level.drones[0].turn != 0) ++banking;
    }
    CHECK(banking > 20);
    CHECK(banking < 80);
}

// ---- exhaust ----

TEST(breathing_exhaust_shows_a_message)
{
    World w(Coord{ 80, 80 });
    w.level.gas.push_back({ 80.0f, 80.0f, 6u, false });
    w.level.skyTick(); // builds the gas grid
    w.level.drones.clear();
    CHECK(w.level.gasAt(w.player.coord) == 1);
    for (int i = 0; i < 400; ++i) { w.player.stats.health = 100; w.level.npcFrame(50); }
    CHECK(w.printer.saw("pakokaasua henkeen") || w.printer.saw("hiilivety") ||
          w.printer.saw("pakokaasu") || w.printer.saw("lyijyll"));
}

TEST(no_exhaust_message_in_clean_air)
{
    World w(Coord{ 80, 80 });
    w.level.skyTick();
    for (int i = 0; i < 200; ++i) w.level.npcFrame(50);
    CHECK(!w.printer.saw("pakokaasua henkeen"));
}

// ---- vomit ----

TEST(walking_in_vomit_gets_a_comment)
{
    World w(Coord{ 60, 60 });
    auto* drunk = w.spawn(new Juoppo(Coord{ 63, 60 }));
    drunk->visionRange = 20;
    Oksennus vomit(Coord{ 60, 60 });
    for (int i = 0; i < 50; ++i) vomit.interact(&w.player);
    CHECK(w.said(drunk, "korkeakulttuuria") || w.said(drunk, "baletti") || w.said(drunk, "kulttuuri"));
}

TEST(nobody_comments_unseen_vomit)
{
    World w(Coord{ 60, 60 });
    auto* drunk = w.spawn(new Juoppo(Coord{ 90, 90 }));
    Oksennus vomit(Coord{ 60, 60 });
    for (int i = 0; i < 50; ++i) vomit.interact(&w.player);
    CHECK(!w.said(drunk, "kulttuuri"));
}

// ---- vision ----

TEST(shop_name_shows_when_wall_is_in_sight)
{
    World w(Coord{ 95, 102 });
    w.add(new Kauppa("TESTNAME", Kauppa::Kind::Produkty), 100, 100); // name row y=102, inside
    w.mask.enable();
    w.level.updateCamera();
    w.mask.init();
    CHECK(!w.mask.visible(Coord{ 107, 102 }));  // the plate itself is behind the wall
    CHECK(w.mask.visible(Coord{ 100, 102 }));   // but the wall is in sight
    string row;
    for (const auto& cell : w.printer.rowCells(&w.level, 102 - (uint32_t)w.level.cameraY)) row += cell.first;
    CHECK(contains(row, "TESTNAME"));
}

TEST(shop_name_hidden_when_no_wall_in_sight)
{
    World w(Coord{ 107, 160 });
    w.add(new Kauppa("TESTNAME", Kauppa::Kind::Produkty), 100, 100);
    w.mask.enable();
    w.player.coord = Coord{ 107, 125 };
    w.level.updateCamera();
    w.mask.init();
    string row;
    for (const auto& cell : w.printer.rowCells(&w.level, 102 - (uint32_t)w.level.cameraY)) row += cell.first;
    CHECK(!contains(row, "TESTNAME"));
}

// ---- timing ----

TEST(frame_clock_holds_twenty_fps)
{
    using clock = std::chrono::steady_clock;
    common::frameSleep(50); // sync
    const auto t0 = clock::now();
    for (int i = 0; i < 20; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15)); // a frame's work
        common::frameSleep(50);
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - t0).count();
    CHECK(ms >= 950 && ms <= 1100);
}

int main(int argc, char** argv)
{
    std::srand(12345);
    const string only = argc > 1 ? argv[1] : "";
    int ran = 0;
    for (const auto& t : tests()) {
        if (!only.empty() && !contains(t.name, only)) continue;
        const int before = failures;
        t.fn();
        ++ran;
        std::cout << (failures == before ? "ok   " : "FAIL ") << t.name << std::endl;
    }
    std::cout << ran << " tests, " << checks << " checks, " << failures << " failures" << std::endl;
    return failures ? 1 : 0;
}
