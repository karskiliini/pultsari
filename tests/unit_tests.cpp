// Unit tests for game logic: shops, break-ins, drones, exhaust, vision.
// Build and run with `make test`.
#include "aagun.hpp"
#include "animation.hpp"
#include "building.hpp"
#include "input.hpp"
#include "common.hpp"
#include "events.hpp"
#include "gaze.hpp"
#include "disease.hpp"
#include "item.hpp"
#include "level.hpp"
#include "mask.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "printerweb.hpp"
#include "translate.hpp"
#include "vehicle.hpp"
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <algorithm>
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
        mask.glances = false; // keep vision deterministic
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

TEST(gun_shop_sells_a_pistol_with_rounds)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Oruzhie), 10, 10);
    w.player.stats.money = 50;
    const uint32_t guns = w.player.inventory.pistooli, rounds = w.player.inventory.ammukset;
    feedKeys("2");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 15);
    CHECK(w.player.inventory.pistooli == guns + 1);
    CHECK(w.player.inventory.ammukset == rounds + 6);
}

TEST(gun_shop_sells_a_kalashnikov_as_the_fifth_item)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Oruzhie), 10, 10);
    w.player.stats.money = 100;
    const uint32_t ak = w.player.inventory.rynkky, rounds = w.player.inventory.ammukset;
    feedKeys("5");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 10);
    CHECK(w.player.inventory.rynkky == ak + 1);
    CHECK(w.player.inventory.ammukset == rounds + 30);
}

TEST(gun_shop_refuses_the_broke_and_food_shops_have_no_fifth_item)
{
    World w;
    auto* gun = w.add(new Kauppa("TEST", Kauppa::Kind::Oruzhie), 10, 10);
    w.player.stats.money = 20;
    const uint32_t ak = w.player.inventory.rynkky;
    feedKeys("5");
    string msg;
    gun->interact(&w.player, msg);
    CHECK(w.player.stats.money == 20);
    CHECK(w.player.inventory.rynkky == ak);

    auto* food = w.add(new Kauppa("FOOD", Kauppa::Kind::Produkty), 40, 10);
    w.player.stats.money = 200;
    feedKeys("5");
    food->interact(&w.player, msg);
    CHECK(w.player.stats.money == 200);
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

TEST(shop_name_hidden_while_inside)
{
    World w(Coord{ 104, 103 });   // inside, on the row below the name plate
    w.add(new Kauppa("TESTNAME", Kauppa::Kind::Produkty), 100, 100);
    w.mask.enable();
    w.level.updateCamera();
    w.mask.init();
    string row;
    for (const auto& cell : w.printer.rowCells(&w.level, 102 - (uint32_t)w.level.cameraY)) row += cell.first;
    CHECK(!contains(row, "TESTNAME"));
}

// walk with keys: d = right, w = up, a = left, s = down
static float walkHeading(const string& keys)
{
    VisionNS::Heading h;
    for (char k : keys) h.step(k == 'd' ? 1 : k == 'a' ? -1 : 0, k == 's' ? 1 : k == 'w' ? -1 : 0);
    return h.degrees();
}

TEST(heading_is_weighted_average_of_steps)
{
    // zigzag right/up averages to north-east
    const float a = walkHeading("dwdwdwdwdwdw"), b = walkHeading("dwdwdwdwdwdwd");
    CHECK((a + b) / 2.f > 40.f && (a + b) / 2.f < 50.f);
    // one step up after a long run right: still mostly right
    const float once = walkHeading("dddddw");
    CHECK(once > 10.f && once < 30.f);
    // two steps up: clearly more, about 45
    const float twice = walkHeading("dddddww");
    CHECK(twice > 38.f && twice < 52.f);
}

TEST(vision_reaches_far_ahead_and_short_behind)
{
    World w(Coord{ 50, 50 });
    w.mask.enable();
    w.level.updateCamera();
    w.mask.init();                              // facing right at start
    CHECK(w.mask.visible(Coord{ 62, 50 }));     // 12 ahead
    CHECK(!w.mask.visible(Coord{ 42, 50 }));    // 8 behind
    CHECK(w.mask.visible(Coord{ 48, 50 }));     // right behind your back
    for (int i = 0; i < 8; ++i) w.player.heading.step(-1, 0);
    w.mask.init();                              // turned around
    CHECK(w.mask.visible(Coord{ 42, 50 }));
    CHECK(!w.mask.visible(Coord{ 62, 50 }));
}

TEST(vision_stares_at_nearest_person_in_front)
{
    World w(Coord{ 50, 50 });
    w.spawn(new Mummo(Coord{ 56, 42 }));        // up-right, inside the front half
    w.mask.enable();
    w.level.updateCamera();
    w.mask.init();
    CHECK(w.mask.visible(Coord{ 60, 42 }));     // next to the granny, where the eyes go
    CHECK(!w.mask.visible(Coord{ 62, 56 }));    // straight-ish ahead but off to the other side
}

TEST(drunk_vision_cone_is_narrower)
{
    CHECK(VisionNS::coneScale(2) == 1.f);
    CHECK(VisionNS::coneScale(15) < 1.f);
    CHECK(VisionNS::coneScale(40) == VisionNS::DRUNK_CONE_SCALE);
    World w(Coord{ 50, 50 });
    w.mask.enable();
    w.level.updateCamera();
    const Coord aside { 56, 45 };               // ~40 degrees off the walking direction
    w.mask.init();
    CHECK(w.mask.visible(aside));
    w.player.stats.promilles = 30;
    w.mask.init();
    CHECK(!w.mask.visible(aside));
    CHECK(w.mask.visible(Coord{ 60, 50 }));     // still sees straight ahead
}

// ---- foreign recruits ----

TEST(african_recruit_talks_about_the_job_he_was_promised)
{
    World w(Coord{ 60, 60 });
    auto* r = w.spawn(new Varvatty(Coord{ 62, 60 }, rekryytti));
    for (int i = 0; i < 5; ++i) { r->coord = Coord{ 62, 60 }; r->npcAct(); }
    bool spoke = false;
    for (auto a : w.level.animations) if (a->isBubble && a->owner == r) spoke = true;
    CHECK(spoke);
    CHECK(r->typeToChar() == "h");
}

TEST(african_recruit_shares_his_ration_once)
{
    World w(Coord{ 60, 60 });
    auto* r = w.spawn(new Varvatty(Coord{ 61, 60 }, rekryytti));
    const uint32_t had = w.player.inventory.lenkki;
    string msg;
    r->interact(msg, &w.player);
    r->interact(msg, &w.player);
    CHECK(w.player.inventory.lenkki == had + 1);
}

TEST(north_korean_marches_and_hits_back)
{
    World w(Coord{ 150, 150 });
    auto* k = w.spawn(new Varvatty(Coord{ 60, 60 }, korealainen));
    const Coord start = k->coord;
    for (int i = 0; i < 20; ++i) k->npcAct();
    CHECK(k->coord != start);
    CHECK(k->typeToChar() == "n");
    bool hit = false;
    common::realtimeEnabled = false; // no post-hit grace period for this check
    for (int i = 0; i < 100 && !hit; ++i) {
        w.player.stats.health = 100;
        k->stats.health = 100;
        string msg;
        k->interact(msg, &w.player);
        hit = w.player.stats.health < 100;
    }
    common::realtimeEnabled = true;
    CHECK(hit);
}

// ---- petrol station ----

TEST(petrol_station_sells_hot_dogs)
{
    World w(Coord{ 30, 30 });
    auto* st = w.add(new Bensa(), 100, 100);
    w.player.stats.money = 20;
    const uint32_t had = w.player.inventory.lenkki;
    feedKeys("1");
    string msg;
    st->interact(&w.player, msg);
    CHECK(w.player.inventory.lenkki == had + 1);
    CHECK(w.player.stats.money == 14);
}

TEST(petrol_station_says_when_it_is_dry)
{
    World w(Coord{ 30, 30 });
    auto* st = w.add(new Bensa(), 100, 100);
    string msg;
    st->outOfFuel = true;
    st->getInteractMsg(&w.player, msg);
    CHECK(contains(msg, "LOPPU"));
}

TEST(people_at_the_petrol_station_complain_about_prices)
{
    World w(Coord{ 30, 30 });
    auto* st = w.add(new Bensa(), 100, 100);
    auto* guy = w.spawn(new Juoppo(Coord{ 106, 107 }));
    for (int i = 0; i < 40; ++i) st->npcAct();
    CHECK(w.said(guy, "bensa") || w.said(guy, "Hinnat") || w.said(guy, "Jalostamo") || w.said(guy, "tankkaan") ||
          w.said(guy, "litraa") || w.said(guy, "kopeekoita") || w.said(guy, "öljymaa"));
}

// ---- Medvedev ----

static const KremlinArea everywhere { { 0, 0 }, { 200, 200 } };

static bool foamOnScreen(const World& w)
{
    for (auto a : w.level.animations) if (!a->isBubble && a->color && string(a->color) == "1;97") return true;
    return false;
}

TEST(medvedev_rants_about_nukes_and_winds_himself_up)
{
    World w(Coord{ 150, 150 });
    auto* m = w.spawn(new Medvedev(Coord{ 60, 60 }, everywhere));
    common::realtimeEnabled = false;
    m->npcAct();
    CHECK(m->froth == 1);
    bool spoke = false;
    for (auto a : w.level.animations) if (a->isBubble && a->owner == m) spoke = true;
    CHECK(spoke);
    for (int i = 0; i < 6; ++i) m->npcAct();
    CHECK(m->froth > 1);
    common::realtimeEnabled = true;
}

TEST(medvedev_foams_at_the_mouth_when_wound_up)
{
    World w(Coord{ 150, 150 });
    auto* m = w.spawn(new Medvedev(Coord{ 60, 60 }, everywhere));
    m->froth = Medvedev::MAX_FROTH - 2;
    for (int i = 0; i < 60 && !foamOnScreen(w); ++i) m->npcAct();
    CHECK(foamOnScreen(w));
}

TEST(medvedev_posts_to_telegram_and_passes_out)
{
    World w(Coord{ 150, 150 });
    auto* m = w.spawn(new Medvedev(Coord{ 60, 60 }, everywhere));
    common::realtimeEnabled = false;
    Medvedev::latestPost.clear();
    m->froth = Medvedev::MAX_FROTH - 1;
    for (int i = 0; i < 5 && !m->passedOut; ++i) m->npcAct();
    CHECK(m->passedOut > 0);
    CHECK(!Medvedev::latestPost.empty());
    CHECK(w.said(m, "TELEGRAM"));
    // he lies still in his foam, then wakes up for another round
    const Coord at = m->coord;
    for (int i = 0; i < 20 && m->passedOut; ++i) m->npcAct();
    CHECK(m->passedOut == 0);
    CHECK(m->coord == at);
    CHECK(m->froth < Medvedev::FOAM_AT);
    common::realtimeEnabled = true;
}

TEST(medvedev_sprays_foam_when_you_hit_him)
{
    World w(Coord{ 60, 60 });
    auto* m = w.spawn(new Medvedev(Coord{ 61, 60 }, everywhere));
    m->stats.health = 100;
    m->froth = Medvedev::FOAM_AT;
    string msg;
    m->interact(msg, &w.player);
    CHECK(contains(msg, "vaahtoa"));
    CHECK(foamOnScreen(w));
}

TEST(citizens_read_medvedevs_posts_aloud)
{
    World w(Coord{ 60, 60 });
    Medvedev::latestPost = "TESTIPOSTAUS";
    auto* k = w.spawn(new Kansalainen(Coord{ 62, 60 }));
    bool read = false;
    for (int i = 0; i < 3000 && !read; ++i) { k->coord = Coord{ 62, 60 }; k->npcAct(); read = w.said(k, "TESTIPOSTAUS"); }
    CHECK(read);
    Medvedev::latestPost.clear();
}

// ---- translations ----

TEST(russian_in_a_message_gets_a_finnish_translation)
{
    World w;
    w.printer.showMessage("ХРУЩЁВКА. Seinässä lukee jotain.", &w.level, false);
    CHECK(w.printer.saw("ХРУЩЁВКА (HRUŠTŠOVKA"));
    CHECK(contains(withTranslation("Myyjä: 'Спасибо. Seuraava!'"), "Спасибо (Kiitos). Seuraava"));
    CHECK(contains(withTranslation("Vova: 'Я ДЗЮДОИСТ! Я тебя сломаю!'"), "(OLEN JUDOKA! Murskaan sinut!)"));
    CHECK(contains(withTranslation("ОХРАНА! ОХРАНА!!"), "(VARTIJAT! VARTIJAT!!)"));
    CHECK(contains(withTranslation("Груз 200 ei ole pivomerkki."), "Груз 200 (Lasti 200, eli kaatuneen ruumis) ei"));
    // already translated, or no Russian: left alone
    CHECK(withTranslation("БУТЫРКА (vankila)") == "БУТЫРКА (vankila)");
    CHECK(withTranslation("Pelkkää suomea.") == "Pelkkää suomea.");
}

TEST(speech_bubbles_carry_the_translation_too)
{
    World w;
    auto* k = w.spawn(new Kansalainen(Coord{ 52, 50 }));
    k->say("Спасибо, брат!");
    CHECK(w.said(k, "Spasibo, brat! (Kiitos, veli!)"));
}

// every bit of Russian in the game's own text has a dictionary entry
TEST(all_russian_in_the_code_is_in_the_dictionary)
{
    std::vector<string> files;
    for (const char* dir : { "src", "include" }) {
        if (DIR* d = opendir(dir)) {
            while (dirent* e = readdir(d)) {
                const string n = e->d_name;
                if (n == "intro.cpp") continue; // the title drips in letter by letter
                if (contains(n, ".cpp") || contains(n, ".hpp")) files.push_back(string(dir) + "/" + n);
            }
            closedir(d);
        }
    }
    CHECK(files.size() > 10);
    int missing = 0;
    for (const auto& f : files) {
        std::ifstream in(f);
        std::stringstream ss;
        ss << in.rdbuf();
        const string raw = ss.str();
        // turn C escapes like \xD0\x9F into the bytes they stand for
        string src;
        for (size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] == '\\' && i + 3 < raw.size() && raw[i + 1] == 'x' && isxdigit((unsigned char)raw[i + 2]) && isxdigit((unsigned char)raw[i + 3])) {
                src += (char)std::stoi(raw.substr(i + 2, 2), nullptr, 16);
                i += 3;
            } else src += raw[i];
        }
        for (size_t i = 0; i < src.size(); ++i) {
            const unsigned char b = src[i];
            if (b != 0xD0 && b != 0xD1) continue;
            size_t j = i, end = i;
            while (j + 1 < src.size()) {
                const unsigned char c = src[j];
                if (c == 0xD0 || c == 0xD1) { j += 2; end = j; }
                else if (c == ' ' || c == ',' || c == '.' || c == '!' || c == '?' || c == '-') ++j;
                else if (c >= '0' && c <= '9') { ++j; end = j; }
                else break;
            }
            const string run = src.substr(i, end - i);
            if (translateRussian(run).empty()) {
                ++missing;
                std::cerr << "  no translation in " << f << ": " << run << "\n";
            }
            i = end;
        }
    }
    CHECK(missing == 0);
}

// ---- metro ----

TEST(metro_takes_you_to_the_next_station)
{
    World w(Coord{ 30, 30 });
    auto* a = w.add(new Metro("A"), 20, 20);
    auto* b = w.add(new Metro("B"), 200, 120);
    w.player.stats.money = 100;
    string msg;
    CHECK(a->getEnterMsg(&w.player, msg));
    a->interact(&w.player, msg);
    CHECK(w.player.coord == b->getSpawn());
    CHECK(w.player.stats.money == 40);
    CHECK(contains(msg, "B"));
    w.player.stats.money = 100;               // pay this time too, no fare-dodging luck involved
    b->interact(&w.player, msg);              // and the line loops back round
    CHECK(w.player.coord == a->getSpawn());
}

TEST(metro_fare_dodgers_sometimes_get_caught)
{
    int rode = 0, caught = 0;
    for (int i = 0; i < 200; ++i) {
        World w(Coord{ 30, 30 });
        auto* a = w.add(new Metro("A"), 20, 20);
        auto* b = w.add(new Metro("B"), 200, 120);
        w.player.stats.money = 0;
        string msg;
        a->interact(&w.player, msg);
        if (w.player.coord == b->getSpawn()) ++rode; else ++caught;
    }
    CHECK(rode > 100);
    CHECK(caught > 20);
}

// ---- supermarket: lure the guards away and slip in ----

struct Market {
    World w;
    Hypermarket* hm;
    Vartija* g1;
    Vartija* g2;
    Coord door;
    Market() : w(Coord{ 113, 60 }) {
        hm = w.add(new Hypermarket(), 100, 40);          // door on the bottom wall
        door = hm->getDoor();
        g1 = w.spawn(new Vartija(Coord{ door.x, door.y + 1 }));
        g2 = w.spawn(new Vartija(Coord{ door.x + 2, door.y + 1 }));
        g1->homePos = g2->homePos = door;
    }
};

TEST(market_guards_block_the_door)
{
    Market m;
    string msg;
    CHECK(!m.hm->getEnterMsg(&m.w.player, msg));
    CHECK(contains(msg, "Vartija"));
}

TEST(market_lured_guard_leaves_the_door_then_gives_up)
{
    Market m;
    m.w.player.coord = Coord{ m.door.x, m.door.y + 4 };
    m.g1->provoke(&m.w.player);
    uint32_t farthest = 0;
    // the player runs off down the street; the guard follows for a while
    for (int i = 0; i < 60; ++i) {
        if (m.w.player.coord.y < m.door.y + 40) m.w.player.coord.y += 1;
        m.g1->npcAct();
        farthest = std::max(farthest, m.g1->coord.distance(m.door));
    }
    CHECK(farthest > 6);            // he left his post
    CHECK(!m.g1->chasing);          // and gave up
    for (int i = 0; i < 200 && m.g1->coord.distance(m.door) > 3; ++i) m.g1->npcAct();
    CHECK(m.g1->coord.distance(m.door) <= 3); // back at the door
}

TEST(market_noise_draws_a_guard)
{
    Market m;
    m.w.player.coord = Coord{ 160, 90 };   // well out of sight
    const Coord noise { m.door.x + 10, m.door.y + 6 };
    m.g1->hearNoise(noise);
    uint32_t closest = 1000;
    for (int i = 0; i < 30; ++i) { m.g1->npcAct(); closest = std::min(closest, m.g1->coord.distance(noise)); }
    CHECK(closest <= 2);
}

TEST(market_thrown_item_makes_guards_look)
{
    Market m;
    m.w.player.coord = Coord{ m.door.x + 4, m.door.y + 1 };
    m.w.player.executeThrow(6, DirectionNS::down);   // a stone clatters onto the street
    CHECK(m.g1->investigateTicks > 0 || m.g2->investigateTicks > 0);
}

TEST(market_empty_door_lets_you_in_and_the_theft_alarm_brings_them_back)
{
    Market m;
    m.g1->coord = Coord{ m.door.x - 15, m.door.y + 8 };
    m.g2->coord = Coord{ m.door.x + 15, m.door.y + 8 };
    string msg;
    CHECK(m.hm->getEnterMsg(&m.w.player, msg));
    const auto before = m.w.player.inventory;
    m.hm->interact(&m.w.player, msg);
    const auto& after = m.w.player.inventory;
    const uint32_t gained = (after.vodka - before.vodka) + (after.kalja - before.kalja) + (after.lenkki - before.lenkki) +
                            (after.kalat - before.kalat) + (after.lonkka - before.lonkka) + (after.bansku - before.bansku);
    CHECK(gained >= 1);
    CHECK(contains(msg, "VARAS"));
    CHECK(m.g1->chasing && m.g2->chasing);
}

// ---- empty bottles: the return hatch and the people who want your sack ----

TEST(picked_up_bottle_goes_into_the_sack)
{
    World w;
    Bona b(Coord{ 51, 50 });
    CHECK(b.interact(&w.player));
    CHECK(w.player.inventory.pullot == 1);
}

TEST(kopecks_roll_over_into_roubles)
{
    Stats s;
    s.money = 3;
    s.addKopecks(80);
    CHECK(s.money == 3 && s.kopecks == 80);
    CHECK(moneyText(s) == "3,80");
    s.addKopecks(40);
    CHECK(s.money == 4 && s.kopecks == 20);
    s.addKopecks(80);
    CHECK(moneyText(s) == "5");
}

// standing just east of the hatch, ready to bump it
static void atHatch(Market& m) { const Coord h = m.hm->getHatch(); m.w.player.coord = Coord{ h.x + 1, h.y }; }

TEST(hatch_takes_one_bottle_per_bump_and_pays_on_the_receipt)
{
    Market m;
    atHatch(m);
    m.w.player.inventory.pullot = 6;
    m.w.player.stats.money = 10;
    for (uint32_t i = 1; i <= 6; ++i) {
        m.w.player.move(DirectionNS::left, &m.w.level);
        CHECK(m.w.player.inventory.pullot == 6 - i);
        CHECK(m.w.player.stats.money == 10);       // nothing paid before the receipt
    }
    CHECK(m.hm->hatchBottles == 6);
    m.w.player.move(DirectionNS::left, &m.w.level);  // one more bump: the receipt
    CHECK(m.w.player.stats.money == 11 && m.w.player.stats.kopecks == 20);
    CHECK(m.hm->hatchBottles == 0);
    CHECK(m.w.printer.saw("kuitin"));
}

TEST(handing_in_a_bottle_calms_a_guard_who_sees_it)
{
    Market m;
    atHatch(m);
    m.g1->coord = Coord{ m.w.player.coord.x + 2, m.w.player.coord.y };
    m.g1->provoke(&m.w.player);
    m.w.player.inventory.pullot = 1;
    m.w.player.move(DirectionNS::left, &m.w.level);
    CHECK(!m.g1->chasing);
}

TEST(loitering_at_the_hatch_without_bottles_angers_the_guards)
{
    Market m;
    atHatch(m);
    m.g1->coord = Coord{ m.w.player.coord.x + 2, m.w.player.coord.y };
    m.w.player.inventory.pullot = 0;
    m.w.player.move(DirectionNS::left, &m.w.level);
    CHECK(m.g1->chasing);
    CHECK(m.w.player.stats.money == 20 && m.w.player.stats.kopecks == 0);
}

TEST(four_bottles_draw_nobody)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 52, 50 }));
    w.player.inventory.pullot = 4;
    for (int i = 0; i < 300; ++i) w.level.npcFrame(500);
    CHECK(w.player.inventory.pullot == 4);
    CHECK(j->stolenBottles == 0);
}

TEST(a_drunk_comes_for_a_full_sack_takes_a_handful_and_picks_a_fight)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 56, 50 }));   // further than he can normally see
    w.player.inventory.pullot = 25;
    bool came = false;
    for (int i = 0; i < 400 && w.player.inventory.pullot == 25; ++i) {
        w.level.npcFrame(500);
        if (j->coord.distance(w.player.coord) <= 2) came = true;
    }
    CHECK(came);
    CHECK(w.player.inventory.pullot < 25);
    CHECK(j->stolenBottles == 25 - w.player.inventory.pullot);
    CHECK(j->stolenBottles >= 2);                       // a handful, not just one
    CHECK(j->brawlActions > 0);                         // and now he wants a fight
}

TEST(someone_feared_takes_just_one_bottle)
{
    World w;
    auto* s = w.spawn(new Skinhead(Coord{ 51, 50 }));
    w.player.inventory.pullot = 25;
    for (int i = 0; i < 400 && w.player.inventory.pullot == 25; ++i) w.level.npcFrame(500);
    CHECK(w.player.inventory.pullot == 24);
    CHECK(!s->hasLoot());
}

TEST(the_poorer_want_bottles_more)
{
    // same sack, standing right next to you: how often do they help themselves?
    auto thefts = [](Person* p) {
        World w;
        w.spawn(p);
        int n = 0;
        for (int i = 0; i < 600; ++i) {
            p->coord = Coord{ 51, 50 };
            p->fleeActions = p->brawlActions = 0;
            w.player.inventory.pullot = 10;
            w.level.npcFrame(500);
            if (w.player.inventory.pullot < 10) ++n;
        }
        return n;
    };
    const int drunk = thefts(new Juoppo(Coord{ 51, 50 }));
    const int fsb = thefts(new FSBAgent(Coord{ 51, 50 }));
    CHECK(drunk > 50);
    CHECK(drunk > fsb * 5);
}

TEST(a_brawler_forgets_you_out_of_sight)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 51, 50 }));
    j->brawlActions = 10;
    w.player.coord = Coord{ 90, 50 };
    w.player.inventory.pullot = 30;
    for (int i = 0; i < 5; ++i) w.level.npcFrame(500);   // he gets to act at least once
    CHECK(j->brawlActions == 0);
}

TEST(catching_a_bottle_thief_gets_everything_back)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 51, 50 }));
    j->stolenBottles = 3;
    j->stolenItems = { 0, 3 };   // a Baltika and a sausage
    j->stolenMoney = 5;
    const auto before = w.player.inventory;
    const uint32_t money = w.player.stats.money;
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.inventory.pullot == before.pullot + 3);
    CHECK(w.player.inventory.kalja == before.kalja + 1);
    CHECK(w.player.inventory.lenkki == before.lenkki + 1);
    CHECK(w.player.stats.money == money + 5);
    CHECK(!j->hasLoot());
}

// ---- fire, smoke and wind ----

static size_t smokePuffs(const Level& l) { size_t n = 0; for (const auto& g : l.gas) if (g.smoke) ++n; return n; }

TEST(drone_sometimes_strikes_a_building_near_the_player)
{
    World w(Coord{ 100, 100 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* nearShop = w.add(new Kauppa("NEAR", Kauppa::Kind::Produkty), 110, 110);
    w.add(new Kauppa("FAR", Kauppa::Kind::Produkty), 300, 20);
    w.level.nearStrikeCooldown = 1;
    w.level.drones.clear();
    w.level.skyTick();
    bool aimed = false;
    for (const auto& d : w.level.drones) if (d.target == nearShop) aimed = true;
    CHECK(aimed);
    CHECK(w.level.nearStrikeCooldown >= 250); // and then not again for minutes
}

TEST(drone_hit_sets_the_building_ablaze)
{
    World w(Coord{ 100, 100 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 110, 110);
    w.level.droneImpact(Coord{ 117, 112 }, k);
    const auto& fires = w.level.events->getFires();
    CHECK(fires.size() >= 6);
    bool onWall = false;
    for (const auto& f : fires) if (k->hitWall(f.coord)) onWall = true;
    CHECK(onWall);
    for (int i = 0; i < 30; ++i) w.level.events->update(); // calm events keep it burning
    CHECK(!w.level.events->getFires().empty());
}

TEST(fire_smokes_and_the_wind_carries_it)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    w.level.events->setAblaze(200, 100, 204, 102);
    w.level.windAngle = 0.0f;   // blowing east
    w.level.windSpeed = 0.7f;
    for (int i = 0; i < 25; ++i) { w.level.windAngle = 0.0f; w.level.skyTick(); }
    CHECK(smokePuffs(w.level) > 5);
    float sum = 0; size_t n = 0;
    for (const auto& g : w.level.gas) if (g.smoke) { sum += g.x; ++n; }
    CHECK(n && sum / n > 205.0f);   // drifted downwind of the fire
    CHECK(w.level.gasAt(Coord{ 0, 0 }) == 0);
}

TEST(wind_blowing_west_takes_smoke_west)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    w.level.events->setAblaze(200, 100, 204, 102);
    w.level.windSpeed = 0.7f;
    for (int i = 0; i < 25; ++i) { w.level.windAngle = 3.14159f; w.level.skyTick(); }
    float sum = 0; size_t n = 0;
    for (const auto& g : w.level.gas) if (g.smoke) { sum += g.x; ++n; }
    CHECK(n && sum / n < 199.0f);
}

TEST(wind_changes_direction_over_time)
{
    World w(Coord{ 20, 20 });
    const float start = w.level.windAngle;
    float lo = start, hi = start;
    for (int i = 0; i < 3000; ++i) {
        w.level.skyTick();
        lo = std::min(lo, w.level.windAngle); hi = std::max(hi, w.level.windAngle);
    }
    CHECK(hi - lo > 1.0f);
    CHECK(w.level.windSpeed >= 0.1f && w.level.windSpeed <= 0.8f);
}

TEST(flames_flicker)
{
    World w(Coord{ 50, 50 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    w.level.events->setAblaze(48, 48, 56, 52);
    w.level.updateCamera();
    w.mask.init();
    const uint32_t row = 50 - (uint32_t)w.level.cameraY;
    auto snapshot = [&]() { string s; for (const auto& c : w.printer.rowCells(&w.level, row)) s += c.first + c.second; return s; };
    const string a = snapshot();
    bool changed = false;
    for (int i = 0; i < 5 && !changed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        changed = snapshot() != a;
    }
    CHECK(changed);
}

TEST(breathing_smoke_makes_you_cough)
{
    World w(Coord{ 80, 80 });
    for (int i = 0; i < 300 && !w.printer.saw("Savu") && !w.printer.saw("savua"); ++i) {
        GasPuff g { 80.0f, 80.0f, 30u, false };
        g.smoke = true;
        w.level.gas.push_back(g);
        w.level.windSpeed = 0.1f;
        w.player.stats.health = 100;
        w.level.skyTick();
    }
    CHECK(w.printer.saw("Savu") || w.printer.saw("savua"));
}

// ---- character height and line of sight ----

static bool sees(World& w, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
    return w.level.raycast(Coord{ x0, y0 }, Coord{ x1, y1 });
}

static Veteraani* veteran(World& w, Coord at, bool oneLegged)
{
    auto* v = w.spawn(new Veteraani(at));
    v->oneLegged = oneLegged;
    return v;
}

TEST(standing_person_hides_everything_behind)
{
    World w;
    w.spawn(new Mummo({ 52, 50 }));
    CHECK(!sees(w, 50, 50, 53, 50));
    CHECK(!sees(w, 50, 50, 56, 50));
}

TEST(one_legged_veteran_hides_only_the_square_behind)
{
    World w;
    veteran(w, { 52, 50 }, true);
    CHECK(sees(w, 50, 50, 52, 50));  // the veteran himself
    CHECK(!sees(w, 50, 50, 53, 50)); // right behind him
    CHECK(sees(w, 50, 50, 54, 50));  // further back is in sight
    CHECK(sees(w, 50, 50, 58, 50));
}

TEST(standing_person_right_behind_a_veteran_is_seen_over_him)
{
    World w;
    veteran(w, { 52, 50 }, true);
    w.spawn(new Mummo({ 53, 50 }));
    CHECK(sees(w, 50, 50, 53, 50));
}

TEST(two_legged_veteran_blocks_like_anyone)
{
    World w;
    veteran(w, { 52, 50 }, false);
    CHECK(!sees(w, 50, 50, 54, 50));
}

TEST(rat_does_not_block_the_view)
{
    World w;
    w.spawn(new Rotta({ 52, 50 }));
    CHECK(sees(w, 50, 50, 53, 50));
    CHECK(sees(w, 50, 50, 56, 50));
}

TEST(crawling_player_cannot_see_past_a_veteran)
{
    World w;
    veteran(w, { 52, 50 }, true);
    w.player.crawling = true;
    CHECK(!sees(w, 50, 50, 54, 50));
    // but a standing man behind him towers over his head
    w.spawn(new Mummo({ 55, 50 }));
    CHECK(sees(w, 50, 50, 55, 50));
}

TEST(crawling_player_still_sees_over_a_rat)
{
    World w;
    w.spawn(new Rotta({ 52, 50 }));
    w.player.crawling = true;
    CHECK(sees(w, 50, 50, 55, 50));
}

TEST(crawling_player_hides_behind_a_veteran)
{
    World w({ 53, 50 });
    veteran(w, { 52, 50 }, true);
    auto* cop = w.spawn(new Cop({ 50, 50 }));
    CHECK(cop->canSeeTarget(w.player.coord));
    w.player.crawling = true;
    CHECK(!cop->canSeeTarget(w.player.coord));
}

TEST(only_heavy_drinking_makes_you_crawl)
{
    World w;
    w.player.stats.promilles = 10;
    for (int i = 0; i < 500; ++i) w.player.updateCrawling();
    CHECK(!w.player.crawling);

    w.player.stats.promilles = 30;
    bool crawled = false;
    for (int i = 0; i < 500 && !crawled; ++i) { w.player.updateCrawling(); crawled = w.player.crawling; }
    CHECK(crawled);
    CHECK(w.printer.saw("Konttaat"));
    CHECK(w.player.height() == HEIGHT_LOW);

    // sobering up gets you back on your feet
    w.player.stats.promilles = 10;
    w.player.updateCrawling();
    CHECK(!w.player.crawling);
    CHECK(w.player.height() == HEIGHT_STANDING);
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

// the title bar names where the player stands, not the stage's old landmark
TEST(location_follows_the_player)
{
    World w;
    w.level.roads = { { true, 112, "TVERSKAJA UL." }, { false, 135, "SADOVOJE KOLTSO" } };
    w.add(new Kremlin(), 250, 20);  // walls at x 250..295, y 20..40
    CHECK(w.level.locationAt(Coord{ 272, 30 }) == "KREML");
    CHECK(w.level.locationAt(Coord{ 250, 30 }) == "KREMLIN MUURI");  // on the wall
    CHECK(w.level.locationAt(Coord{ 272, 41 }) == "KREMLIN MUURI");  // just outside it
    CHECK(w.level.locationAt(Coord{ 60, 113 }) == "TVERSKAJA UL.");
    CHECK(w.level.locationAt(Coord{ 136, 113 }) == "TVERSKAJA UL. / SADOVOJE KOLTSO");
    CHECK(w.level.locationAt(Coord{ 60, 120 }) == "TVERSKAJA UL.");   // a few steps off the street
    CHECK(w.level.locationAt(Coord{ 60, 170 }).empty());

    // nowhere named: the title bar falls back to the level's name
    w.printer.levelName = "Gorkin Puisto";
    w.player.coord = Coord{ 60, 170 };
    CHECK(w.printer.locationName(&w.level) == "Gorkin Puisto");
    w.player.coord = Coord{ 251, 30 };
    CHECK(w.printer.locationName(&w.level) == "KREMLIN MUURI");

    w.level.isWarLevel = true;
    CHECK(w.level.locationAt(Coord{ 60, 113 }).empty());
}

// ---- driving ----

// a car standing next to the player, its driver gone and the engine idling
static Car* parkedCar(World& w, Coord at = Coord{ 60, 50 }, bool police = false)
{
    Car* car = new Car(at, true, 1, police ? Car::Kind::Police : Car::Kind::Random);
    w.level.addVehicle(car);
    car->script = Car::Script::Parked;
    car->driverInside = false;
    return car;
}

static void frames(World& w, int n, DriveControls c)
{
    for (int i = 0; i < n; ++i) {
        if (w.player.driving) w.player.driving->controls = c;
        w.level.vehicleFrame(50);
        updateDriverGaze(&w.player, 50);
    }
}

TEST(driving_cars_draw_72_directions_with_the_front_marked)
{
    World w;
    Car* car = parkedCar(w);
    car->makeFree();
    std::vector<string> looks;
    for (int h = 0; h < Car::HEADINGS; ++h) {
        car->heading = h;
        car->headingF = h * 2.0f * (float)M_PI / Car::HEADINGS;
        car->drive(0);
        const auto body = car->cells();
        CHECK(body.size() >= 2);
        string look;
        int glass = 0;
        float cx = 0, cy = 0, gx = 0, gy = 0;
        for (size_t i = 0; i < body.size(); ++i) {
            look += std::to_string((int)body[i].x - (int)body[0].x) + "," + std::to_string((int)body[i].y) + car->glyphAt(i) + car->colorAt(i) + ";";
            cx += body[i].x; cy += body[i].y;
            if (string(car->colorAt(i)) == "1;96") { ++glass; gx = body[i].x; gy = body[i].y; }
        }
        cx /= body.size(); cy /= body.size();
        // one windscreen, and it is on the side the car is heading
        CHECK(glass == 1);
        const float a = h * 2.0f * (float)M_PI / Car::HEADINGS;
        CHECK((gx - cx) * std::cos(a) + (gy - cy) * 2.0f * std::sin(a) > -0.01f);
        looks.push_back(look);
    }
    std::sort(looks.begin(), looks.end());
    const size_t distinct = std::unique(looks.begin(), looks.end()) - looks.begin();
    CHECK(distinct >= 24);   // the outline changes all the way round, even standing still
}

TEST(driving_police_car_left_running_can_be_taken)
{
    World w(Coord{ 60, 49 });
    Car* car = parkedCar(w, Coord{ 60, 50 }, true);
    CHECK(enterNearbyCar(&w.player, &w.level));
    CHECK(w.player.driving == car);
    CHECK(car->freeMode && car->playerAtWheel && car->engineOn);
    CHECK(w.printer.saw("Politsijan auto"));
}

TEST(driving_dead_driver_is_dragged_out)
{
    World w(Coord{ 60, 49 });
    Car* car = parkedCar(w);
    car->driverInside = true;
    car->driverDead = true;
    CHECK(enterNearbyCar(&w.player, &w.level));
    CHECK(w.printer.saw("revit kuskin ruumiin"));
    CHECK(w.player.driving == car);
}

TEST(driving_empty_car_without_keys_stays)
{
    World w(Coord{ 60, 49 });
    Car* car = parkedCar(w);
    car->keysIn = false;
    car->engineOn = false;
    CHECK(!enterNearbyCar(&w.player, &w.level));
    CHECK(w.player.driving == nullptr);
    CHECK(w.printer.saw("avaimia"));
}

TEST(driving_moving_lane_car_cannot_be_entered)
{
    World w(Coord{ 60, 49 });
    Car* car = new Car(Coord{ 60, 50 }, true, 1);
    w.level.addVehicle(car);
    CHECK(!enterNearbyCar(&w.player, &w.level));
    CHECK(w.printer.saw("ajaa ohi"));
}

TEST(driving_clutch_down_revs_freely_and_letting_go_moves_the_car)
{
    World w(Coord{ 60, 49 });
    parkedCar(w);
    CHECK(enterNearbyCar(&w.player, &w.level));
    Car* car = w.player.driving;
    DriveControls c; c.shift = 1; c.clutch = true;
    frames(w, 1, c);
    CHECK(car->gear == 1);
    DriveControls rev; rev.clutch = true; rev.throttle = 2;
    frames(w, 12, rev);
    CHECK(car->rpm > 5000);
    CHECK(std::fabs(car->velocity) < 0.1f);
    const float x0 = car->px;
    DriveControls go; go.throttle = 2;
    frames(w, 20, go);
    CHECK(car->engineOn);
    CHECK(car->velocity > 3.0f);
    CHECK(car->px > x0 + 1.0f);
    CHECK(w.player.coord == car->driverCell());
}

TEST(driving_every_gear_change_crunches)
{
    World w(Coord{ 60, 49 });
    parkedCar(w);
    CHECK(enterNearbyCar(&w.player, &w.level));
    Car* car = w.player.driving;
    for (int k = 0; k < 3; ++k) {
        DriveControls c; c.shift = 1; c.clutch = true;
        frames(w, 1, c);
        CHECK(car->grindMs > 0);
        frames(w, 10, DriveControls{});
    }
    CHECK(w.printer.saw("vaihde"));
    // S + Space goes down a gear
    DriveControls down; down.shift = -1; down.clutch = true;
    frames(w, 1, down);
    CHECK(car->gear == 2);
}

TEST(driving_third_gear_from_standstill_stalls)
{
    World w(Coord{ 60, 49 });
    parkedCar(w);
    CHECK(enterNearbyCar(&w.player, &w.level));
    Car* car = w.player.driving;
    car->gear = 3;
    car->clutchBite = 0.0f;
    frames(w, 30, DriveControls{});
    CHECK(!car->engineOn);
    CHECK(w.printer.saw("sammuu"));
}

TEST(driving_into_a_wall_hurts)
{
    World w(Coord{ 60, 49 });
    w.add(new Alko(), 72, 47);
    parkedCar(w);
    CHECK(enterNearbyCar(&w.player, &w.level));
    Car* car = w.player.driving;
    car->gear = 2;
    car->velocity = 10.0f;
    car->rpm = 10.0f * 2.0f * 250.0f;
    const uint32_t hp = w.player.stats.health = 20;
    DriveControls go; go.throttle = 1;
    frames(w, 30, go);
    CHECK(w.player.stats.health < hp);
    CHECK(w.printer.saw("RYS"));
    CHECK(car->px < 72.0f);
}

TEST(driving_get_out_leaves_the_car_running)
{
    World w(Coord{ 60, 49 });
    Car* car = parkedCar(w);
    CHECK(enterNearbyCar(&w.player, &w.level));
    CHECK(leaveCar(&w.player, &w.level));
    CHECK(w.player.driving == nullptr);
    CHECK(!car->playerAtWheel);
    CHECK(!car->hitVehicle(w.player.coord));
    CHECK(car->engineOn);
}

TEST(driving_drunk_eyes_wander_more_than_sober_ones)
{
    auto wander = [](uint32_t promilles) {
        World w(Coord{ 60, 49 });
        parkedCar(w);
        enterNearbyCar(&w.player, &w.level);
        w.player.stats.promilles = promilles;
        float worst = 0, sum = 0;
        for (int i = 0; i < 2000; ++i) {
            updateDriverGaze(&w.player, 50);
            float off = std::fabs(w.player.gaze - w.player.driving->heading * 2.0f * (float)M_PI / Car::HEADINGS);
            if (off > M_PI) off = 2 * M_PI - off;
            worst = std::max(worst, off);
            sum += off;
        }
        return std::make_pair(worst, sum / 2000);
    };
    const auto sober = wander(2), drunk = wander(30);
    CHECK(sober.second < 0.25f);           // mostly on the road
    CHECK(drunk.second > sober.second);
    CHECK(drunk.first > 0.8f);             // and now and then far off it
}

TEST(driving_vision_follows_the_gaze)
{
    World w(Coord{ 60, 49 });
    parkedCar(w);
    w.mask.enable();
    CHECK(enterNearbyCar(&w.player, &w.level));
    w.player.stats.promilles = 0;
    w.player.gaze = 0;        // straight ahead, east
    w.player.gazeOffset = 0;
    w.player.camLeadX = w.player.camLeadY = 0;
    w.level.updateCamera();
    w.mask.init();
    const Coord me = w.player.coord;
    CHECK(w.mask.visible(Coord{ me.x + 18, me.y }));     // far down the road ahead
    CHECK(!w.mask.visible(Coord{ me.x - 18, me.y }));    // nothing behind
    CHECK(w.mask.visible(Coord{ me.x - 1, me.y }));      // right around the car
}

TEST(driving_walk_into_a_stopped_car_to_take_it)
{
    World w(Coord{ 60, 49 });
    Car* car = parkedCar(w);
    CHECK(car->standing());
    w.player.move(DirectionNS::down, &w.level);
    CHECK(w.player.driving == car);
}

TEST(driving_cannot_jump_into_a_moving_car)
{
    World w(Coord{ 60, 49 });
    Car* car = new Car(Coord{ 60, 50 }, true, 1);
    w.level.addVehicle(car);
    CHECK(!car->standing());
    w.player.move(DirectionNS::down, &w.level);
    CHECK(w.player.driving == nullptr);
    CHECK(w.printer.saw("ajaa ohi"));
    // the same car stuck behind something for a while is fair game
    car->msSinceMoved = 2000;
    CHECK(car->standing());
}

TEST(driving_shot_drivers_mostly_run_some_die_some_ram)
{
    World w(Coord{ 60, 40 });
    int dead = 0, ran = 0, ram = 0;
    const int N = 600;
    for (int i = 0; i < N; ++i) {
        Car* car = new Car(Coord{ 60, 50 }, true, 1);
        w.level.addVehicle(car);
        car->script = Car::Script::Parked;
        car->takeShot(&w.player, Coord{ 60, 50 });
        if (car->driverDead) ++dead;
        else if (!car->driverInside) ++ran;
        else if (car->ai == Car::Ai::Ram) ++ram;
        w.level.vehicles.pop_back();
        delete car;
        // the drivers who ran: off the map again
        for (auto p : w.level.persons) if (p->type == kansalainen) p->stats.health = 0;
        w.level.cleanDead();
    }
    CHECK(ran > N * 0.68 && ran < N * 0.82);   // about 75 %
    CHECK(dead > N * 0.07 && dead < N * 0.19);
    CHECK(ram > N * 0.06 && ram < N * 0.18);
    CHECK(dead + ran + ram == N);
}

TEST(driving_shot_driver_who_runs_leaves_the_car_running)
{
    World w(Coord{ 60, 40 });
    Car* car = parkedCar(w);
    car->driverInside = true;
    std::srand(1);
    for (int i = 0; i < 50 && car->driverInside; ++i) {
        car->shotAt = false;
        car->driverDead = false;
        car->ai = Car::Ai::None;
        car->takeShot(&w.player, car->coord);
    }
    CHECK(!car->driverInside);
    CHECK(car->engineOn && car->standing());
    CHECK(w.count(kansalainen) == 1);
    CHECK(w.printer.saw("juoksee karkuun"));
}

TEST(driving_shot_driver_who_rams_comes_at_the_shooter)
{
    World w(Coord{ 90, 50 });
    Car* car = parkedCar(w);
    car->driverInside = true;
    car->makeFree();
    car->ai = Car::Ai::Ram;
    car->aiTarget = &w.player;
    car->aiFrom = w.player.coord;
    car->shotAt = true;
    w.player.stats.health = 30;
    // the player shrugs off hits for a moment after the last one (an earlier test)
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    const float d0 = std::fabs(car->px - 90.5f);
    float closest = d0;
    for (int i = 0; i < 120; ++i) {
        w.level.vehicleFrame(50);
        closest = std::min(closest, std::fabs(car->px - 90.5f) + std::fabs(car->py / 2.0f - 50.5f));
    }
    CHECK(closest < 3.0f);
    CHECK(w.player.stats.health < 30);
    CHECK(w.printer.saw("p\xC3\xA4\xC3\xA4llesi"));
    CHECK(contains(w.player.lastHurtBy, "ajoi ylitsesi"));   // named on the death notice
    CHECK(car->ai != Car::Ai::Ram);   // and then away
}

TEST(death_notice_knows_who_shot_you)
{
    World w(Coord{ 60, 46 });
    Person* cop = w.spawn(new Cop(Coord{ 60, 40 }));
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    w.level.fireBullet(cop, (float)M_PI / 2, 10, 3, 100);
    CHECK(w.player.lastHurtBy == "Politsija (ampui sinua)");
    // let the moment of grace after the hit pass, so the next test's hit counts
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
}

TEST(driving_a_bullet_can_kill_the_driver)
{
    World w(Coord{ 60, 46 });
    bool killed = false;
    for (int i = 0; i < 80 && !killed; ++i) {
        Car* car = new Car(Coord{ 60, 50 }, true, 1);
        w.level.addVehicle(car);
        car->script = Car::Script::Parked;
        w.level.fireBullet(&w.player, (float)M_PI / 2, 10, 5, 100);
        killed = car->driverDead;
        w.level.vehicles.pop_back();
        delete car;
    }
    CHECK(killed);
}

int main(int argc, char** argv)
{
    const string only = argc > 1 ? argv[1] : "";
    int ran = 0;
    for (const auto& t : tests()) {
        if (!only.empty() && !contains(t.name, only)) continue;
        const int before = failures;
        std::srand(12345); // each test sees the same dice, whatever ran before it
        t.fn();
        ++ran;
        std::cout << (failures == before ? "ok   " : "FAIL ") << t.name << std::endl;
    }
    std::cout << ran << " tests, " << checks << " checks, " << failures << " failures" << std::endl;
    return failures ? 1 : 0;
}

// ---- traffic ----

static bool carShouted(const World& w, const Car* car)
{
    for (auto a : w.level.animations) if (a->isBubble && a->owner == car) return true;
    return false;
}

TEST(car_passing_close_honks_and_shouts)
{
    World w(Coord{ 50, 50 });
    auto* car = new Car(Coord{ 58, 49 }, true, -1);
    w.level.addVehicle(car);
    int shouts = 0;
    for (int i = 0; i < 30; ++i) {
        car->frame(1000);
        // count each new bubble, then clear it so a repeat would show
        for (auto it = w.level.animations.begin(); it != w.level.animations.end();) {
            if ((*it)->isBubble && (*it)->owner == car) { ++shouts; delete *it; it = w.level.animations.erase(it); }
            else ++it;
        }
    }
    CHECK(shouts == 1); // once per pass, not every step
    CHECK(car->coord.x < 45); // it did drive past
    CHECK(contains(w.level.audio.formatSoundLine(w.player.coord, 400), "T\xC3\x96\xC3\x96"));
}

TEST(car_two_lanes_away_stays_quiet)
{
    World w(Coord{ 50, 50 });
    auto* car = new Car(Coord{ 58, 48 }, true, -1);
    w.level.addVehicle(car);
    for (int i = 0; i < 30; ++i) car->frame(1000);
    CHECK(!carShouted(w, car));
}

// ---- needles and HIV ----

TEST(hiv_needle_infects_half_the_rolls)
{
    int infected = 0;
    for (uint32_t roll = 0; roll < 100; ++roll) {
        Disease::Hiv h;
        if (h.needleStick(10, roll)) ++infected;
        CHECK(h.sticks == 1);
    }
    CHECK(infected == 50);

    // a second stick does not reinfect or move the clock
    Disease::Hiv h;
    CHECK(h.needleStick(10, 0));
    CHECK(!h.needleStick(500, 0));
    CHECK(h.infectedTurn == 10 && h.sticks == 2);

    // the real dice land near one in two
    World w;
    int sick = 0;
    for (int i = 0; i < 2000; ++i) {
        w.player.hiv = Disease::Hiv();
        w.player.stepOnNeedle();
        if (w.player.hiv.infected) ++sick;
    }
    CHECK(sick > 900 && sick < 1100);
}

TEST(hiv_incubates_before_symptoms)
{
    Disease::Hiv h;
    CHECK(h.stage(5000) == 0); // healthy stays healthy
    h.needleStick(100, 0);
    CHECK(h.stage(100) == 0);
    CHECK(h.stage(100 + Disease::INCUBATION_TURNS - 1) == 0);
    CHECK(h.stage(100 + Disease::INCUBATION_TURNS) == 1);
    CHECK(h.stage(100 + Disease::INCUBATION_TURNS + Disease::STAGE_TURNS) == 2);
    CHECK(h.stage(h.stageStart(Disease::AIDS_STAGE)) == Disease::AIDS_STAGE);
    CHECK(h.stage(h.stageStart(Disease::AIDS_STAGE) + 100000) == Disease::AIDS_STAGE);
    CHECK(h.wasted(h.stageStart(Disease::AIDS_STAGE) - 1) == 0);
    CHECK(h.wasted(h.stageStart(Disease::AIDS_STAGE) + Disease::WASTING_TURNS * 2) == 2);
}

TEST(hiv_symptoms_worsen_stage_by_stage)
{
    CHECK(Disease::maxHealth(0) == 0); // no limit when healthy
    CHECK(Disease::visionDistance(12, 0) == 12);
    CHECK(Disease::hearingPercent(0) == 100);
    for (uint32_t s = 1; s <= Disease::AIDS_STAGE; ++s) {
        if (s > 1) CHECK(Disease::maxHealth(s) < Disease::maxHealth(s - 1));
        CHECK(Disease::maxHealth(s) < PlayerNS::INITIAL_HP);
        CHECK(Disease::visionDistance(12, s) < Disease::visionDistance(12, s - 1));
        CHECK(Disease::hearingPercent(s) < Disease::hearingPercent(s - 1));
    }
    CHECK(string(Disease::hudLabel(0)).empty());
    CHECK(string(Disease::hudLabel(2)) == "HIV+");
    CHECK(string(Disease::hudLabel(5)) == "AIDS");
}

TEST(hiv_stepping_on_a_needle_stings_but_tells_nothing)
{
    World w;
    w.level.addItem(new Neula(Coord{ 51, 50 }));
    const uint32_t hp = w.player.stats.health;
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.hiv.sticks == 1);
    CHECK(w.player.stats.health == hp - 1);
    bool left = false;
    for (auto i : w.level.items) if (i->type == ENeula) left = true;
    CHECK(!left); // pulled out and thrown away
    CHECK(!w.printer.saw("HIV"));
}

TEST(hiv_bush_can_hide_a_needle)
{
    World w;
    Pensas bush(Coord{ 51, 50 });
    bush.needle = true;
    CHECK(contains(bush.getMsg(), "ruisku"));
    bush.interact(&w.player);
    CHECK(w.player.hiv.sticks == 1);
    CHECK(!bush.needle);
    CHECK(!contains(bush.getMsg(), "ruisku"));
}

TEST(hiv_progresses_in_the_player_and_kills)
{
    World w;
    w.player.hiv.needleStick(0, 0);
    w.player.stats.health = 10;
    while (w.player.stats.turn < Disease::FLU_TURNS) w.player.updateTurn();
    CHECK(w.printer.saw("kuumeinen"));
    CHECK(w.player.diseaseStage() == 0);
    CHECK(w.player.stats.health == 10);

    while (w.player.stats.turn < Disease::INCUBATION_TURNS) w.player.updateTurn();
    CHECK(w.player.diseaseStage() == 1);
    CHECK(w.printer.saw("HIV"));
    CHECK(w.player.stats.health == Disease::maxHealth(1));
    CHECK(w.player.hearingPercent() < 100);

    // healing cannot lift VOIMA over what the body still holds up
    w.player.stats.health = 10;
    w.player.updateTurn();
    CHECK(w.player.stats.health == Disease::maxHealth(1));

    while (w.player.diseaseStage() < Disease::AIDS_STAGE) w.player.updateTurn();
    CHECK(w.printer.saw("AIDS"));
    CHECK(w.player.stats.health == Disease::maxHealth(Disease::AIDS_STAGE));
    CHECK(!w.player.diedOfAids());

    for (uint32_t i = 0; i < Disease::WASTING_TURNS * 4 && w.player.stats.health > 0; ++i) w.player.updateTurn();
    CHECK(w.player.stats.health == 0);
    CHECK(w.player.diedOfAids());
}

TEST(hiv_narrows_sight_and_hearing)
{
    World w;
    w.mask.enable();
    w.level.updateCamera();
    w.mask.init();
    const Coord far { 60, 50 }; // 10 cells away in the open
    CHECK(w.mask.visible(far));
    w.level.audio.addSound(Coord{ 64, 50 }, SoundType::Engine, "lähellä", "BRUUM", "kaukana", 20);
    CHECK(!w.level.audio.getHeardSounds(w.player.coord, w.player.hearingPercent()).empty());

    w.player.hiv.needleStick(0, 0);
    w.player.stats.turn = w.player.hiv.stageStart(4);
    CHECK(w.player.diseaseStage() == 4);
    w.mask.init();
    CHECK(!w.mask.visible(far));
    CHECK(w.mask.visible(Coord{ 53, 50 }));
    CHECK(w.level.audio.getHeardSounds(w.player.coord, w.player.hearingPercent()).empty());
}


TEST(hiv_needles_lie_where_the_dealers_stand)
{
    World w(Coord{ 20, 20 });
    auto* dealer = w.spawn(new Diileri(Coord{ 80, 60 }));
    w.add(new Metro("TEST"), 150, 100);
    w.level.scatterNeedles();
    int nearDealer = 0, nearMetro = 0;
    for (auto i : w.level.items) {
        if (i->type != ENeula) continue;
        if (i->coord.distance(dealer->coord) <= 3) ++nearDealer;
        if (i->coord.x >= 145 && i->coord.x <= 170 && i->coord.y >= 100 && i->coord.y <= 115) ++nearMetro;
        CHECK(i->coord.distance(w.player.coord) > 5); // nothing to do with the player
    }
    CHECK(nearDealer >= 1);
    CHECK(nearMetro >= 1);
}

// ---- anti-aircraft guns ----

// a gun at (x, y) with `crew` men below it
static AAGun* placeTestGun(World& w, uint32_t x, uint32_t y, int crew)
{
    auto* gun = new AAGun(Coord{ x, y });
    w.level.addVehicle(gun);
    for (int k = 0; k < crew; ++k) w.spawn(new ITMies(Coord{ x + 2u * k, y + 1 }, gun));
    return gun;
}

TEST(aa_gun_fires_a_hundred_shells_then_clicks)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* gun = placeTestGun(w, 200, 150, 0);
    CHECK(gun->ammo == 100);
    string msg;
    CHECK(gun->approach(&w.player, msg));
    gun->angle = 0.0f;
    int fired = 0;
    for (int i = 0; i < 100; ++i) if (gun->fire(msg)) ++fired;
    CHECK(fired == 100);
    CHECK(gun->ammo == 0);
    CHECK(!gun->fire(msg));             // empty: nothing more comes out
    CHECK(contains(msg, "loppuivat"));
    CHECK(gun->ammo == 0);
}

TEST(aa_gun_crew_takes_a_bottle_of_vodka_each)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 1);
    w.player.inventory.vodka = 0;
    string msg;
    CHECK(!gun->approach(&w.player, msg));  // no bottle: shoved away
    CHECK(w.player.manning == nullptr);
    w.player.inventory.vodka = 2;
    CHECK(gun->approach(&w.player, msg));
    CHECK(w.player.inventory.vodka == 1);
    CHECK(w.player.manning == gun);
    CHECK(gun->gunner == &w.player);
    CHECK(gun->crew().size() == 1 && gun->awakeCrew().empty());   // drank it and passed out
    gun->leave();
    CHECK(w.player.manning == nullptr && gun->gunner == nullptr);
}

TEST(aa_gun_two_crew_need_two_bottles)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 2);
    w.player.inventory.vodka = 1;
    string msg;
    CHECK(!gun->approach(&w.player, msg));
    CHECK(w.player.inventory.vodka == 1);   // the bottle is not wasted
}

TEST(aa_gun_sleeping_crew_lets_the_drunk_take_it)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 1);
    gun->crew().front()->doze(50);
    w.player.inventory.vodka = 3;
    string msg;
    CHECK(gun->approach(&w.player, msg));
    CHECK(w.player.inventory.vodka == 3);   // no bribe needed
    CHECK(contains(msg, "kuorsaa"));
}

TEST(aa_gun_can_be_captured_once_the_crew_is_dead)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 1);
    gun->crew().front()->stats.health = 0;
    w.player.inventory.vodka = 0;
    string msg;
    CHECK(gun->approach(&w.player, msg));
    CHECK(contains(msg, "Kaappaat"));
}

TEST(aa_gun_wrecked_gun_cannot_be_used)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 0);
    gun->wrecked = true;
    string msg;
    CHECK(!gun->approach(&w.player, msg));
    CHECK(!gun->fire(msg));
    CHECK(gun->ammo == 100);
}

TEST(aa_shell_flies_over_close_people_and_hits_the_car)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 0);
    w.spawn(new Juoppo(Coord{ 203, 150 }));       // two cells out: under the arming distance
    auto* car = new Car(Coord{ 215, 150 }, true, -1);
    w.level.addVehicle(car);
    const ShellHit hit = traceShell(w.level, gun->centre(), 0.0f, gun);
    CHECK(car->hitVehicle(hit.at));
    CHECK(hit.what == car->name());
}

TEST(aa_shell_stops_at_a_wall_and_downs_drones)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 0);
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 220, 147);
    ShellHit hit = traceShell(w.level, gun->centre(), 0.0f, gun);
    CHECK(hit.at.x == 220 && k->hitWall(hit.at));
    Drone d { 210.0f, 150.0f, 0.0f, 0.0f };
    w.level.drones.push_back(d);
    hit = traceShell(w.level, gun->centre(), 0.0f, gun);
    CHECK(hit.drone);
    aaExplosion(w.level, hit.at, nullptr);
    CHECK(w.level.drones.empty());
}

TEST(aa_explosion_is_devastating)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 150, 100);
    auto* car = new Car(Coord{ 157, 106 }, true, -1);   // two rows below the burst
    w.level.addVehicle(car);
    auto* near = w.spawn(new Juoppo(Coord{ 158, 105 }));
    auto* edge = w.spawn(new Juoppo(Coord{ 161, 104 }));
    near->stats.health = 20;
    edge->stats.health = 20;
    const string what = aaExplosion(w.level, Coord{ 158, 104 }, &w.player);  // on the shop's bottom wall
    CHECK(near->stats.health <= 8);                 // one cell away: torn up
    CHECK(edge->stats.health < 20);                 // three cells: still hurt
    CHECK(car->wrecked);                            // the car blew up
    CHECK(contains(what, car->name()));
    bool breach = false;
    for (const auto& b : k->breaches) breach = breach || (b.y == k->wallBot && b.x >= 156 && b.x <= 160);
    CHECK(breach);                                  // the wall is blown open
    CHECK(w.level.events->getFires().size() >= 3);  // and it all burns
    bool smoke = false;
    for (const auto& g : w.level.gas) smoke = smoke || g.smoke;
    CHECK(smoke);
    // a wrecked car goes nowhere
    const Coord was = car->coord;
    for (int i = 0; i < 20; ++i) car->frame(1000);
    CHECK(car->coord == was);
}

TEST(aa_explosion_cannot_breach_the_kremlin)
{
    World w(Coord{ 20, 20 });
    auto* kr = w.add(new Kremlin(), 250, 20);
    const string what = aaExplosion(w.level, Coord{ 260, 41 }, nullptr);
    CHECK(kr->breaches.empty());
    CHECK(contains(what, "Kremlin"));
}

TEST(aa_guns_guard_the_kremlin)
{
    World w(Coord{ 20, 20 });
    w.add(new Kremlin(), 250, 20);
    populateAAGuns(w.level, 1);
    int guns = 0, crewed = 0;
    for (auto v : w.level.vehicles) {
        auto* g = dynamic_cast<AAGun*>(v);
        if (!g) continue;
        ++guns;
        if (!g->crew().empty()) ++crewed;
        const Coord c = g->centre();
        CHECK(c.x > 230 && c.x < 315 && c.y > 10 && c.y < 60);   // close to the walls
        CHECK(!w.level.hitBuilding(c));
    }
    CHECK(guns >= 3);
    CHECK(crewed == guns);
}

TEST(aa_crew_shoots_at_drones_and_dozes_off)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* gun = placeTestGun(w, 200, 150, 1);
    ITMies* m = gun->crew().front();
    for (int i = 0; i < 400 && w.level.drones.size() < 50; ++i) {
        if (w.level.drones.empty()) w.level.drones.push_back(Drone{ 210.0f, 145.0f, 0.0f, 0.0f });
        m->fireCooldown = 0;
        m->sleepActs = 0;
        m->npcAct();
        if (w.level.drones.empty()) break;
    }
    CHECK(w.level.drones.empty());      // sooner or later the drone comes down
    m->passedOut = true;
    CHECK(m->asleep() && m->typeToChar() == "z");
}

// ---- everyone shoots at drones ----

TEST(soldier_shoots_down_a_drone_in_range)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    w.spawn(new Sotilas(Coord{ 100, 100 }));
    w.level.drones.push_back(Drone{ 106.0f, 97.0f, 0.0f, 0.0f });
    for (int i = 0; i < 300 && !w.level.drones.empty(); ++i) antiDroneFire(w.level);
    CHECK(w.level.drones.empty());          // sooner or later it comes down
    CHECK(!w.level.events->getFires().empty());   // in flames
}

TEST(soldier_leaves_a_far_drone_alone)
{
    World w(Coord{ 20, 20 });
    auto* s = w.spawn(new Sotilas(Coord{ 100, 100 }));
    w.level.drones.push_back(Drone{ 140.0f, 100.0f, 0.0f, 0.0f });
    for (int i = 0; i < 100; ++i) antiDroneFire(w.level);
    CHECK(w.level.drones.size() == 1);
    CHECK(!w.said(s, "Drooni"));
}

TEST(stray_rounds_hit_bystanders_and_the_player)
{
    World w(Coord{ 20, 20 });
    w.spawn(new Sotilas(Coord{ 100, 100 }));
    auto* bystander = w.spawn(new Juoppo(Coord{ 104, 100 }));
    bystander->stats.health = 200;
    for (int i = 0; i < 300; ++i) {
        if (w.level.drones.empty()) w.level.drones.push_back(Drone{ 112.0f, 100.0f, 0.0f, 0.0f });
        antiDroneFire(w.level);
    }
    CHECK(bystander->stats.health < 200);   // in the line of fire
    // the player in the way gets it too
    bystander->stats.health = 0;
    w.level.cleanDead();
    w.player.coord = Coord{ 104, 100 };
    w.player.stats.health = 200;
    common::realtimeEnabled = false;   // no breather after an earlier test's hit on the player
    for (int i = 0; i < 300 && w.player.stats.health == 200; ++i) {
        if (w.level.drones.empty()) w.level.drones.push_back(Drone{ 112.0f, 100.0f, 0.0f, 0.0f });
        antiDroneFire(w.level);
    }
    CHECK(w.player.stats.health < 200);
    CHECK(w.printer.saw("Harhaluoti"));
}

TEST(aa_gun_crew_aims_at_the_drone_and_its_shells_burst)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* gun = placeTestGun(w, 200, 150, 1);
    ITMies* m = gun->crew().front();
    w.level.drones.push_back(Drone{ 230.0f, 130.0f, 0.0f, 0.0f });   // 35 cells out: in gun range, not rifle range
    m->fireCooldown = 0;
    m->npcAct();
    const Coord c = gun->centre();
    CHECK(std::fabs(gun->angle - std::atan2(130.0f - c.y, 230.0f - c.x)) < 0.01f);
    CHECK(!w.level.events->getFires().empty());   // a shell went off somewhere
    CHECK(gun->ammo == AAGun::AMMO);               // the crew's own crates, not the player's 100
    // it keeps at it until the drone is down
    for (int i = 0; i < 200 && !w.level.drones.empty(); ++i) { m->sleepActs = 0; m->npcAct(); }
    CHECK(w.level.drones.empty());
}

TEST(aa_gun_ignores_drones_while_the_drunk_has_it)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 0);
    string msg;
    gun->approach(&w.player, msg);
    w.level.drones.push_back(Drone{ 210.0f, 145.0f, 0.0f, 0.0f });
    for (int i = 0; i < 50; ++i) antiDroneFire(w.level);
    CHECK(w.level.drones.size() == 1);   // nobody else is shooting: the drunk has to
}

// ---- drones on petrol stations ----

TEST(drones_go_for_petrol_stations)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, false);
    w.add(new Bensa(), 100, 100);
    for (int i = 0; i < 3000; ++i) w.level.events->update();
    // count what the strikes were aimed at
    bool station = false;
    for (const auto& d : w.level.drones) station = station || (d.target && d.target->type == EGasStation);
    for (int i = 0; i < 3000 && !station; ++i) {
        w.level.events->update();
        for (const auto& d : w.level.drones) station = station || (d.target && d.target->type == EGasStation);
    }
    CHECK(station);
}

TEST(drone_on_a_petrol_station_blows_up_the_queue)
{
    World w(Coord{ 106, 118 });   // across the street: close enough to hear it
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* st = w.add(new Bensa(), 100, 100);
    w.add(new Pumppu(), 103, 107);
    w.add(new Pumppu(), 109, 107);
    std::vector<Car*> queue;
    for (int k = 0; k < 6; ++k) {
        auto* car = new Car(Coord{ 98u + 5u * k, 112 }, true, -1);
        w.level.addVehicle(car);
        queue.push_back(car);
    }
    auto* far = new Car(Coord{ 300, 180 }, true, -1);
    w.level.addVehicle(far);
    auto* customer = w.spawn(new Juoppo(Coord{ 106, 106 }));
    customer->stats.health = 20;
    w.level.droneImpact(Coord{ 106, 102 }, st);
    int wrecked = 0;
    for (auto c : queue) if (c->wrecked) ++wrecked;
    CHECK(wrecked >= 4);                 // the queue at the pumps went up
    CHECK(!far->wrecked);                // not the whole city
    CHECK(customer->stats.health == 0);  // nobody at the pumps lives through it
    CHECK(w.level.events->getFires().size() >= 25);   // far more fire than an ordinary strike
    CHECK(w.printer.saw("BENSA-ASEMA"));
}

// ---- the FSB collects drunks who walk down the road ----

TEST(fsb_risk_grows_with_the_road_walked)
{
    RoadRun run;
    for (uint32_t i = 0; i < RoadRun::SAFE; ++i) {
        run.step(true);
        CHECK(run.dispatchChance() == 0.0f);   // crossing a street, or a short stretch, is safe
    }
    float last = 0.0f, survive = 1.0f, surviveToWarning = 1.0f;
    for (uint32_t t = RoadRun::SAFE + 1; t <= RoadRun::CERTAIN; ++t) {
        run.step(true);
        const float p = run.dispatchChance();
        CHECK(p > last);
        last = p;
        survive *= 1.0f - p;
        if (t <= RoadRun::WARN) surviveToWarning *= 1.0f - p;
    }
    CHECK(surviveToWarning > 0.9f);  // the black car tails him before anything happens, nearly always
    CHECK(survive < 0.1f);           // by CERTAIN tiles the FSB has almost surely come
}

TEST(fsb_risk_resets_off_the_road)
{
    RoadRun run;
    for (int i = 0; i < 60; ++i) run.step(true);
    CHECK(run.dispatchChance() > 0.0f);
    uint32_t before = run.tiles;
    run.step(false);
    CHECK(run.tiles < before);       // every step on the pavement forgives a little
    for (int i = 0; i < 20; ++i) run.step(false);
    CHECK(run.tiles == 0);
    CHECK(run.dispatchChance() == 0.0f);

    // crossing streets all day long never adds up
    RoadRun crossing;
    for (int k = 0; k < 50; ++k) {
        for (int i = 0; i < 3; ++i) crossing.step(true);
        for (int i = 0; i < 4; ++i) crossing.step(false);
    }
    CHECK(crossing.dispatchChance() == 0.0f);
}

static bool hasFsbCar(const World& w)
{
    for (auto v : w.level.vehicles) if (auto c = dynamic_cast<const Car*>(v)) if (c->fsbCar) return true;
    return false;
}

TEST(fsb_black_car_tails_and_leaves_when_player_steps_off)
{
    World w(Coord{ 60, 112 });
    w.level.roads = { { true, 112, "TVERSKAJA UL." } };
    w.level.fsbTick();
    w.level.fsbPatrol.run.tiles = RoadRun::WARN - 1;
    w.player.coord.x += 1;
    w.level.fsbTick();
    CHECK(w.level.fsbPatrol.stage == FsbPatrol::Stage::Tailing);
    CHECK(hasFsbCar(w));
    CHECK(w.printer.saw("Musta Aurus hidastaa"));

    // off the road onto the pavement: the car gives up
    for (int i = 0; i < 6; ++i) {
        w.player.coord.y -= 1;
        w.level.fsbTick();
    }
    CHECK(w.level.fsbPatrol.stage == FsbPatrol::Stage::Idle);
    CHECK(w.level.fsbPatrol.run.tiles == 0);
    CHECK(w.printer.saw("kiihdytt"));
}

TEST(fsb_conscripts_player_who_runs_down_the_road)
{
    World w(Coord{ 40, 112 });
    w.level.roads = { { true, 112, "TVERSKAJA UL." } };
    w.level.fsbTick();
    // stroll east down Tverskaja until the FSB decides
    int walked = 0;
    while (w.level.fsbPatrol.stage != FsbPatrol::Stage::Dispatched && walked < 300) {
        w.player.coord.x = (w.player.coord.x + 1) % common::SIZEX;
        w.level.fsbTick();
        w.level.vehicleFrame(100);
        ++walked;
    }
    CHECK(w.level.fsbPatrol.stage == FsbPatrol::Stage::Dispatched);
    CHECK(walked > (int)RoadRun::WARN / 2);   // never at once
    CHECK(w.printer.saw("Musta Aurus hidastaa"));  // warned first
    CHECK(w.printer.saw("vapaaehtoinen"));

    // he stops; the cars pull up and the men jump out and grab him
    for (int frame = 0; frame < 2000 && !w.player.conscripted; ++frame) {
        w.level.vehicleFrame(50);
        w.level.npcFrame(50);
        if (frame % 8 == 0) w.level.fsbTick();
    }
    CHECK(w.printer.saw("Stoi! FSB!"));
    CHECK(w.player.conscripted);
}
