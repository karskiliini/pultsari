// Unit tests for game logic: shops, break-ins, drones, exhaust, vision.
// Build and run with `make test`.
#include <csignal>
#include "quips.hpp"
#include "aagun.hpp"
#include "patrol.hpp"
#include "animation.hpp"
#include "armart.hpp"
#include "building.hpp"
#include "bradley.hpp"
#include "tracer.hpp"
#include "input.hpp"
#include "common.hpp"
#include "events.hpp"
#include "fpv.hpp"
#include "gamestate.hpp"
#include "gaze.hpp"
#include "disease.hpp"
#include "item.hpp"
#include "level.hpp"
#include "mask.hpp"
#include "metro.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "printerweb.hpp"
#include "subtitles.hpp"
#include "handsintro.hpp"
#include "translate.hpp"
#include "trigger.hpp"
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

// both hands at once; Thing::None empties a hand
static void hold(Inventory& inv, Thing l, Thing r)
{
    inv.left = Held();
    inv.right = Held();
    if (l != Thing::None) inv.left = Inventory::heldOf(l);
    if (r != Thing::None) inv.right = Inventory::heldOf(r);
    if (l == Thing::Vodka) inv.left.fill = VODKA_SIPS;
    if (r == Thing::Vodka) inv.right.fill = VODKA_SIPS;
}

static bool itemAt(const Level& level, ItemType t)
{
    for (auto i : level.items) if (i->type == t) return true;
    return false;
}

// ---- Russian shops ----

TEST(shop_sells_food_for_money)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.stats.money = 20;
    w.player.inventory.right = Held();
    const uint32_t had = w.player.inventory.count(Thing::Lenkki);
    feedKeys("1");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 14);
    CHECK(w.player.inventory.count(Thing::Lenkki) == had + 1);
}

TEST(shop_kiosk_sells_vodka)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Kioski), 10, 10);
    w.player.stats.money = 30;
    w.player.inventory.left = Held();
    const uint32_t had = w.player.inventory.count(Thing::Vodka);
    feedKeys("4");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 5);
    CHECK(w.player.inventory.count(Thing::Vodka) == had + 1);
}

TEST(gun_shop_sells_a_pistol_with_rounds)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Oruzhie), 10, 10);
    w.player.stats.money = 50;
    w.player.inventory.right = Held();
    const uint32_t guns = w.player.inventory.count(Thing::Pistooli);
    feedKeys("2");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 15);
    CHECK(w.player.inventory.count(Thing::Pistooli) == guns + 1);
    CHECK(w.player.inventory.find(Thing::Pistooli) && w.player.inventory.find(Thing::Pistooli)->rounds == MAKAROV_ROUNDS);
}

TEST(gun_shop_sells_a_kalashnikov_as_the_fifth_item)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Oruzhie), 10, 10);
    w.player.stats.money = 100;
    const uint32_t ak = w.player.inventory.count(Thing::Rynkky);
    feedKeys("5");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 10);
    CHECK(w.player.inventory.count(Thing::Rynkky) == ak + 1);
    CHECK(w.player.inventory.back.what == Thing::Rynkky && w.player.inventory.back.rounds == AK_ROUNDS);
}

TEST(gun_shop_refuses_the_broke_and_food_shops_have_no_fifth_item)
{
    World w;
    auto* gun = w.add(new Kauppa("TEST", Kauppa::Kind::Oruzhie), 10, 10);
    w.player.stats.money = 20;
    const uint32_t ak = w.player.inventory.count(Thing::Rynkky);
    feedKeys("5");
    string msg;
    gun->interact(&w.player, msg);
    CHECK(w.player.stats.money == 20);
    CHECK(w.player.inventory.count(Thing::Rynkky) == ak);

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
    w.player.inventory.right = Held();
    const uint32_t had = w.player.inventory.count(Thing::Lenkki);
    feedKeys("1");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 1);
    CHECK(w.player.inventory.count(Thing::Lenkki) == had);
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

// ---- shoplifting ----

// the notice roll, forced for one shoplift: 0 = the seller is on his phone, 1 = he sees it all
static string shoplift(World& w, Kauppa* k, const string& keys, int notice)
{
    Kauppa::noticeOverride = notice;
    feedKeys(keys);
    string msg;
    k->interact(&w.player, msg);
    Kauppa::noticeOverride = -1;
    return msg;
}

static Myyja* sellerOf(World& w)
{
    for (auto p : w.level.persons) if (p->type == myyja && p->stats.health > 0) return dynamic_cast<Myyja*>(p);
    return nullptr;
}

TEST(shop_menu_offers_shoplifting)
{
    World w;
    for (auto kind : { Kauppa::Kind::Produkty, Kauppa::Kind::Kioski, Kauppa::Kind::Oruzhie }) {
        auto* k = w.add(new Kauppa("TEST", kind), 10, 10);
        string msg;
        k->getInteractMsg(&w.player, msg);
        const string tail = "  v+nro) varasta";
        CHECK(msg.size() > tail.size() && msg.compare(msg.size() - tail.size(), tail.size(), tail) == 0);
    }
}

TEST(shoplift_puts_item_in_hand_without_paying)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.stats.money = 20;
    w.player.inventory.right = Held();
    const uint32_t had = w.player.inventory.count(Thing::Lenkki);
    shoplift(w, k, "v1", 0);
    CHECK(w.player.stats.money == 20);
    CHECK(w.player.inventory.count(Thing::Lenkki) == had + 1);
    CHECK(w.player.inventory.right.what == Thing::Lenkki);
    CHECK(w.count(myyja) == 0);
    CHECK(!k->sellerHostile);
}

TEST(shoplift_cancelled_by_any_other_key)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.stats.money = 20;
    w.player.inventory.right = Held();
    shoplift(w, k, "vx", 1);
    CHECK(w.player.stats.money == 20);
    CHECK(w.player.inventory.right.empty());
    CHECK(w.count(myyja) == 0);
    CHECK(!k->sellerHostile);
}

TEST(shoplift_with_full_hands_takes_nothing)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    hold(w.player.inventory, Thing::Kalja, Thing::Kala);
    w.player.inventory.pocket = Inventory::heldOf(Thing::Lipas);
    const uint32_t had = w.player.inventory.count(Thing::Lenkki);
    const string msg = shoplift(w, k, "v1", 1);
    CHECK(msg == HINT_HANDS_FULL);
    CHECK(w.player.inventory.count(Thing::Lenkki) == had);
    CHECK(w.count(myyja) == 0);
    CHECK(!k->sellerHostile);
}

TEST(noticed_shoplift_spawns_armed_seller_who_chases)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Kioski), 10, 10);
    w.player.inventory.right = Held();
    const string msg = shoplift(w, k, "v2", 1);
    CHECK(w.player.inventory.right.what == Thing::Bansku); // the loot is yours, for now
    CHECK(k->sellerHostile);
    Myyja* m = sellerOf(w);
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(m->coord == k->getDoor());
    CHECK(m->alerted);
    CHECK(m->chaseTarget == &w.player);
    CHECK(w.said(m, "VARAS! Seis!") || w.said(m, "Minun kauppani! Minun!"));
    CHECK(contains(msg, "Myyj\xC3\xA4 ryntää perääsi "));
    CHECK(contains(msg, " kädessä!"));
    // he chases you down the street, whether he sees you or not
    const uint32_t start = m->coord.distance(w.player.coord);
    for (int i = 0; i < 10; ++i) m->npcAct();
    CHECK(m->coord.distance(w.player.coord) < start);
    CHECK(m->alerted);
}

TEST(seller_weapon_odds)
{
    int n[3] = { 0, 0, 0 };
    for (int i = 0; i < 10000; ++i) ++n[(int)rollSellerWeapon()];
    CHECK(n[(int)SellerWeapon::Nitoja] >= 7000 && n[(int)SellerWeapon::Nitoja] <= 8000);
    CHECK(n[(int)SellerWeapon::Veitsi] >= 1500 && n[(int)SellerWeapon::Veitsi] <= 2500);
    CHECK(n[(int)SellerWeapon::Pistooli] >= 200 && n[(int)SellerWeapon::Pistooli] <= 800);
}

TEST(hostile_shop_is_closed)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.inventory.right = Held();
    shoplift(w, k, "v1", 1);
    string msg;
    CHECK(!k->getEnterMsg(&w.player, msg));
    CHECK(msg == "Kauppa on kiinni. Myyj\xC3\xA4 jahtaa sinua jossain.");
    Myyja* m = sellerOf(w);
    CHECK(m != nullptr);
    if (!m) return;
    m->damage(m->stats.health);
    w.level.cleanDead();
    CHECK(!k->getEnterMsg(&w.player, msg));
    CHECK(msg == "Kauppa on hiljainen. Kukaan ei myy mit\xC3\xA4\xC3\xA4n.");
}

TEST(seller_strikes_with_his_weapon)
{
    const uint32_t dmg[] = { 1, 2 };
    const SellerWeapon arms[] = { SellerWeapon::Nitoja, SellerWeapon::Veitsi };
    for (int a = 0; a < 2; ++a) {
        World w;
        common::realtimeEnabled = false;   // no breather after an earlier hit on the player
        auto* m = w.spawn(new Myyja(Coord{ 51, 50 }, arms[a]));
        m->chaseTarget = &w.player;
        m->alerted = true;
        bool hit = false;
        for (int i = 0; i < 200 && !hit; ++i) {
            w.player.stats.health = 100;
            m->coord = Coord{ 51, 50 };
            m->npcAct();
            if (w.player.stats.health < 100) {
                hit = true;
                CHECK(w.player.stats.health == 100 - dmg[a]);
            }
        }
        CHECK(hit);
    }
}

TEST(seller_shoots_his_pistol_from_afar)
{
    World w;
    common::realtimeEnabled = false;   // no breather after an earlier hit on the player
    auto* m = w.spawn(new Myyja(Coord{ 54, 50 }, SellerWeapon::Pistooli));
    m->chaseTarget = &w.player;
    m->alerted = true;
    bool hit = false;
    for (int i = 0; i < 300 && !hit; ++i) {
        w.player.stats.health = 100;
        m->coord = Coord{ 54, 50 };
        m->npcAct();
        if (w.player.stats.health < 100) {
            hit = true;
            CHECK(w.player.stats.health == 98);
        }
    }
    CHECK(hit);
}

TEST(dead_seller_drops_knife_or_pistol_but_no_stapler)
{
    const SellerWeapon arms[] = { SellerWeapon::Nitoja, SellerWeapon::Veitsi, SellerWeapon::Pistooli };
    for (auto arm : arms) {
        World w;
        auto* m = w.spawn(new Myyja(Coord{ 60, 50 }, arm));
        m->damage(m->stats.health);
        w.level.cleanDead();
        CHECK(itemAt(w.level, EVeitsi) == (arm == SellerWeapon::Veitsi));
        CHECK(itemAt(w.level, EPistooli) == (arm == SellerWeapon::Pistooli));
        CHECK(itemAt(w.level, ERuumis));
    }
}

TEST(player_can_punch_seller_in_doorway)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.coord = k->getSpawn();
    auto* m = w.spawn(new Myyja(k->getDoor(), SellerWeapon::Nitoja));
    const uint32_t had = m->stats.health;
    feedKeys("x"); // should the shop menu open after all, it must not wait for a key
    w.player.move(k->door == DirectionNS::down ? DirectionNS::up : DirectionNS::down, &w.level);
    CHECK(m->stats.health < had);
    CHECK(w.player.coord == k->getSpawn());
    CHECK(!w.printer.saw("Kauppa on kiinni"));
}

TEST(seller_leaves_doorway_even_with_item_outside)
{
    World w(Coord{ 17, 40 });
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.level.addItem(new Ruumis(k->getSpawn()));
    auto* m = w.spawn(new Myyja(k->getDoor(), SellerWeapon::Nitoja));
    m->npcAct();
    CHECK(!(m->coord == k->getDoor()));
    CHECK(!w.level.hitBuilding(m->coord));
}

TEST(shoplift_prompts_for_the_number)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.inventory.right = Held();
    shoplift(w, k, "v1", 0);
    CHECK(w.printer.saw("Mit\xC3\xA4 varastat? (nro)"));
}

TEST(no_room_for_the_seller_means_nobody_noticed)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.inventory.right = Held();
    // customers stand in every doorway cell and outside it
    const Coord door = k->getDoor();
    for (auto c : { door, Coord{ door.x - 1, door.y }, Coord{ door.x + 1, door.y }, k->getSpawn() })
        w.spawn(new Juoppo(c));
    const string msg = shoplift(w, k, "v1", 1);
    CHECK(w.count(myyja) == 0);
    CHECK(!k->sellerHostile);
    CHECK(!contains(msg, "rynt"));
    CHECK(w.player.inventory.right.what == Thing::Lenkki);
}

TEST(dead_seller_is_forgotten_by_his_shop)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.inventory.right = Held();
    shoplift(w, k, "v1", 1);
    Myyja* m = sellerOf(w);
    CHECK(m != nullptr);
    if (!m) return;
    m->damage(m->stats.health);
    w.level.cleanDead();
    CHECK(k->seller == nullptr);
}

TEST(punched_dead_seller_drops_his_knife_once)
{
    World w;
    common::realtimeEnabled = false;
    auto* m = w.spawn(new Myyja(Coord{ 51, 50 }, SellerWeapon::Veitsi));
    m->stats.health = 1;
    w.player.move(DirectionNS::right, &w.level);
    w.level.cleanDead();
    size_t knives = 0;
    for (auto i : w.level.items) if (i->type == EVeitsi) ++knives;
    CHECK(knives == 1);
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
    const uint32_t had = w.player.inventory.count(Thing::Kalja);
    feedKeys("1");
    a->interact(&w.player, msg);
    CHECK(w.player.inventory.count(Thing::Kalja) == had);
    CHECK(InputNS::Input::getMenuKey() == '1'); // the key was left alone
}

TEST(alko_sells_beer)
{
    World w;
    auto* a = w.add(new Alko(), 10, 10);
    w.player.stats.promilles = 5;
    w.player.stats.money = 20;
    w.player.inventory.left = Held();
    const uint32_t had = w.player.inventory.count(Thing::Kalja);
    feedKeys("1");
    string msg;
    a->interact(&w.player, msg);
    CHECK(w.player.inventory.count(Thing::Kalja) == had + 1);
    CHECK(w.player.stats.money == 10);
}

TEST(alko_jaguar_needs_fifteen)
{
    World w;
    auto* a = w.add(new Alko(), 10, 10);
    w.player.stats.promilles = 5;
    w.player.stats.money = 12;
    w.player.inventory.left = Held();
    const uint32_t had = w.player.inventory.count(Thing::Lonkka);
    feedKeys("2");
    string msg;
    a->interact(&w.player, msg);
    CHECK(w.player.inventory.count(Thing::Lonkka) == had);
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
    w.player.inventory.left = Held();
    const uint32_t had = w.player.inventory.count(Thing::Vodka);
    feedKeys("1");
    s->interact(&w.player, msg);
    CHECK(w.player.inventory.count(Thing::Vodka) == had + 1);
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
    s->hasAlarm = true;
    auto* cop = w.spawn(new Cop(Coord{ 60, 15 }));
    w.player.inventory.left = Inventory::heldOf(Thing::Kivi);
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
    s->hasAlarm = true;
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
    s->hasAlarm = true;
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
        for (auto i : w.level.items) if (i->type != ESirpaleet) { ++loot; break; }
    }
    CHECK(loot > 20);
    CHECK(loot < 110); // usually there is nothing but dust
}

// defined with the coat tests further down
static void feedKeysLater(const string& now, const string& later, int ms);

static int glassSounds(World& w, const Coord& at)
{
    int n = 0;
    for (const auto& h : w.level.audio.getHeardSounds(at)) if (h.type == SoundType::Glass) ++n;
    return n;
}

static int shards(const Level& level)
{
    int n = 0;
    for (auto i : level.items) if (i->type == ESirpaleet) ++n;
    return n;
}

TEST(walking_into_a_shop_window_asks_and_then_breaks_in)
{
    World w(Coord{ 23, 19 });   // on the street, right above the front window at (23,20)
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
    s->hasAlarm = true;
    feedKeysLater("k", "k", 80);   // a key already waiting (from walking) does not answer
    w.player.move(DirectionNS::down, &w.level);
    CHECK(s->smashed);
    CHECK(s->isBreach(Coord{ 23, 20 }));
    CHECK(w.player.coord == (Coord{ 23, 19 }));   // breaking the glass took the turn
    CHECK(glassSounds(w, w.player.coord) == 1);
    CHECK(s->alarmTicks > 0);
    CHECK(w.printer.saw("Murtaudutko"));
    CHECK(w.printer.saw("H\xC3\xA4lytin alkaa ulvoa"));
    // glass on the hole and around it
    CHECK(shards(w.level) >= 2);
    bool onHole = false;
    for (auto i : w.level.items) if (i->type == ESirpaleet && i->coord == (Coord{ 23, 20 })) onHole = true;
    CHECK(onHole);
    // the next step goes through the hole into the shop
    Sirpaleet::cutOverride = 0;
    w.player.move(DirectionNS::down, &w.level);
    Sirpaleet::cutOverride = -1;
    CHECK(w.player.coord == (Coord{ 23, 20 }));
}

TEST(saying_no_leaves_the_shop_window_whole)
{
    World w(Coord{ 23, 19 });
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
    feedKeysLater("k", "e", 80);
    w.player.move(DirectionNS::down, &w.level);
    CHECK(!s->smashed);
    CHECK(shards(w.level) == 0);
    CHECK(w.player.coord == (Coord{ 23, 19 }));
}

TEST(a_broken_window_does_not_break_twice)
{
    World w(Coord{ 23, 26 });
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
    s->hasAlarm = false;
    s->smashWindow(Coord{ 23, 20 }, &w.player);
    const size_t glass = shards(w.level);
    s->smashWindow(Coord{ 23, 20 }, &w.player);
    CHECK(glassSounds(w, Coord{ 23, 22 }) == 1);
    CHECK((size_t)shards(w.level) == glass);
    CHECK(s->breaches.size() == 1);
}

TEST(shop_without_alarm_breaks_quietly_and_sends_no_patrol)
{
    World w(Coord{ 23, 26 });
    auto* s = w.add(new SuljettuLiike("ZARA", "sign"), 20, 20);
    s->hasAlarm = false;
    s->smashWindow(Coord{ 23, 20 }, &w.player);
    CHECK(s->smashed);
    CHECK(s->alarmTicks == 0);
    for (int i = 0; i < 40; ++i) s->npcAct();
    CHECK(w.count(poliisi) == 0);
    for (const auto& h : w.level.audio.getHeardSounds(Coord{ 23, 22 })) CHECK(h.type != SoundType::Siren);
}

TEST(only_some_closed_shops_have_an_alarm)
{
    int alarms = 0;
    for (int i = 0; i < 200; ++i) {
        SuljettuLiike s("ZARA", "sign");
        if (s.hasAlarm) ++alarms;
    }
    CHECK(alarms > 50);
    CHECK(alarms < 150);
}

TEST(glass_on_the_ground_can_cut_you_and_stays_there)
{
    World w(Coord{ 40, 40 });
    w.level.addItem(new Sirpaleet(Coord{ 41, 40 }));
    CHECK(!w.level.hitItem(Coord{ 41, 40 }));   // people walk over it, not around it
    w.player.stats.health = 10;
    Sirpaleet::cutOverride = 1;
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.coord == (Coord{ 41, 40 }));
    CHECK(w.player.stats.health == 9);
    CHECK(w.printer.saw("haava") || w.printer.saw("Haava") || w.printer.saw("Veri"));
    CHECK(w.player.lastHurtBy == "lasinsirpaleet");
    CHECK(shards(w.level) == 1);
    // careful steps: nothing happens
    Sirpaleet::cutOverride = 0;
    w.player.move(DirectionNS::left, &w.level);
    w.player.move(DirectionNS::right, &w.level);
    Sirpaleet::cutOverride = -1;
    CHECK(w.player.stats.health == 9);
}

// ---- sliding doors at the grocery store ----

TEST(jammed_sliding_door_mocks_you_then_gives_way_on_the_third_try)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    k->slidingDoor = true;
    k->doorJammed = true;
    CHECK(k->typeToChar(k->getDoor()) == "\xE2\x96\xA4");
    string msg;
    for (uint32_t i = 1; i < SLIDING_DOOR_FORCE_BUMPS; ++i) {
        msg.clear();
        CHECK(!k->getEnterMsg(&w.player, msg));
        CHECK(!msg.empty());
    }
    msg.clear();
    CHECK(k->getEnterMsg(&w.player, msg));
    CHECK(contains(msg, "liukuoven auki"));
    CHECK(!k->doorJammed);
    CHECK(k->typeToChar(k->getDoor()) != "\xE2\x96\xA4");
}

TEST(jammed_sliding_door_blocks_the_player_at_the_doorway)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    k->slidingDoor = true;
    k->doorJammed = true;
    const Coord door = k->getDoor();
    w.player.coord = Coord{ door.x, door.y + 1 };
    w.player.move(DirectionNS::up, &w.level);
    CHECK(w.player.coord == (Coord{ door.x, door.y + 1 }));
    CHECK(!w.printer.log.empty());
}

TEST(working_sliding_door_lets_you_in)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    k->slidingDoor = true;
    k->doorJammed = false;
    string msg;
    CHECK(k->getEnterMsg(&w.player, msg));
}

TEST(sliding_doors_break_down_and_come_back_now_and_then)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    k->slidingDoor = true;
    int flips = 0;
    bool was = k->doorJammed;
    for (int i = 0; i < 20000; ++i) {
        k->npcAct();
        if (k->doorJammed != was) { ++flips; was = k->doorJammed; }
    }
    CHECK(flips > 10);
    // a shop without sliding doors never jams
    auto* kiosk = w.add(new Kauppa("KIOSK", Kauppa::Kind::Kioski), 40, 10);
    for (int i = 0; i < 2000; ++i) kiosk->npcAct();
    CHECK(!kiosk->doorJammed);
}

// ---- the militia the alarm sends ----

static Cop* alarmCop(World& w, const Coord& at, bool radio)
{
    Cop* c = dynamic_cast<Cop*>(w.level.createPerson(at, poliisi));
    if (!c) return nullptr;
    c->chaseTarget = &w.player;
    c->alerted = true;
    c->lastKnownPos = w.player.coord;
    c->alertCooldown = 40;
    c->hasRadio = radio;
    c->callsBackup = radio;
    c->arrests = true;
    return c;
}

TEST(radio_cop_calls_in_more_men_when_the_burglar_keeps_running)
{
    World w(Coord{ 60, 60 });
    Cop* c = alarmCop(w, Coord{ 60, 52 }, true);
    CHECK(c != nullptr);
    if (!c) return;
    const size_t before = w.count(poliisi);
    for (uint32_t i = 0; i < COP_BACKUP_TURNS; ++i) {
        w.player.coord = Coord{ 60, c->coord.y + 8 };   // always a step ahead
        c->npcAct();
    }
    CHECK(c->backupCalled);
    CHECK(w.count(poliisi) > before);
    CHECK(w.said(c, "Lis\xC3\xA4\xC3\xA4 miehi\xC3\xA4"));
    // only once
    const size_t after = w.count(poliisi);
    for (int i = 0; i < 10; ++i) { w.player.coord = Coord{ 60, c->coord.y + 8 }; c->npcAct(); }
    CHECK(w.count(poliisi) == after);
}

TEST(cop_without_a_radio_has_to_manage_alone)
{
    World w(Coord{ 60, 60 });
    Cop* c = alarmCop(w, Coord{ 60, 52 }, false);
    CHECK(c != nullptr);
    if (!c) return;
    const size_t before = w.count(poliisi);
    for (int i = 0; i < 12; ++i) { w.player.coord = Coord{ 60, c->coord.y + 8 }; c->npcAct(); }
    CHECK(!c->backupCalled);
    CHECK(w.count(poliisi) == before);
}

TEST(alarm_cop_next_to_the_burglar_arrests_him)
{
    World w(Coord{ 60, 60 });
    Cop* c = alarmCop(w, Coord{ 60, 59 }, false);
    CHECK(c != nullptr);
    if (!c) return;
    w.player.stats.health = 100;
    for (int i = 0; i < 60 && !w.player.inJail; ++i) { c->coord = Coord{ 60, 59 }; c->npcAct(); }
    CHECK(w.player.inJail);
}

TEST(alarm_patrol_cops_carry_radios_only_sometimes)
{
    int radios = 0, cops = 0;
    for (int i = 0; i < 40; ++i) {
        World w(Coord{ 100, 100 });
        cops += w.level.sendPatrol(&w.player, 2, true);
        for (auto p : w.level.persons) if (auto c = dynamic_cast<Cop*>(p)) {
            CHECK(c->arrests);
            CHECK(c->chaseTarget == &w.player);
            if (c->hasRadio) { ++radios; CHECK(c->callsBackup); }
        }
    }
    CHECK(cops > 40);
    CHECK(radios > 5);
    CHECK(radios < cops - 5);
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

TEST(turning_is_limited_per_update)
{
    using VisionNS::Vec;
    // right to left, at most 60 degrees at a time: up-right, up-left, left
    Vec v { 1.f, 0.f };
    const Vec left { -1.f, 0.f };
    v = VisionNS::turnTowards(v, left, 60.f);
    CHECK(std::fabs(VisionNS::angleBetween(v, Vec{ 1.f, 0.f }) - 60.f) < 0.5f);
    CHECK(v.y < 0.f);                           // an about-turn goes counter-clockwise: up first
    v = VisionNS::turnTowards(v, left, 60.f);
    CHECK(std::fabs(VisionNS::angleBetween(v, left) - 60.f) < 0.5f);
    v = VisionNS::turnTowards(v, left, 60.f);
    CHECK(VisionNS::angleBetween(v, left) < 0.5f);
    // a small turn is done at once, the shorter way round
    v = VisionNS::turnTowards(Vec{ 1.f, 0.f }, Vec{ 1.f, 1.f }, 60.f);
    CHECK(VisionNS::angleBetween(v, Vec{ 1.f, 1.f }) < 0.5f);
}

TEST(field_of_vision_turns_around_gradually)
{
    World w(Coord{ 50, 50 });
    w.mask.enable();
    w.mask.maxTurn = VisionNS::TURN_PER_STEP;
    w.level.updateCamera();
    w.mask.init();                              // facing right
    CHECK(w.mask.visible(Coord{ 62, 50 }));
    for (int i = 0; i < 8; ++i) w.player.heading.step(-1, 0);
    w.mask.init();                              // turned a little: still not seeing far behind
    CHECK(!w.mask.visible(Coord{ 42, 50 }));
    w.mask.init();
    w.mask.init();                              // 3 x 60 degrees: now facing left
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
    hold(w.player.inventory, Thing::None, Thing::None);
    const uint32_t had = w.player.inventory.count(Thing::Lenkki);
    string msg;
    r->interact(msg, &w.player);
    r->interact(msg, &w.player);
    CHECK(w.player.inventory.count(Thing::Lenkki) == had + 1);
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
    w.player.inventory.right = Held();
    const uint32_t had = w.player.inventory.count(Thing::Lenkki);
    feedKeys("1");
    string msg;
    st->interact(&w.player, msg);
    CHECK(w.player.inventory.count(Thing::Lenkki) == had + 1);
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
    // the station only takes the fare: the trip itself is the mini-game, played by the main loop
    CHECK(w.player.metroFrom == a);
    CHECK(w.player.metroTrip == b);
    CHECK(!w.player.metroDodged);
    CHECK(w.player.stats.money == 40);
    CHECK(w.player.coord == (Coord{ 30, 30 }));
    CHECK(a->nextOnLine() == b);
    CHECK(b->nextOnLine() == a);               // and the line loops back round
    w.player.stats.money = 100;
    b->interact(&w.player, msg);
    CHECK(w.player.metroTrip == a);
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
        if (w.player.metroTrip == b) { ++rode; CHECK(w.player.metroDodged); } else ++caught;
    }
    CHECK(rode > 100);
    CHECK(caught > 20);
}

// ---- the metro mini-game ----

using MetroNS::MetroGame;
using MetroNS::Kind;
using MetroNS::Outcome;
using MetroNS::Phase;

static MetroNS::Npc metroNpc(Kind k, int x, int y, int waitMs = -1)
{
    MetroNS::Npc n;
    n.kind = k;
    n.x = x;
    n.y = y;
    n.glyph = "?";
    n.waitMs = waitMs;
    return n;
}

// a trip from A to B (C after that), sober, with full health and some money
struct Trip {
    World w;
    MetroGame g;
    explicit Trip(bool dodged = false) : w(), g((w.player.stats.promilles = 0, w.player.stats.health = 10, w.player.stats.money = 500, &w.player), "A", "B", "C", dodged) { }
    // let time pass until the trip ends or `ms` is up
    void run(int ms) { for (int t = 0; t < ms && !g.over() && !g.prompt.id; t += 50) g.tick(50); }
};

TEST(metro_trip_starts_in_the_tunnel_and_the_tunnel_leads_to_the_platform)
{
    Trip trip;
    auto& g = trip.g;
    CHECK(g.phase == Phase::Tunnel);
    CHECK(g.px == 1);
    // the far end of the upper walkway is the way up to the platform
    g.px = MetroGame::W - 2;
    g.py = 6;
    CHECK(g.tiles[6][MetroGame::W - 1] == MetroNS::TExit);
    g.step(1, 0);
    CHECK(g.phase == Phase::Platform);
}

TEST(metro_turning_back_in_the_tunnel_returns_you_to_the_street)
{
    Trip trip;
    trip.g.step(-1, 0);
    trip.g.step(-1, 0);
    CHECK(trip.g.outcome == Outcome::TurnedBack);
}

TEST(metro_train_in_the_tunnel_kills)
{
    Trip trip;
    auto& g = trip.g;
    g.npcs.clear();
    g.px = 40;
    g.py = 9;                                  // on the upper track
    g.trains[0].active = true;
    g.trains[0].x = 42;                        // coming from the right
    g.tick(50);
    CHECK(g.outcome == Outcome::Dead);
    CHECK(trip.w.player.stats.health > 0);    // the main loop does the dying
    CHECK(contains(g.cause, "Juna"));
}

TEST(metro_gravel_between_the_tracks_is_safe)
{
    Trip trip;
    auto& g = trip.g;
    g.npcs.clear();
    g.px = 40;
    g.py = 11;
    if (g.tiles[11][40] != MetroNS::TGravel) g.px = 41;
    g.trains[0].active = true;
    g.trains[0].x = 30;
    g.trains[1].active = true;
    g.trains[1].x = 30;
    g.tick(50);
    CHECK(g.outcome == Outcome::None);
}

TEST(metro_live_rail_shocks_but_the_boards_are_safe)
{
    Trip trip;
    auto& g = trip.g;
    g.npcs.clear();
    g.trains[0].nextMs = g.trains[1].nextMs = 100000;
    int live = -1, cover = -1;
    for (int x = 1; x < MetroGame::W - 1; ++x) {
        if (g.tiles[7][x] == MetroNS::TLive && g.tiles[6][x] == MetroNS::TWalk && live < 0) live = x;
        if (g.tiles[7][x] == MetroNS::TCover && g.tiles[6][x] == MetroNS::TWalk && cover < 0) cover = x;
    }
    CHECK(live >= 0 && cover >= 0);

    g.px = cover;
    g.py = 6;
    g.step(0, 1);
    CHECK(g.py == 7);
    CHECK(trip.w.player.stats.health == 10);

    g.px = live;
    g.py = 6;
    g.step(0, 1);
    CHECK(trip.w.player.stats.health == 6);
    CHECK(g.stunMs > 0);
    CHECK(contains(g.msg, "virtakisko"));
    // and it keeps on biting until you get off it, once the legs obey again
    g.stunMs = 0;
    g.tick(50);
    CHECK(trip.w.player.stats.health == 6);
    g.sinceShock = 5000;
    g.tick(50);
    CHECK(trip.w.player.stats.health == 2);
}

TEST(metro_walking_into_people_on_the_platform_is_a_punch_and_the_cop_comes)
{
    Trip trip;
    auto& g = trip.g;
    g.startPlatform();
    g.npcs.clear();
    g.px = 40;
    g.py = 15;
    g.npcs.push_back(metroNpc(Kind::Babushka, 40, 14));
    g.npcs.push_back(metroNpc(Kind::Cop, 44, 15));
    const uint32_t mummot = trip.w.player.scoreBoard.mummot;
    g.step(0, -1);
    CHECK(g.hits == 1);
    CHECK(g.py == 15);                                       // a punch, not a step
    CHECK(trip.w.player.scoreBoard.mummot == mummot + 1);
    CHECK(g.npcs[1].chasing);
    trip.run(5000);
    CHECK(g.outcome == Outcome::Arrested);
}

TEST(metro_the_third_punch_gets_returned)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.joltMs = 1000000;
    g.npcs.push_back(metroNpc(Kind::Citizen, g.px + 1, g.py));
    g.step(1, 0);
    g.step(1, 0);
    CHECK(g.npcs[0].angryMs == 0);
    g.step(1, 0);
    CHECK(g.hits == 3);
    CHECK(g.npcs[0].angryMs > 0);
    const uint32_t hp = trip.w.player.stats.health;
    trip.run(1500);
    CHECK(trip.w.player.stats.health < hp);
}

TEST(metro_punching_a_cop_or_the_fsb_ends_the_trip)
{
    {
        Trip trip;
        trip.g.startRide();
        trip.g.npcs.clear();
        trip.g.npcs.push_back(metroNpc(Kind::Cop, trip.g.px + 1, trip.g.py));
        trip.g.step(1, 0);
        CHECK(trip.g.outcome == Outcome::Arrested);
    }
    {
        Trip trip;
        trip.g.startRide();
        trip.g.npcs.clear();
        trip.g.npcs.push_back(metroNpc(Kind::Fsb, trip.g.px + 1, trip.g.py));
        trip.g.step(1, 0);
        CHECK(trip.g.outcome == Outcome::Recruited);
    }
}

TEST(metro_open_doors_let_you_on_the_train_closed_ones_do_not)
{
    Trip trip;
    auto& g = trip.g;
    g.startPlatform();
    g.npcs.clear();
    g.px = 10;                                 // under the first carriage's door
    g.py = 6;
    g.step(0, -1);                             // no train: down onto the track
    CHECK(g.phase == Phase::Platform);
    CHECK(g.py == 5);

    g.py = 6;
    g.stop = MetroGame::Stop::Boarding;
    g.trains[0].active = true;
    g.step(0, -1);
    CHECK(g.phase == Phase::Ride);
}

TEST(metro_falling_onto_the_track_as_the_train_comes_kills)
{
    Trip trip;
    auto& g = trip.g;
    g.startPlatform();
    g.npcs.clear();
    g.px = 40;
    g.py = 5;
    g.stop = MetroGame::Stop::Arriving;
    g.stopMs = 2400;                           // the train is all but in
    g.trains[0].active = true;
    g.tick(50);
    CHECK(g.outcome == Outcome::Dead);
}

TEST(metro_fsb_recruiter_signs_you_up)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.npcs.push_back(metroNpc(Kind::Fsb, g.px + 1, g.py, 0));
    g.tick(50);
    CHECK(g.prompt.id == 6);
    g.key('1');
    CHECK(g.outcome == Outcome::Recruited);
}

TEST(metro_fsb_recruiter_takes_no_for_an_answer_only_twice)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.npcs.push_back(metroNpc(Kind::Fsb, g.px + 1, g.py, 0));
    for (int round = 0; round < 2; ++round) {
        g.tick(50);
        CHECK(g.prompt.id == 6);
        g.key('2');
        CHECK(g.outcome == Outcome::None);
        CHECK(g.npcs[0].refusals == round + 1);
        g.npcs[0].stunMs = 0;                  // he's back
        g.npcs[0].x = g.px + 1;
        g.npcs[0].y = g.py;
    }
    g.tick(50);
    CHECK(g.prompt.id == 7);                   // he grabs your arm
    g.key('2');                                // no cop about: you tear free
    CHECK(g.outcome == Outcome::None);
    CHECK(g.npcs[0].talked);
}

TEST(metro_buying_from_the_dealer_next_to_a_cop_gets_you_arrested)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.npcs.push_back(metroNpc(Kind::Dealer, g.px - 1, g.py, 0));
    g.npcs.push_back(metroNpc(Kind::Cop, g.px + 5, g.py - 2));
    g.tick(50);
    CHECK(g.prompt.id == 2);
    g.key('1');
    CHECK(trip.w.player.stats.money == 350);
    CHECK(g.outcome == Outcome::Arrested);
}

TEST(metro_gopniks_want_a_hundred)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.npcs.push_back(metroNpc(Kind::Gopnik, g.px + 1, g.py));
    g.npcs.push_back(metroNpc(Kind::Gopnik, g.px + 6, g.py - 3));
    g.tick(50);
    CHECK(g.prompt.id == 1);
    g.key('1');
    CHECK(trip.w.player.stats.money == 400);
    CHECK(g.npcs[0].talked && g.npcs[1].talked);
    CHECK(g.npcs[0].angryMs == 0);
    // saying no to a gopnik with no money is another matter
    trip.w.player.stats.money = 50;
    g.npcs[0].talked = false;
    g.prompt = MetroNS::Prompt();
    g.npcs[0].waitMs = 0;
    g.tick(50);
    CHECK(g.prompt.id == 1);
    g.key('1');
    CHECK(g.npcs[0].angryMs > 0);
}

TEST(metro_ticket_check_catches_fare_dodgers)
{
    Trip trip(true);
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.npcs.push_back(metroNpc(Kind::Cop, g.px + 1, g.py, 0));
    trip.w.player.stats.money = 50;
    g.tick(50);
    CHECK(g.prompt.id == 4);
    g.key('1');                                // the fine is 300 and you have 50
    CHECK(g.outcome == Outcome::Arrested);
}

TEST(metro_running_from_the_ticket_check_gets_a_head_start_and_he_tires)
{
    Trip trip(true);
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.joltMs = 1000000;
    g.npcs.push_back(metroNpc(Kind::Cop, g.px + 1, g.py, 0));
    g.tick(50);
    CHECK(g.prompt.id == 4);
    g.key('3');
    CHECK(g.npcs[0].chasing);
    g.tick(50);
    CHECK(g.outcome == Outcome::None);        // not caught on the spot
    // he gives up in the end, if you keep away
    g.npcs[0].x = 70;
    g.npcs[0].y = 7;
    g.px = 2;
    g.py = 13;
    g.npcs[0].chaseMs = 100;
    trip.run(300);
    CHECK(!g.npcs[0].chasing);
    CHECK(g.outcome == Outcome::None);
}

TEST(metro_paying_passengers_pass_the_ticket_check_sober)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.npcs.push_back(metroNpc(Kind::Cop, g.px + 1, g.py, 0));
    g.tick(50);
    CHECK(g.prompt.id == 0);
    CHECK(g.npcs[0].talked);
    CHECK(g.outcome == Outcome::None);
}

TEST(metro_pickpocket_lifts_money_and_a_punch_gets_it_back)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.npcs.push_back(metroNpc(Kind::Pickpocket, g.px + 1, g.py, 0));
    for (int i = 0; i < 30 && g.npcs[0].stolen == 0; ++i) g.tick(50);
    CHECK(g.npcs[0].stolen > 0);
    CHECK(trip.w.player.stats.money < 500);
    g.npcs[0].x = g.px + 1;
    g.npcs[0].y = g.py;
    g.step(1, 0);
    CHECK(trip.w.player.stats.money == 500);
}

TEST(metro_getting_off_at_the_station)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.rideLeft = 50;
    g.tick(50);
    CHECK(g.doorsOpen());
    g.px = 9;
    g.py = 14;
    g.step(0, 1);
    CHECK(g.outcome == Outcome::Arrived);
    CHECK(contains(MetroNS::summarize(g), "B"));
}

TEST(metro_missing_the_stop_takes_you_one_further)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.joltMs = 1000000;
    g.rideLeft = 50;
    g.tick(50);
    CHECK(g.doorsOpen());
    trip.run(MetroGame::DOORS_MS + 100);
    CHECK(!g.doorsOpen());
    CHECK(g.overshot);
    CHECK(g.outcome == Outcome::None);
    trip.run(16000 + MetroGame::DOORS_MS + 200);
    CHECK(g.outcome == Outcome::Overshot);
    CHECK(contains(g.cause, "C"));
}

TEST(metro_drunk_legs_go_their_own_way)
{
    Trip trip;
    auto& g = trip.g;
    g.startPlatform();
    g.npcs.clear();
    g.stop = MetroGame::Stop::Waiting;
    g.stopMs = -1000000;                       // no train for a good while
    trip.w.player.stats.promilles = 5;         // sober enough: straight steps
    for (int i = 0; i < 100; ++i) {
        g.px = 40;
        g.py = 14;
        g.queued = 'a';
        g.sinceStep = 10000;
        g.tick(50);
        CHECK(g.py == 14);
    }
    trip.w.player.stats.promilles = 40;
    int astray = 0;
    for (int i = 0; i < 100; ++i) {
        g.px = 40;
        g.py = 14;
        g.queued = 'a';
        g.sinceStep = 10000;
        g.tick(50);
        if (g.py != 14) ++astray;
    }
    CHECK(astray > 15);
    CHECK(astray < 70);
}

TEST(metro_hothead_does_not_like_being_stared_at)
{
    Trip trip;
    auto& g = trip.g;
    g.startRide();
    g.npcs.clear();
    g.joltMs = 1000000;
    g.npcs.push_back(metroNpc(Kind::Hothead, g.px + 2, g.py));
    for (int i = 0; i < 60 && g.npcs[0].angryMs == 0; ++i) g.tick(50);
    CHECK(g.npcs[0].angryMs > 0);
    const uint32_t hp = trip.w.player.stats.health;
    trip.run(3000);
    CHECK(trip.w.player.stats.health < hp);
}

TEST(metro_scene_is_drawn_where_the_map_usually_is)
{
    Trip trip;
    const MetroNS::Rows rows = trip.g.render();
    CHECK(rows.size() == (size_t)MetroGame::H);
    for (const auto& r : rows) CHECK(r.size() == (size_t)MetroGame::W);
    trip.w.printer.scene = &rows;
    trip.w.printer.sceneTitle = trip.g.title();
    const auto cells = trip.w.printer.rowCells(&trip.w.level, trip.g.py);
    CHECK(cells.size() == trip.w.printer.viewW);
    CHECK(cells[trip.g.px].first == "@");
    CHECK(contains(trip.w.printer.locationName(&trip.w.level), "METRO"));
    trip.w.printer.scene = nullptr;
    // every part draws, and so does the end card
    trip.g.startPlatform();
    CHECK(trip.g.render()[trip.g.py][trip.g.px].first == "@");
    trip.g.startRide();
    CHECK(trip.g.render()[trip.g.py][trip.g.px].first == "@");
    trip.g.finish(Outcome::Dead, "Testi.");
    bool card = false;
    for (const auto& r : trip.g.render()) {
        string line;
        for (const auto& c : r) line += c.first;
        if (contains(line, "KUOLIT METROSSA")) card = true;
    }
    CHECK(card);
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
    hold(m.w.player.inventory, Thing::None, Thing::None);
    m.hm->interact(&m.w.player, msg);
    CHECK(!m.w.player.inventory.left.empty());   // the first grab always fits
    CHECK(contains(msg, "VARAS"));
    CHECK(m.g1->chasing && m.g2->chasing);
}

// ---- empty bottles: the return hatch and the people who want your sack ----

TEST(picked_up_bottle_goes_into_the_sack)
{
    World w;
    w.player.inventory.right = Held();
    Bona b(Coord{ 51, 50 });
    CHECK(b.interact(&w.player));
    CHECK(w.player.inventory.count(Thing::Tyhja) == 1);
    CHECK(w.player.inventory.right.what == Thing::Tyhja);
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

TEST(hatch_pays_on_the_receipt)
{
    Market m;
    atHatch(m);
    m.w.player.stats.money = 10;
    for (int round = 0; round < 3; ++round) {
        hold(m.w.player.inventory, Thing::Tyhja, Thing::Tyhja);
        m.w.player.move(DirectionNS::left, &m.w.level);
        CHECK(m.w.player.stats.money == 10);       // nothing paid before the receipt
    }
    CHECK(m.hm->hatchBottles == 6);
    m.w.player.move(DirectionNS::left, &m.w.level);  // one more bump: the receipt
    CHECK(m.w.player.stats.money == 11 && m.w.player.stats.kopecks == 20);
    CHECK(m.hm->hatchBottles == 0);
    CHECK(m.w.printer.saw("kuitin"));
}

TEST(deposit_hatch_takes_all_empty_bottles_in_hands)
{
    Market m;
    atHatch(m);
    m.w.player.stats.money = 10;
    hold(m.w.player.inventory, Thing::Tyhja, Thing::Tyhja);
    m.w.player.move(DirectionNS::left, &m.w.level);
    CHECK(m.w.player.inventory.count(Thing::Tyhja) == 0);
    CHECK(m.w.player.inventory.freeHands() == 2);
    CHECK(m.hm->hatchBottles == 2);
    CHECK(m.w.player.stats.money == 10);

    // only the empties go: a full beer stays in the hand
    hold(m.w.player.inventory, Thing::Kalja, Thing::Tyhja);
    m.w.player.move(DirectionNS::left, &m.w.level);
    CHECK(m.hm->hatchBottles == 3);
    CHECK(m.w.player.inventory.left.what == Thing::Kalja && m.w.player.inventory.right.empty());
    m.w.player.move(DirectionNS::left, &m.w.level);  // the receipt: 3 x 20 kop
    CHECK(m.w.player.stats.money == 10 && m.w.player.stats.kopecks == 60);
}

TEST(handing_in_a_bottle_calms_a_guard_who_sees_it)
{
    Market m;
    atHatch(m);
    m.g1->coord = Coord{ m.w.player.coord.x + 2, m.w.player.coord.y };
    m.g1->provoke(&m.w.player);
    m.w.player.inventory.left = Inventory::heldOf(Thing::Tyhja);
    m.w.player.move(DirectionNS::left, &m.w.level);
    CHECK(!m.g1->chasing);
}

TEST(loitering_at_the_hatch_without_bottles_angers_the_guards)
{
    Market m;
    atHatch(m);
    m.g1->coord = Coord{ m.w.player.coord.x + 2, m.w.player.coord.y };
    CHECK(m.w.player.inventory.count(Thing::Tyhja) == 0);   // a full beer is no deposit
    m.w.player.move(DirectionNS::left, &m.w.level);
    CHECK(m.g1->chasing);
    CHECK(m.w.player.stats.money == 20 && m.w.player.stats.kopecks == 0);
}

TEST(one_bottle_draws_nobody)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 52, 50 }));
    hold(w.player.inventory, Thing::Tyhja, Thing::Lenkki);
    for (int i = 0; i < 300; ++i) w.level.npcFrame(500);
    CHECK(w.player.inventory.count(Thing::Tyhja) == 1);
    CHECK(j->stolenBottles == 0);
}

TEST(a_drunk_comes_for_a_full_sack_takes_a_handful_and_picks_a_fight)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 56, 50 }));   // further than he can normally see
    hold(w.player.inventory, Thing::Tyhja, Thing::Tyhja);
    bool came = false;
    for (int i = 0; i < 400 && w.player.inventory.count(Thing::Tyhja) == 2; ++i) {
        w.level.npcFrame(500);
        if (j->coord.distance(w.player.coord) <= 2) came = true;
    }
    CHECK(came);
    CHECK(w.player.inventory.count(Thing::Tyhja) < 2);
    CHECK(j->stolenBottles == 2 - w.player.inventory.count(Thing::Tyhja));
    CHECK(j->stolenBottles >= 2);                       // both hands, not just one
    CHECK(j->brawlActions > 0);                         // and now he wants a fight
}

TEST(someone_feared_takes_just_one_bottle)
{
    World w;
    auto* s = w.spawn(new Skinhead(Coord{ 51, 50 }));
    w.player.stats.money = 0;   // nothing to pick from the pocket meanwhile
    hold(w.player.inventory, Thing::Tyhja, Thing::Tyhja);
    for (int i = 0; i < 400 && w.player.inventory.count(Thing::Tyhja) == 2; ++i) w.level.npcFrame(500);
    CHECK(w.player.inventory.count(Thing::Tyhja) == 1);
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
            hold(w.player.inventory, Thing::Tyhja, Thing::Tyhja);
            w.level.npcFrame(500);
            if (w.player.inventory.count(Thing::Tyhja) < 2) ++n;
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
    hold(w.player.inventory, Thing::Tyhja, Thing::Tyhja);
    for (int i = 0; i < 5; ++i) w.level.npcFrame(500);   // he gets to act at least once
    CHECK(j->brawlActions == 0);
}

TEST(catching_a_bottle_thief_gets_everything_back)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 51, 50 }));
    j->stolenBottles = 1;
    j->stolenItems = { Inventory::heldOf(Thing::Kalja), Inventory::heldOf(Thing::Lenkki) };
    j->stolenMoney = 5;
    hold(w.player.inventory, Thing::None, Thing::None);
    const uint32_t money = w.player.stats.money;
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.inventory.count(Thing::Tyhja) + w.player.inventory.count(Thing::Kalja) == 2);   // two hands
    CHECK(itemAt(w.level, ELenkki) || itemAt(w.level, EKalja) || itemAt(w.level, EBona));        // the rest on the ground
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
    // a feedKeysLater() key nobody read may be written after the next test swapped stdin
    std::signal(SIGPIPE, SIG_IGN);
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
    string msg;
    CHECK(!gun->approach(&w.player, msg));  // no bottle: shoved away
    CHECK(w.player.manning == nullptr);
    hold(w.player.inventory, Thing::Vodka, Thing::Vodka);
    CHECK(gun->approach(&w.player, msg));
    CHECK(w.player.inventory.count(Thing::Vodka) == 1);
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
    hold(w.player.inventory, Thing::Vodka, Thing::Lenkki);
    string msg;
    CHECK(!gun->approach(&w.player, msg));
    CHECK(w.player.inventory.count(Thing::Vodka) == 1);   // the bottle is not wasted
}

TEST(aa_gun_sleeping_crew_lets_the_drunk_take_it)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 1);
    gun->crew().front()->doze(50);
    hold(w.player.inventory, Thing::Vodka, Thing::Vodka);
    string msg;
    CHECK(gun->approach(&w.player, msg));
    CHECK(w.player.inventory.count(Thing::Vodka) == 2);   // no bribe needed
    CHECK(contains(msg, "kuorsaa"));
}

TEST(aa_gun_can_be_captured_once_the_crew_is_dead)
{
    World w(Coord{ 20, 20 });
    auto* gun = placeTestGun(w, 200, 150, 1);
    gun->crew().front()->stats.health = 0;
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

TEST(kremlin_guards_open_fire_on_a_drone_they_see)
{
    World w(Coord{ 20, 20 });
    w.level.events = new EnvironmentEvents(&w.level, true);
    w.add(new Kremlin(), 250, 20);
    auto* outside = new Vartija(Coord{ 272, 43 });   // at the gate, under the open sky
    outside->hasRadio = true;
    w.spawn(outside);
    w.level.drones.push_back(Drone{ 280.0f, 38.0f, 0.0f, 0.0f });
    for (int i = 0; i < 300 && !w.level.drones.empty(); ++i) antiDroneFire(w.level);
    CHECK(w.level.drones.empty());
}

TEST(guards_indoors_or_without_radio_hold_fire)
{
    World w(Coord{ 20, 20 });
    w.add(new Kremlin(), 250, 20);
    auto* inside = new Vartija(Coord{ 270, 38 });    // in the Kremlin halls: no sky to see
    inside->hasRadio = true;
    w.spawn(inside);
    w.spawn(new Vartija(Coord{ 100, 100 }));         // a hypermarket guard: not his business
    w.level.drones.push_back(Drone{ 272.0f, 36.0f, 0.0f, 0.0f });
    w.level.drones.push_back(Drone{ 104.0f, 98.0f, 0.0f, 0.0f });
    for (int i = 0; i < 100; ++i) antiDroneFire(w.level);
    CHECK(w.level.drones.size() == 2);
}

// a parked black car at (x, y) and an FSB man come to grab the player
static Car* parkFsbCar(World& w, uint32_t x, uint32_t y)
{
    auto* car = new Car(Coord{ x, y }, true, -1, Car::Kind::Fsb);
    car->script = Car::Script::Parked;
    w.level.addVehicle(car);
    w.level.fsbPatrol.cars.push_back(car);
    w.level.fsbPatrol.stage = FsbPatrol::Stage::Grabbing;
    return car;
}

static FSBAgent* recruiterAt(World& w, Coord c)
{
    auto* a = dynamic_cast<FSBAgent*>(w.level.createPerson(c, fsb));
    a->recruiter = true;
    a->firstMove = false;
    a->patience = 80;
    a->lastHealth = a->stats.health;
    return a;
}

static bool nextToCar(const Car* car, const Coord& c)
{
    for (const auto& cell : car->cells())
        if (std::abs((int)cell.x - (int)c.x) <= 1 && std::abs((int)cell.y - (int)c.y) <= 1) return true;
    return false;
}

TEST(fsb_drags_player_to_the_car_door_before_conscripting)
{
    World w(Coord{ 60, 120 });
    w.level.roads = { { true, 112, "TVERSKAJA UL." } };
    auto* car = parkFsbCar(w, 62, 113);
    auto* agent = recruiterAt(w, Coord{ 61, 120 });

    agent->npcAct();
    CHECK(agent->holding);               // grabbed, not yet conscripted
    CHECK(!w.player.conscripted);
    CHECK(w.printer.saw("tarttuu sinua niskasta"));

    bool conscriptedAway = false;
    for (int i = 0; i < 200 && !w.player.conscripted; ++i) {
        const Coord before = w.player.coord;
        agent->npcAct();
        if (w.player.conscripted && !nextToCar(car, before)) conscriptedAway = true;
    }
    CHECK(w.player.conscripted);
    CHECK(!conscriptedAway);             // only ever at the car door
    CHECK(nextToCar(car, w.player.coord));
    CHECK(w.printer.saw("takapenkille"));
}

TEST(fsb_grip_breaks_when_player_fights_back)
{
    World w(Coord{ 60, 125 });
    w.level.roads = { { true, 112, "TVERSKAJA UL." } };
    parkFsbCar(w, 62, 113);
    auto* agent = recruiterAt(w, Coord{ 61, 125 });
    agent->npcAct();
    CHECK(agent->holding);

    string msg;
    for (int i = 0; i < 20 && agent->holding && agent->stats.health > 0; ++i) agent->interact(msg, &w.player);
    CHECK(!agent->holding);
    CHECK(!w.player.conscripted);
}

TEST(fsb_grip_breaks_when_shot)
{
    World w(Coord{ 60, 125 });
    w.level.roads = { { true, 112, "TVERSKAJA UL." } };
    parkFsbCar(w, 62, 113);
    auto* agent = recruiterAt(w, Coord{ 61, 125 });
    agent->npcAct();
    CHECK(agent->holding);
    agent->damage(5);                    // a pistol round
    const Coord at = w.player.coord;
    agent->npcAct();
    CHECK(!agent->holding);
    CHECK(w.player.coord == at);         // not dragged this time
    CHECK(w.printer.saw("ote irtoaa"));
}

TEST(fsb_grip_can_be_wriggled_out_of)
{
    World w(Coord{ 60, 125 });
    w.level.roads = { { true, 112, "TVERSKAJA UL." } };
    parkFsbCar(w, 62, 113);
    auto* agent = recruiterAt(w, Coord{ 61, 125 });
    agent->npcAct();
    CHECK(agent->holding);
    w.player.move(DirectionNS::left, &w.level);
    int tries = 1;
    while (agent->holding && tries < 60) { w.player.move(DirectionNS::left, &w.level); ++tries; }
    CHECK(!agent->holding);
    CHECK(tries > 1 || w.printer.saw("Riuhtaiset"));
    CHECK(w.printer.saw("pit\xC3\xA4\xC3\xA4 sinusta kiinni") || tries == 1);
}

TEST(fsb_drives_off_when_player_wins_the_fight)
{
    World w(Coord{ 60, 125 });
    w.level.roads = { { true, 112, "TVERSKAJA UL." } };
    parkFsbCar(w, 62, 113);
    auto* agent = recruiterAt(w, Coord{ 61, 125 });
    auto* quitter = recruiterAt(w, Coord{ 90, 125 });
    quitter->patience = 0;
    quitter->npcAct();                   // lost him: waits by the car, arrests nobody
    CHECK(quitter->gaveUp);

    string msg;
    for (int i = 0; i < 40 && agent->stats.health > 0; ++i) agent->interact(msg, &w.player);
    CHECK(agent->stats.health == 0);
    w.level.cleanDead();
    w.level.fsbTick();
    CHECK(w.level.fsbPatrol.stage == FsbPatrol::Stage::Idle);
    CHECK(w.printer.saw("palaavat autoihin"));
    CHECK(w.count(fsb) == 0);            // the quitter got back in the car too
    CHECK(!w.player.conscripted);
    CHECK(!w.player.inJail);
}

// ---- militsija on the streets ----

// a street with a row of shops along it, a narrow alley between each pair
static void buildStreet(World& w)
{
    w.level.roads.push_back(Road{ true, 100, "TESTSKAJA" });
    w.level.roads.push_back(Road{ false, 200, "TESTOVKA" });
    for (uint32_t x = 20; x < 180; x += 17) w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), x, 92);
}

TEST(cop_spots_are_on_pavements_and_in_alleys)
{
    World w(Coord{ 20, 20 });
    buildStreet(w);
    for (int i = 0; i < 20; ++i) {
        const Coord c = Patrol::pavementSpot(w.level);
        CHECK(!(c.x == 0 && c.y == 0));
        const Road* h = nullptr; const Road* v = nullptr;
        w.level.roadsAt(c, h, v);
        CHECK(!h && !v);                                       // not out in the traffic
        CHECK(c.y == 99 || c.y == 103 || c.x == 199 || c.x == 203);   // right beside the kerb
        CHECK(!w.level.hit(c));
    }
    for (int i = 0; i < 20; ++i) {
        const Coord a = Patrol::alleySpot(w.level);
        CHECK(Patrol::inAlley(w.level, a));
        CHECK(!w.level.hit(a));
        CHECK(a.y >= 90 && a.y <= 98);                         // between the shops
    }
    CHECK(!Patrol::inAlley(w.level, Coord{ 300, 30 }));        // open ground is no alley
}

TEST(streets_are_full_of_cops)
{
    World w(Coord{ 20, 20 });
    buildStreet(w);
    Patrol::populateCops(w.level, 1);
    const size_t cops = w.count(poliisi);
    CHECK(cops >= Patrol::wantedCops(1));
    CHECK(Patrol::wantedCops(1) >= 12);                        // far more than the old two to five
    CHECK(Patrol::wantedCops(8) > Patrol::wantedCops(1));      // and more towards the Kremlin
    size_t street = 0;
    for (auto p : w.level.persons)
        if (p->type == poliisi && (Patrol::onPavement(w.level, p->coord) || Patrol::inAlley(w.level, p->coord))) ++street;
    CHECK(street * 10 >= cops * 9);                             // nearly all on the streets and in the alleys
}

TEST(cops_on_the_beat_stroll_along_the_pavement)
{
    World w(Coord{ 20, 20 });
    buildStreet(w);
    auto* cop = w.spawn(new Cop(Coord{ 300, 103 }));            // on the pavement south of TESTSKAJA
    cop->level = &w.level;
    uint32_t minX = cop->coord.x, maxX = cop->coord.x, onStreet = 0;
    for (int i = 0; i < 200; ++i) {
        cop->npcAct();
        minX = std::min(minX, cop->coord.x);
        maxX = std::max(maxX, cop->coord.x);
        if (Patrol::onPavement(w.level, cop->coord) || Patrol::inAlley(w.level, cop->coord)) ++onStreet;
        const Road* h = nullptr; const Road* v = nullptr;
        w.level.roadsAt(cop->coord, h, v);
        CHECK(!h && !v);                                        // never strolls out into the traffic
    }
    CHECK(maxX - minX >= 30);                                   // walks the beat, no shuffling on the spot
    CHECK(onStreet >= 180);                                     // and keeps to the pavement
}

TEST(new_patrols_walk_in_out_of_sight)
{
    World w(Coord{ 100, 104 });
    buildStreet(w);
    for (int i = 0; i < 2000 && w.count(poliisi) < Patrol::wantedCops(1); ++i) Patrol::patrolTick(w.level);
    CHECK(w.count(poliisi) == Patrol::wantedCops(1));           // topped up, then no more
    for (int i = 0; i < 200; ++i) Patrol::patrolTick(w.level);
    CHECK(w.count(poliisi) == Patrol::wantedCops(1));
    for (auto p : w.level.persons) {
        if (p->type != poliisi) continue;
        const int dx = std::abs((int)p->coord.x - 100), dy = std::abs((int)p->coord.y - 104);
        CHECK(dx >= 44 || dy >= 18);                            // none popped up on screen
    }
    w.level.isWarLevel = true;                                  // no militsija at the front
    for (auto p : w.level.persons) if (p->type == poliisi) p->stats.health = 0;
    w.level.cleanDead();
    for (int i = 0; i < 500; ++i) Patrol::patrolTick(w.level);
    CHECK(w.count(poliisi) == 0);
}

// ---- FPV suicide drones at the front ----

static size_t fpvCount(const Level& l) { size_t n = 0; for (const auto& d : l.drones) if (d.fpv) ++n; return n; }

TEST(fpv_drone_goes_for_a_russian_soldier_not_a_prisoner)
{
    World w(Coord{ 20, 20 });
    w.level.isWarLevel = true;
    auto* pow = w.spawn(new Vanki(Coord{ 105, 100 }));           // close by, but a prisoner
    auto* soldier = w.spawn(new Sotilas(Coord{ 120, 100 }));
    CHECK(!isRussianSoldier(w.level, pow));
    CHECK(isRussianSoldier(w.level, soldier));
    CHECK(fpvPickPrey(w.level, 100.0f, 100.0f) == soldier);
    // the conscripted player at the front is fair game, and the nearest one goes first
    CHECK(isRussianSoldier(w.level, &w.player));
    CHECK(fpvPickPrey(w.level, 22.0f, 22.0f) == &w.player);
}

TEST(fpv_drone_leaves_a_civilian_drunk_alone_in_moscow)
{
    World w(Coord{ 20, 20 });
    CHECK(!isRussianSoldier(w.level, &w.player));       // not called up: just a drunk
    w.player.conscripted = true;
    CHECK(isRussianSoldier(w.level, &w.player));        // the FSB took him: now he is a soldier
}

TEST(fpv_drone_hunts_its_target_and_dives_in)
{
    World w(Coord{ 20, 20 });
    w.level.isWarLevel = true;
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* soldier = w.spawn(new Sotilas(Coord{ 150, 100 }));
    soldier->stats.health = 15;
    launchFpv(w.level, 110.0f, 80.0f, soldier);
    CHECK(fpvCount(w.level) == 1);
    CHECK(w.level.drones[0].prey == soldier);
    bool dived = false;
    for (int f = 0; f < 20 * 30 && !w.level.drones.empty(); ++f) {
        if (f % 8 == 0 && soldier->stats.health > 0) soldier->coord.y += (f / 8) % 2 ? 1 : -1;   // he shuffles about
        w.level.vehicleFrame(50);
        if (!w.level.drones.empty() && w.level.drones[0].diving) dived = true;
    }
    CHECK(dived);
    CHECK(w.level.drones.empty());          // it blew itself up
    CHECK(soldier->stats.health < 15);      // on him
}

TEST(fpv_blast_kills_at_the_point_and_spares_those_far_away)
{
    World w(Coord{ 20, 20 });
    w.level.isWarLevel = true;
    w.level.events = new EnvironmentEvents(&w.level, true);
    auto* target = w.spawn(new Skinhead(Coord{ 100, 100 }));
    auto* far = w.spawn(new Skinhead(Coord{ 110, 100 }));
    fpvBlast(w.level, Coord{ 100, 100 });
    CHECK(target->stats.health == 0);
    CHECK(far->stats.health == 7);
}

TEST(fpv_drones_swarm_the_front_but_not_moscow)
{
    World front(Coord{ 100, 100 });
    front.level.isWarLevel = true;
    front.spawn(new Sotilas(Coord{ 130, 110 }));
    for (int i = 0; i < 200; ++i) fpvTick(front.level);
    CHECK(fpvCount(front.level) >= 3);
    CHECK(fpvCount(front.level) <= fpvMax(front.level));

    World moscow(Coord{ 100, 100 });
    moscow.spawn(new Sotilas(Coord{ 130, 110 }));
    for (int i = 0; i < 200; ++i) fpvTick(moscow.level);
    CHECK(fpvCount(moscow.level) == 0);
}

TEST(soldiers_at_the_front_shoot_fpv_drones_down)
{
    World w(Coord{ 20, 20 });
    w.level.isWarLevel = true;
    w.level.events = new EnvironmentEvents(&w.level, true);
    w.spawn(new Sotilas(Coord{ 100, 100 }));
    w.spawn(new Sotilas(Coord{ 104, 102 }));
    launchFpv(w.level, 110.0f, 95.0f, nullptr);
    w.level.drones[0].vx = w.level.drones[0].vy = 0;
    for (int i = 0; i < 400 && fpvCount(w.level) > 0; ++i) antiDroneFire(w.level);
    CHECK(fpvCount(w.level) == 0);
}

TEST(comrades_at_the_front_do_not_send_the_player_to_the_front)
{
    World w(Coord{ 100, 100 });
    w.level.isWarLevel = true;
    auto* s = w.spawn(new Sotilas(Coord{ 101, 100 }));
    s->firstMove = false;
    const uint32_t hp = w.player.stats.health;
    for (int i = 0; i < 50; ++i) s->npcAct();
    CHECK(!w.player.inJail);
    CHECK(w.player.stats.health == hp);
}

// ---- hands ----
TEST(hands_give_fills_left_then_right_then_refuses)
{
    Inventory inv;
    inv.left = Held();
    inv.right = Held();
    CHECK(inv.give(Thing::Kalja));
    CHECK(inv.give(Thing::Kalja));
    CHECK(!inv.give(Thing::Kalja));
    CHECK(inv.count(Thing::Kalja) == 2);
    CHECK(inv.left.what == Thing::Kalja && inv.right.what == Thing::Kalja);
}

TEST(hands_vodka_starts_with_six_sips)
{
    Inventory inv;
    inv.left = Held();
    CHECK(inv.give(Thing::Vodka));
    CHECK(inv.left.what == Thing::Vodka);
    CHECK(inv.left.fill == VODKA_SIPS);
    CHECK(inv.left.fill == 6);
}

TEST(hands_pistol_goes_to_belt_only_with_belt)
{
    Inventory a;
    a.left = Held();
    a.right = Held();
    CHECK(a.give(Thing::Pistooli));
    CHECK(a.belt.empty());
    CHECK(a.right.what == Thing::Pistooli);
    CHECK(a.right.rounds == MAKAROV_ROUNDS);

    Inventory b;
    b.left = Held();
    b.right = Held();
    b.hasBelt = true;
    CHECK(b.give(Thing::Pistooli));
    CHECK(b.belt.what == Thing::Pistooli);
    CHECK(b.belt.rounds == 8);
    CHECK(b.freeHands() == 2);
}

TEST(hands_second_ak_is_refused)
{
    Inventory inv;
    CHECK(inv.give(Thing::Rynkky));
    CHECK(inv.back.what == Thing::Rynkky);
    CHECK(inv.back.rounds == AK_ROUNDS);
    inv.left = Held();
    CHECK(!inv.give(Thing::Rynkky));
    CHECK(inv.count(Thing::Rynkky) == 1);
    CHECK(inv.left.empty());
}

TEST(hands_lipas_prefers_pocket)
{
    Inventory inv;
    inv.left = Held();
    inv.right = Held();
    CHECK(inv.give(Thing::Lipas));
    CHECK(inv.pocket.what == Thing::Lipas);
    CHECK(inv.freeHands() == 2);
    CHECK(inv.give(Thing::Lipas));
    CHECK(inv.left.what == Thing::Lipas);
}

TEST(hands_small_item_overflows_to_pocket)
{
    Inventory inv; // both hands full in the default kit
    CHECK(inv.freeHands() == 0);
    CHECK(inv.give(Thing::Veitsi));
    CHECK(inv.pocket.what == Thing::Veitsi);
    CHECK(!inv.give(Thing::Granata));
    CHECK(!inv.give(Thing::Kivi));
}

TEST(hands_take_on_empty_is_false)
{
    Inventory inv;
    CHECK(!inv.take(Thing::Vodka));
    CHECK(inv.take(Thing::Kalja));
    CHECK(inv.left.empty());
    CHECK(!inv.take(Thing::Kalja));
    CHECK(inv.find(Thing::Lenkki) == &inv.right);
    CHECK(inv.find(Thing::Kalja) == nullptr);
}

TEST(hands_default_kit_is_beer_and_sausage)
{
    Inventory inv;
    CHECK(inv.left.what == Thing::Kalja);
    CHECK(inv.right.what == Thing::Lenkki);
    CHECK(inv.count(Thing::Kalja) == 1);
    CHECK(inv.count(Thing::Lenkki) == 1);
    CHECK(inv.freeHands() == 0);
    CHECK(isSmall(Thing::Lipas) && isSmall(Thing::Granata) && isSmall(Thing::Veitsi));
    CHECK(isDrink(Thing::Vodka) && !isDrink(Thing::Lenkki));
    CHECK(isWeapon(Thing::Rynkky) && isWeapon(Thing::Pistooli));
    CHECK(string(thingName(Thing::Kalja)) == "PIVO");
    CHECK(inv.pocket.empty());
}

TEST(hands_give_held_keeps_fill_and_seller)
{
    Inventory inv;
    inv.left = Held();
    Held h;
    h.what = Thing::Vodka;
    h.fill = 2;
    h.pissSeller = "Vasja";
    CHECK(inv.give(h));
    CHECK(inv.left.fill == 2 && inv.left.pissSeller == "Vasja");
    CHECK(stealableThing(2) == Thing::Vodka);
}

TEST(pickup_with_full_hands_leaves_item_on_map)
{
    World w;
    CHECK(w.player.inventory.freeHands() == 0);   // the default kit: beer and sausage
    w.level.addItem(new Kalja(Coord{ 51, 50 }));
    w.player.move(DirectionNS::right, &w.level);
    CHECK(itemAt(w.level, EKalja));
    CHECK(w.player.inventory.count(Thing::Kalja) == 1);
    CHECK(w.printer.saw(HINT_HANDS_FULL));

    // a free hand picks it up
    w.player.inventory.right = Held();
    w.player.move(DirectionNS::right, &w.level);
    w.player.move(DirectionNS::left, &w.level);
    CHECK(!itemAt(w.level, EKalja));
    CHECK(w.player.inventory.count(Thing::Kalja) == 2);
}

TEST(shop_refuses_and_keeps_money_when_hands_full)
{
    World w;
    auto* k = w.add(new Kauppa("TEST", Kauppa::Kind::Produkty), 10, 10);
    w.player.stats.money = 20;
    feedKeys("1");
    string msg;
    k->interact(&w.player, msg);
    CHECK(w.player.stats.money == 20);
    CHECK(w.player.inventory.count(Thing::Lenkki) == 1);
    CHECK(contains(msg, HINT_HANDS_FULL));
}

TEST(robber_loot_that_does_not_fit_drops_to_ground)
{
    World w;
    w.player.inventory.pocket = Inventory::heldOf(Thing::Lipas);   // the pocket is taken too
    Car* car = new Car(Coord{ 51, 50 }, true, -1);
    car->loot = true;
    w.level.addVehicle(car);
    w.player.move(DirectionNS::right, &w.level);
    CHECK(!car->loot);
    CHECK(w.player.inventory.count(Thing::Pistooli) == 0);   // no free hand for it
    CHECK(itemAt(w.level, EPistooli));
    CHECK(itemAt(w.level, ELipas));
    for (auto i : w.level.items) CHECK(i->coord.distance(w.player.coord) <= 1);
}

// ---- drinking, eating, throwing from the hands ----

TEST(vodka_sip_lowers_fill_by_one_and_adds_promille)
{
    World w;
    hold(w.player.inventory, Thing::Vodka, Thing::None);
    const int p0 = w.player.stats.promilles;
    CHECK(w.player.drink(&w.level));
    CHECK(w.player.inventory.left.what == Thing::Vodka);
    CHECK(w.player.inventory.left.fill == 5);
    CHECK(w.player.stats.promilles == p0 + 1);
    CHECK(w.printer.saw(VODKA_SIP_LINES[5]));
}

TEST(six_sips_leave_empty_bottle_in_hand)
{
    World w;
    hold(w.player.inventory, Thing::Vodka, Thing::None);
    for (int i = 0; i < 6; ++i) w.player.drink(&w.level);
    CHECK(w.player.inventory.left.what == Thing::Tyhja);
    CHECK(w.player.inventory.left.pissSeller.empty());
    CHECK(w.printer.saw(VODKA_SIP_LINES[0]));
}

TEST(beer_is_drunk_in_one_go)
{
    World w;
    hold(w.player.inventory, Thing::Kalja, Thing::None);
    const int p0 = w.player.stats.promilles;
    CHECK(w.player.drink(&w.level));
    CHECK(w.player.inventory.left.what == Thing::Tyhja);
    CHECK(w.player.stats.promilles == p0 + 2);
    bool one = false;
    for (const auto& l : BEER_LINES) if (w.printer.saw(l)) one = true;
    CHECK(one);
}

TEST(piss_vodka_reveals_on_first_sip)
{
    World w;
    hold(w.player.inventory, Thing::Vodka, Thing::None);
    w.player.inventory.left.pissSeller = "Vasja";
    common::realtimeEnabled = false;   // no breather after an earlier test's hit
    const int hp = w.player.stats.health;
    CHECK(w.player.drink(&w.level));
    CHECK(w.player.stats.health == hp - 2);
    CHECK(w.player.inventory.left.what == Thing::Tyhja);
    CHECK(w.printer.saw("Vasja"));
}

TEST(drink_with_no_drink_says_nothing_to_drink)
{
    World w;
    hold(w.player.inventory, Thing::Lenkki, Thing::None);
    CHECK(!w.player.drink(&w.level));
    CHECK(w.printer.saw("Ei oo ehtaa tavaraa"));
}

TEST(drink_target_prefers_vodka_then_left_then_right)
{
    World w;
    hold(w.player.inventory, Thing::Kalja, Thing::Vodka);
    CHECK(w.player.drinkTarget() == &w.player.inventory.right);
    hold(w.player.inventory, Thing::Lonkka, Thing::Kalja);
    CHECK(w.player.drinkTarget() == &w.player.inventory.left);
    hold(w.player.inventory, Thing::Lenkki, Thing::Kalja);
    CHECK(w.player.drinkTarget() == &w.player.inventory.right);
    hold(w.player.inventory, Thing::Lenkki, Thing::None);
    CHECK(w.player.drinkTarget() == nullptr);
}

TEST(eat_uses_food_in_hand)
{
    World w;
    hold(w.player.inventory, Thing::Kalja, Thing::Lenkki);
    CHECK(w.player.eat(&w.level));
    CHECK(w.player.inventory.right.empty());
    CHECK(w.player.inventory.left.what == Thing::Kalja);
}

TEST(throw_asks_which_hand_only_when_both_are_full)
{
    World w;
    hold(w.player.inventory, Thing::Kalja, Thing::Lenkki);
    feedKeys("o");
    DirectionNS::Direction dir = DirectionNS::right;
    (void)dir;
    Held* h = w.player.chooseHand(&w.level);
    CHECK(h == &w.player.inventory.right);
    hold(w.player.inventory, Thing::Kalja, Thing::Lenkki);
    feedKeys("x");
    CHECK(w.player.chooseHand(&w.level) == nullptr);
    hold(w.player.inventory, Thing::None, Thing::Lenkki);
    CHECK(w.player.chooseHand(&w.level) == &w.player.inventory.right);   // no key needed
}

TEST(full_hands_kick_bona_and_it_counts)
{
    World w;
    hold(w.player.inventory, Thing::Kalja, Thing::Lenkki);
    w.player.inventory.pocket = Inventory::heldOf(Thing::Lipas);
    Bona* b = new Bona(Coord{ 51, 50 });
    w.level.addItem(b);
    w.player.move(DirectionNS::right, &w.level);
    CHECK(!itemAt(w.level, EBona));
    CHECK(w.player.inventory.count(Thing::Tyhja) == 0);
    CHECK(!w.printer.log.empty() && contains(w.printer.log.back(), "potkaiset pullon rikki"));   // not the pickup line
    NextLevel exit(Coord{ 60, 60 }, &w.level);
    CHECK(exit.interact(&w.player));   // that was the last bottle: the exit is open
}

TEST(overlay_throw_takes_from_selected_hand)
{
    World w;
    hold(w.player.inventory, Thing::Kalja, Thing::Kalja);
    w.player.inventory.left.fill = 0;
    w.player.inventory.right.rounds = 7;   // a marker to tell the hands apart
    CHECK(w.player.takeThrowable(w.player.inventory.right) == thingToThrowNum(Thing::Kalja));
    CHECK(w.player.inventory.right.empty());
    CHECK(w.player.inventory.left.what == Thing::Kalja);
}

TEST(overlay_eat_uses_selected_hand)
{
    World w;
    hold(w.player.inventory, Thing::Lenkki, Thing::Lenkki);
    w.player.eatHeld(w.player.inventory.right, &w.level);
    CHECK(w.player.inventory.right.empty());
    CHECK(w.player.inventory.left.what == Thing::Lenkki);
}

// ---- weapons, belt, magazines, pocket, coats ----

static Held loaded(Thing gun)
{
    Held h = Inventory::heldOf(gun);
    h.rounds = gun == Thing::Rynkky ? AK_ROUNDS : MAKAROV_ROUNDS;
    return h;
}

TEST(pistol_needs_a_free_hand)
{
    World w;
    auto& inv = w.player.inventory;
    inv.hasBelt = true;
    hold(inv, Thing::Kalja, Thing::Lenkki);
    inv.belt = loaded(Thing::Pistooli);
    CHECK(!w.player.drawPistol());
    CHECK(w.printer.saw(HINT_PISTOL_BOTTLE));
    CHECK(inv.belt.what == Thing::Pistooli);
    CHECK(!w.player.shootAt(0.0f));              // no hand to hold it: no shot
    CHECK(inv.belt.rounds == MAKAROV_ROUNDS);

    inv.right = Held();
    CHECK(w.player.drawPistol());
    CHECK(inv.right.what == Thing::Pistooli);
    CHECK(inv.right.rounds == MAKAROV_ROUNDS);
    CHECK(inv.belt.empty());
    CHECK(w.player.drawPistol());                // already in hand counts as drawn
}

TEST(belt_frees_the_hand_until_drawn)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::None, Thing::Pistooli);
    inv.right.rounds = 5;
    w.level.addItem(new Vyo(Coord{ 51, 50 }));
    w.player.move(DirectionNS::right, &w.level);
    CHECK(inv.hasBelt);
    CHECK(!itemAt(w.level, EVyo));
    CHECK(inv.belt.what == Thing::Pistooli);
    CHECK(inv.belt.rounds == 5);                 // the same gun, not a new one
    CHECK(inv.freeHands() == 2);

    CHECK(w.player.drawPistol());
    CHECK(inv.freeHands() == 1);
    CHECK(inv.belt.empty());
    w.player.holsterPistol();
    CHECK(inv.belt.what == Thing::Pistooli);
    CHECK(inv.freeHands() == 2);

    // without a belt the pistol just stays in the hand
    World v;
    hold(v.player.inventory, Thing::None, Thing::Pistooli);
    v.player.holsterPistol();
    CHECK(v.player.inventory.right.what == Thing::Pistooli);
}

TEST(soldier_corpse_sometimes_leaves_a_belt)
{
    int belts = 0;
    for (int n = 0; n < 300; ++n) {
        Sotilas s(Coord{ 10, 10 });
        Item* i = s.dropItem();
        if (i && i->type == EVyo) ++belts;
        CHECK(s.dropItem() == nullptr);          // one corpse, one loot
        delete i;
    }
    CHECK(belts > 60 && belts < 130);
}

TEST(soldiers_are_small_s_officers_big_S)
{
    World w;
    auto* s = w.spawn(new Sotilas(Coord{ 60, 50 }));
    auto* o = w.spawn(new Sotilas(Coord{ 62, 50 }, true));
    CHECK(s->typeToChar() == "s");
    CHECK(o->typeToChar() == "S");
    CHECK(o->type == sotilas);                       // still a soldier to everyone else
    CHECK(o->stats.health > s->stats.health);
    CHECK(w.level.nameOf(s) == "Sotilas");
    CHECK(w.level.nameOf(o) == "Upseeri");
    // now and then a soldier the level makes is an officer
    int officers = 0, soldiers = 0;
    for (int n = 0; n < 200; ++n) {
        Person* p = w.level.createPerson(w.level.freePosition(), sotilas);
        if (!p) continue;
        ++soldiers;
        if (p->typeToChar() == "S") ++officers;
    }
    CHECK(soldiers > 150);
    CHECK(officers > soldiers / 20 && officers < soldiers / 2);
}

TEST(officer_corpse_leaves_a_pistol_or_a_belt)
{
    int pistols = 0, belts = 0;
    for (int n = 0; n < 300; ++n) {
        Sotilas o(Coord{ 10, 10 }, true);
        Item* i = o.dropItem();
        if (i && i->type == EPistooli) ++pistols;
        if (i && i->type == EVyo) ++belts;
        CHECK(o.dropItem() == nullptr);
        delete i;
    }
    CHECK(pistols > 100 && pistols < 200);
    CHECK(belts > 40);
}

TEST(soldier_takes_the_vodka_from_your_hand_and_lets_you_go)
{
    World w;
    hold(w.player.inventory, Thing::Vodka, Thing::None);
    auto* s = w.spawn(new Sotilas(Coord{ 51, 50 }));
    s->firstMove = false;
    bool taken = false;
    for (int i = 0; i < 300 && !taken; ++i) {
        s->coord = Coord{ 51, 50 };
        s->passedOut = 0;
        s->npcAct();
        taken = !w.player.inventory.inHand(Thing::Vodka);
    }
    CHECK(taken);
    CHECK(!w.player.inJail);                         // a bottle buys you off
    CHECK(s->stolenItems.size() == 1);               // bump him to get back what is left of it
    for (int i = 0; i < 20; ++i) {
        s->coord = Coord{ 51, 50 };
        s->passedOut = 0;
        s->npcAct();
    }
    CHECK(!w.player.inJail);                         // busy with the bottle
}

TEST(officer_does_not_take_your_bottle)
{
    World w;
    hold(w.player.inventory, Thing::Vodka, Thing::None);
    auto* o = w.spawn(new Sotilas(Coord{ 51, 50 }, true));
    o->firstMove = false;
    for (int i = 0; i < 50; ++i) {
        o->coord = Coord{ 51, 50 };
        o->npcAct();
    }
    CHECK(w.player.inventory.inHand(Thing::Vodka) != nullptr);
    CHECK(w.player.inJail);
}

TEST(soldier_sprays_bursts_officer_aims)
{
    World w;
    auto* s = w.spawn(new Sotilas(Coord{ 54, 50 }));
    auto* o = w.spawn(new Sotilas(Coord{ 46, 50 }, true));
    CHECK(s->ammo == 12);
    CHECK(o->ammo == 8);
    string msg;
    s->fire(&w.player, msg);
    CHECK(s->ammo == 9 || s->ammo == 10);            // two or three rounds
    o->fire(&w.player, msg);
    CHECK(o->ammo == 7);                             // one aimed shot
}

TEST(hit_soldier_rages_until_an_officer_calms_him)
{
    World w(Coord{ 20, 20 });
    auto* s = w.spawn(new Sotilas(Coord{ 150, 100 }));
    auto* o = w.spawn(new Sotilas(Coord{ 153, 100 }, true));
    s->firstMove = false;
    string msg;
    s->interact(msg, &w.player);
    CHECK(s->rage > 0);
    bool calmed = false;
    for (int i = 0; i < 300 && !calmed; ++i) {
        s->rage = 15;
        s->passedOut = 0;
        s->coord = Coord{ 150, 100 };
        s->npcAct();
        calmed = s->rage == 0;
    }
    CHECK(calmed);
    CHECK(w.said(o, "Rauhoitu"));
}

TEST(soldiers_drink_and_pass_out_officers_do_not)
{
    World w(Coord{ 20, 20 });
    auto* s = w.spawn(new Sotilas(Coord{ 150, 100 }));
    auto* o = w.spawn(new Sotilas(Coord{ 150, 110 }, true));
    s->firstMove = o->firstMove = false;
    bool out = false, officerOut = false;
    for (int i = 0; i < 2000; ++i) {
        s->npcAct();
        o->npcAct();
        out = out || s->passedOut > 0;
        officerOut = officerOut || o->passedOut > 0;
    }
    CHECK(out);
    CHECK(!officerOut);
}

TEST(ak_needs_both_hands_empty_and_hints)
{
    World w;
    auto& inv = w.player.inventory;
    inv.back = loaded(Thing::Rynkky);
    hold(inv, Thing::Kalja, Thing::None);
    CHECK(!w.player.shoulderAk());
    CHECK(w.printer.saw(HINT_AK_HANDS));
    CHECK(!inv.akRaised);

    inv.left = Held();
    CHECK(w.player.shoulderAk());
    CHECK(inv.akRaised);
    CHECK(inv.freeHands() == 0);
    CHECK(!inv.give(Thing::Kalja));              // the rifle fills both hands
    CHECK(inv.left.empty() && inv.right.empty());
    CHECK(inv.give(Thing::Granata));             // the pocket is still free
    CHECK(inv.pocket.what == Thing::Granata);

    w.player.lowerAk();
    CHECK(!inv.akRaised);
    CHECK(inv.freeHands() == 2);
    CHECK(inv.back.what == Thing::Rynkky);
}

TEST(front_puts_a_loaded_ak_in_your_hands)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::Kalja, Thing::Lenkki);
    w.player.issueRifle();
    CHECK(inv.back.what == Thing::Rynkky);
    CHECK(inv.back.rounds == AK_ROUNDS);
    CHECK(inv.akRaised);
    CHECK(inv.left.empty() && inv.right.empty());
    // what was in the hands lies on the ground next to you
    int kalja = 0, lenkki = 0;
    for (auto i : w.level.items) {
        if (i->held.what == Thing::Kalja) ++kalja;
        if (i->held.what == Thing::Lenkki) ++lenkki;
    }
    CHECK(kalja == 1 && lenkki == 1);
    CHECK(w.printer.saw(AK_ISSUED_MSG));
}

TEST(front_refills_the_ak_you_already_carry_and_it_stays_after)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::None, Thing::None);
    inv.back = loaded(Thing::Rynkky);
    inv.back.rounds = 3;
    w.player.issueRifle();
    CHECK(inv.count(Thing::Rynkky) == 1);
    CHECK(inv.back.rounds == AK_ROUNDS);

    // back home from the front the rifle comes along
    GameState gs;
    gs.sendToWar();
    gs.returnFromWar();
    w.player.resetPosition();
    w.player.onNewLevel();
    CHECK(gs.phase == GamePhase::ReturnMoscow);
    CHECK(inv.back.what == Thing::Rynkky);
}

TEST(trigger_tells_a_held_space_from_a_new_pull)
{
    Trigger t;
    CHECK(t.pull());                  // the first press
    t.tick(50);
    CHECK(!t.pull());                 // the key repeating: still held
    t.tick(100);
    CHECK(!t.pull());                 // repeats keep it held, however long
    t.tick(Trigger::REPEAT_GAP_MS);
    CHECK(t.pull());                  // let go and pressed again
    CHECK(!t.pull());                 // two in the same frame: the second is the repeat
}

TEST(makarov_has_eight_rounds_and_reloads_from_pocket)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::None, Thing::None);
    CHECK(inv.give(Thing::Pistooli));
    CHECK(inv.right.rounds == 8);
    inv.pocket = Inventory::heldOf(Thing::Lipas);
    for (int i = 0; i < 8; ++i) CHECK(w.player.shootAt(0.0f));
    CHECK(inv.right.rounds == MAKAROV_ROUNDS);   // the last round out, the spare in
    CHECK(inv.pocket.empty());
    CHECK(w.printer.saw("Vaihdat lippaan."));
    for (int i = 0; i < 8; ++i) CHECK(w.player.shootAt(0.0f));
    CHECK(inv.right.rounds == 0);
    CHECK(!w.player.shootAt(0.0f));
    CHECK(w.printer.saw("Ei ammuksia!"));

    // the same spare fills the rifle
    inv.back = Inventory::heldOf(Thing::Rynkky);
    inv.pocket = Inventory::heldOf(Thing::Lipas);
    CHECK(w.player.reload(Thing::Rynkky));
    CHECK(inv.back.rounds == AK_ROUNDS);
    CHECK(inv.pocket.empty());
    CHECK(!w.player.reload(Thing::Rynkky));      // no spare left
}

TEST(empty_ak_does_not_block_loaded_pistol)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::None, Thing::None);
    inv.right = loaded(Thing::Pistooli);
    inv.back = Inventory::heldOf(Thing::Rynkky);   // no rounds left
    CHECK(w.player.readyWeapon() == Thing::Pistooli);

    // pistol on the belt, hands free, rifle empty: still the pistol
    inv.hasBelt = true;
    inv.belt = inv.right;
    inv.right = Held();
    CHECK(w.player.readyWeapon() == Thing::Pistooli);

    // a loaded rifle and free hands: the rifle
    inv.back.rounds = AK_ROUNDS;
    CHECK(w.player.readyWeapon() == Thing::Rynkky);

    // the pistol in hand goes first
    CHECK(w.player.drawPistol());
    CHECK(w.player.readyWeapon() == Thing::Pistooli);

    hold(inv, Thing::None, Thing::None);
    inv.belt = Held();
    inv.back = Held();
    CHECK(w.player.readyWeapon() == Thing::None);
}

TEST(fall_can_drop_pocket_item_and_notices_later)
{
    World w;
    auto& inv = w.player.inventory;
    inv.pocket = Inventory::heldOf(Thing::Granata);
    w.player.maybeDropPocket(true);
    CHECK(inv.pocket.empty());
    bool here = false;
    for (auto i : w.level.items) if (i->type == EGranata && i->coord == w.player.coord) here = true;
    CHECK(here);                                 // on the ground at once
    CHECK(w.player.pocketNoticeTurns == PlayerNS::POCKET_NOTICE_TURNS);
    for (uint32_t t = 1; t < PlayerNS::POCKET_NOTICE_TURNS; ++t) w.player.updateTurn();
    CHECK(!w.printer.saw("tasku tuntuu kevyelt"));
    w.player.updateTurn();
    CHECK(w.printer.saw("Hetkinen... tasku tuntuu kevyelt\xC3\xA4!"));

    // dropping to all fours rolls the dice: about one fall in four
    int falls = 0, dropped = 0;
    w.player.stats.promilles = 40;
    for (int n = 0; n < 100000 && falls < 200; ++n) {
        w.player.crawling = false;
        inv.pocket = Inventory::heldOf(Thing::Lipas);
        w.player.updateCrawling();
        if (!w.player.crawling) {
            CHECK(inv.pocket.what == Thing::Lipas);   // no fall, nothing lost
            continue;
        }
        ++falls;
        if (inv.pocket.empty()) ++dropped;
    }
    CHECK(falls == 200);
    CHECK(dropped > 25 && dropped < 80);
}

TEST(empty_pocket_drop_spawns_nothing)
{
    World w;
    w.player.inventory.pocket = Held();
    const size_t n = w.level.items.size();
    w.player.maybeDropPocket(true);
    CHECK(w.level.items.size() == n);
    CHECK(w.player.pocketNoticeTurns == 0);
}

TEST(coat_restores_full_health)
{
    World w;
    w.player.stats.health = 3;
    w.player.usedCoat = true;
    w.level.addItem(new Takki(Coord{ 51, 50 }));
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.stats.health == PlayerNS::INITIAL_HP);
    CHECK(!itemAt(w.level, ETakki));
    CHECK(w.printer.saw("Uusi takki! Vain v\xC3\xA4h\xC3\xA4n verta kauluksessa."));
    CHECK(!w.player.usedCoat);
    CHECK(w.player.inventory.count(Thing::Kalja) == 1);   // worn, not carried
    CHECK(w.player.inventory.count(Thing::Lenkki) == 1);
    Takki t(Coord{ 0, 0 });
    CHECK(t.typeToChar() == "[");

    // a coat never takes health away
    w.player.stats.health = PlayerNS::INITIAL_HP + 2;
    w.player.wearCoat(PlayerNS::INITIAL_HP);
    CHECK(w.player.stats.health == PlayerNS::INITIAL_HP + 2);
}

// keys already waiting are drained before a prompt: `now` is in the pipe at once, `later` comes after a moment
static void feedKeysLater(const string& now, const string& later, int ms = 80)
{
    int fds[2];
    if (pipe(fds) != 0) return;
    if (!now.empty() && write(fds[1], now.data(), now.size()) < 0) { }
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    clearerr(stdin);
    const int w = fds[1];
    std::thread([w, later, ms]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        if (write(w, later.data(), later.size()) < 0) { }
        close(w);
    }).detach();
}

static Ruumis* corpseAt(const Level& level, const Coord& c)
{
    for (auto i : level.items)
        if (i->type == ERuumis && i->coord == c) return static_cast<Ruumis*>(i);
    return nullptr;
}

TEST(stripping_corpse_coat_angers_witnesses)
{
    World w;
    w.player.stats.health = 2;
    auto* corpse = new Ruumis(Coord{ 51, 50 });
    w.level.addItem(corpse);
    auto* cop = w.spawn(new Cop(Coord{ 55, 50 }));
    auto* passer = w.spawn(new Kansalainen(Coord{ 51, 53 }));
    auto* blindMan = w.spawn(new Kansalainen(Coord{ 50, 53 }));
    blindMan->blind = true;
    feedKeysLater("", "k");
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.printer.saw("Riisutko takin? (k/e)"));
    CHECK(corpse->coatTaken);
    CHECK(w.player.stats.health == PlayerNS::INITIAL_HP);
    CHECK(w.player.coord == (Coord{ 51, 50 }));            // a body does not block the way
    CHECK(corpseAt(w.level, Coord{ 51, 50 }) == corpse);   // it stays where it fell
    CHECK(w.player.inventory.count(Thing::Kalja) == 1 && w.player.inventory.count(Thing::Lenkki) == 1);
    CHECK(cop->alerted);
    CHECK(cop->chaseTarget == &w.player);
    CHECK(w.said(cop, "Ruumiinry\xC3\xB6st\xC3\xA4j\xC3\xA4!") || w.said(cop, "Paskiainen!"));
    CHECK(passer->brawlActions > 0);                       // a civilian comes at you too
    CHECK(!blindMan->alerted && blindMan->brawlActions == 0);

    // a 'k' already waiting (a step down) does not answer the question
    World v;
    v.player.stats.health = 2;
    auto* body = new Ruumis(Coord{ 51, 50 });
    v.level.addItem(body);
    feedKeysLater("k", "e");
    v.player.move(DirectionNS::right, &v.level);
    CHECK(v.printer.saw("Riisutko takin? (k/e)"));
    CHECK(!body->coatTaken);
    CHECK(v.player.stats.health == 2);
}

TEST(gunshot_kill_leaves_corpse_with_coat)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::None, Thing::None);
    inv.right = loaded(Thing::Pistooli);
    auto* victim = w.spawn(new Kansalainen(Coord{ 54, 50 }));
    for (int i = 0; i < 8 && victim->stats.health > 0; ++i) w.player.shootAt(0.0f);
    CHECK(victim->stats.health == 0);
    w.level.cleanDead();
    CHECK(w.count(kansalainen) == 0);
    Ruumis* corpse = corpseAt(w.level, Coord{ 54, 50 });
    CHECK(corpse != nullptr);
    if (!corpse) return;
    CHECK(corpse->typeToChar() == "\xE2\x80\xA0");
    CHECK(!corpse->coatTaken);

    // a rat leaves no body
    auto* rat = w.spawn(new Rotta(Coord{ 60, 60 }));
    rat->stats.health = 0;
    w.level.cleanDead();
    CHECK(corpseAt(w.level, Coord{ 60, 60 }) == nullptr);

    w.player.stats.health = 3;
    w.player.coord = Coord{ 53, 50 };
    feedKeysLater("", "k");
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.stats.health == PlayerNS::INITIAL_HP);
    CHECK(corpse->coatTaken);
    NextLevel exit(Coord{ 70, 70 }, &w.level);
    CHECK(exit.interact(&w.player));                       // a body is not a bottle for the exit
}

TEST(corpse_coat_only_once)
{
    World w;
    w.player.stats.health = 2;
    auto* corpse = new Ruumis(Coord{ 51, 50 });
    w.level.addItem(corpse);
    feedKeysLater("", "k");
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.stats.health == PlayerNS::INITIAL_HP);
    w.player.move(DirectionNS::left, &w.level);
    w.player.stats.health = 2;
    w.printer.log.clear();
    feedKeysLater("", "k");
    w.player.move(DirectionNS::right, &w.level);
    CHECK(!w.printer.saw("Riisutko takin?"));
    CHECK(w.player.stats.health == 2);
    CHECK(corpseAt(w.level, Coord{ 51, 50 }) == corpse);   // still a body, now coatless
    CHECK(!w.player.stripCorpseCoat(corpse));

    // no question when a coat would give nothing: full health, or the disease's cap
    World v;
    auto* body = new Ruumis(Coord{ 51, 50 });
    v.level.addItem(body);
    v.player.move(DirectionNS::right, &v.level);           // full health
    CHECK(!v.printer.saw("Riisutko takin?"));
    v.player.move(DirectionNS::left, &v.level);
    v.player.hiv.needleStick(0, 0);
    v.player.stats.turn = v.player.hiv.stageStart(3);
    CHECK(v.player.coatCap() == Disease::maxHealth(3));
    v.player.stats.health = Disease::maxHealth(3);
    v.player.move(DirectionNS::right, &v.level);
    CHECK(!v.printer.saw("Riisutko takin?"));
    CHECK(!body->coatTaken);
}

TEST(divari_used_coat_gives_three_hp)
{
    World w;
    auto* d = w.add(new Divari(), 10, 10);
    w.player.stats.health = 4;
    w.player.stats.money = 40;
    string msg;
    CHECK(d->getEnterMsg(&w.player, msg));       // nothing to sell, but money for the coat
    CHECK(d->getInteractMsg(&w.player, msg));
    CHECK(contains(msg, "K\xC3\xA4ytetty takki 30\xE2\x82\xBD"));
    feedKeys("5");
    msg.clear();
    d->interact(&w.player, msg);
    CHECK(w.player.stats.health == 4 + PlayerNS::USED_COAT_HP);
    CHECK(w.player.stats.money == 10);
    CHECK(w.player.usedCoat);

    // never past full health
    w.player.stats.health = 9;
    w.player.stats.money = 30;
    feedKeys("5");
    d->interact(&w.player, msg);
    CHECK(w.player.stats.health == PlayerNS::INITIAL_HP);
    CHECK(w.player.stats.money == 0);

    // broke: no coat
    w.player.stats.health = 4;
    feedKeys("5");
    d->interact(&w.player, msg);
    CHECK(w.player.stats.health == 4);
    CHECK(w.player.stats.money == 0);
    CHECK(!d->getEnterMsg(&w.player, msg));      // nothing to sell, no money: out

    // a coat that would give nothing is not sold
    w.player.stats.health = PlayerNS::INITIAL_HP;
    w.player.stats.money = 40;
    w.player.usedCoat = false;
    feedKeys("5");
    msg.clear();
    d->interact(&w.player, msg);
    CHECK(w.player.stats.money == 40);
    CHECK(!w.player.usedCoat);
    CHECK(contains(msg, "Et tarvitse"));
}

// ---- arm pictures ----

static Held heldOf(Thing t, uint8_t fill = 0) { Held h; h.what = t; h.fill = fill; return h; }

static std::vector<Held> allHeldVariants()
{
    std::vector<Held> v;
    for (int t = (int)Thing::None; t <= (int)Thing::Lipas; ++t) v.push_back(heldOf((Thing)t));
    for (uint8_t f = 0; f <= VODKA_SIPS; ++f) v.push_back(heldOf(Thing::Vodka, f));
    return v;
}

static bool plainAsciiRow(const string& r)
{
    if ((int)r.size() != ARM_W) return false;
    for (char c : r) if (c < 0x20 || c > 0x7e) return false;
    return true;
}

// the inside of the bottle body: rows 15-20, columns 2-7
static string bottleInside(const std::vector<string>& pic, int row) { return pic[row].substr(2, 6); }

TEST(arm_pictures_are_14_by_22)
{
    for (const Held& h : allHeldVariants())
        for (int stage = 0; stage <= 3; ++stage)
            for (bool left : { true, false }) {
                const auto pic = armPicture(left, h, stage);
                CHECK((int)pic.size() == ARM_H);
                for (const auto& r : pic) {
                    CHECK(plainAsciiRow(r));
                    CHECK(plainAsciiRow(mirrorRow(r)));
                }
            }
}

TEST(arm_right_sleeve_mirrors_the_left)
{
    for (int stage = 0; stage <= 3; ++stage) {
        const auto l = armPicture(true, Held(), stage), r = armPicture(false, Held(), stage);
        for (int row = 0; row < ARM_H; ++row) CHECK(r[row] == mirrorRow(l[row]));
    }
    // a pistol is drawn for the right hand and mirrored into the left
    const auto pl = armPicture(true, heldOf(Thing::Pistooli), 0), pr = armPicture(false, heldOf(Thing::Pistooli), 0);
    for (int row = 9; row < ARM_H; ++row) CHECK(pl[row] == mirrorRow(pr[row]));
}

TEST(arm_hands_differ_by_what_they_hold)
{
    // every thing that fits in a hand has its own picture
    std::vector<Thing> kinds;
    for (int t = (int)Thing::None; t <= (int)Thing::Lipas; ++t) kinds.push_back((Thing)t);
    for (size_t i = 0; i < kinds.size(); ++i)
        for (size_t j = i + 1; j < kinds.size(); ++j)
            CHECK(armPicture(true, heldOf(kinds[i], 6), 0) != armPicture(true, heldOf(kinds[j], 6), 0));
    // the sleeve does not depend on the hand
    const auto a = armPicture(true, Held(), 2), b = armPicture(true, heldOf(Thing::Pistooli), 2);
    for (int row = 0; row < 9; ++row) CHECK(a[row] == b[row]);
    // other things do not print their name in the picture (the caption has it)
    CHECK(!contains(armPicture(true, heldOf(Thing::Kivi), 0)[21], "KIVI"));
    CHECK(!contains(armPicture(false, heldOf(Thing::Lenkki), 1)[21], "MAKKARA"));
}

TEST(arm_vodka_fill_three_shows_three_liquid_rows)
{
    const auto pic = armPicture(true, heldOf(Thing::Vodka, 3), 0);
    for (int row = 15; row <= 17; ++row) CHECK(bottleInside(pic, row) == "      ");
    CHECK(bottleInside(pic, 18) == "~~~~~~");
    CHECK(bottleInside(pic, 19) == "######");
    CHECK(bottleInside(pic, 20) == "######");
    // full: the label stays on its row
    const auto full = armPicture(true, heldOf(Thing::Vodka, 6), 0);
    CHECK(bottleInside(full, 15) == "~~~~~~");
    CHECK(contains(bottleInside(full, 16), "VODKA"));
    CHECK(bottleInside(full, 20) == "######");
    const auto five = armPicture(true, heldOf(Thing::Vodka, 5), 0);
    CHECK(bottleInside(five, 15) == "      ");
    CHECK(contains(bottleInside(five, 16), "VODKA"));
    CHECK(bottleInside(five, 16)[0] == '~');
}

TEST(arm_empty_bottle_has_no_liquid)
{
    for (const Held& h : { heldOf(Thing::Tyhja), heldOf(Thing::Vodka, 0) }) {
        const auto pic = armPicture(true, h, 0);
        for (int row = 15; row <= 20; ++row) CHECK(bottleInside(pic, row) == "      ");
    }
}

TEST(arm_sleeve_stage_thresholds)
{
    CHECK(sleeveStage(10, false) == 0);
    CHECK(sleeveStage(8, false) == 0);
    CHECK(sleeveStage(7, false) == 1);
    CHECK(sleeveStage(5, false) == 1);
    CHECK(sleeveStage(4, false) == 2);
    CHECK(sleeveStage(2, false) == 2);
    CHECK(sleeveStage(1, false) == 3);
    CHECK(sleeveStage(0, false) == 3);
    CHECK(sleeveStage(10, true) == 1);
    CHECK(sleeveStage(3, true) == 2);
}

TEST(police_coat_off_a_corpse_shows_blue_and_whole_in_the_arms)
{
    World w;
    w.player.stats.health = 2;
    auto* corpse = new Ruumis(Coord{ 51, 50 }, Coat::Poliisi);
    w.level.addItem(corpse);
    feedKeysLater("", "k");
    w.player.move(DirectionNS::right, &w.level);
    CHECK(corpse->coatTaken);
    CHECK(w.player.coat == Coat::Poliisi);
    CHECK(w.printer.saw("poliisin takin"));
    const int stage = sleeveStage(w.player.stats.health, w.player.usedCoat);
    CHECK(stage == 0);
    for (bool left : { true, false }) {
        const auto pic = armPicture(left, Held(), stage, w.player.coat);
        const auto mask = armColours(left, Held(), stage, w.player.coat);
        int blue = 0;
        for (int r = 0; r < 9; ++r) {
            CHECK(mask[r].find('J') == string::npos);           // none of the poster's own jacket
            blue += (int)std::count(mask[r].begin(), mask[r].end(), 'B');
            CHECK(pic[r].find("( )") == string::npos);          // whole: no holes
            CHECK(pic[r].find("/ \\") == string::npos);
        }
        CHECK(blue > 40);
        CHECK(pic != armPicture(left, Held(), 0, Coat::Oma));
    }
    // it tears like any coat as the strength goes, and stays blue
    const auto torn = armColours(true, Held(), 2, Coat::Poliisi);
    CHECK(torn[0].find('B') != string::npos && torn[0].find('J') == string::npos);
    CHECK(armPicture(true, Held(), 2, Coat::Poliisi) == armPicture(true, Held(), 2, Coat::Oma));
}

TEST(coats_differ_by_who_wore_them)
{
    CHECK(coatOf(poliisi) == Coat::Poliisi);
    CHECK(coatOf(sotilas) == Coat::Sotilas);
    CHECK(coatOf(fsb) == Coat::Musta);
    CHECK(coatOf(kansalainen) == Coat::Siviili);
    CHECK(coatColour(Coat::Oma) == 'J' && coatColour(Coat::Poliisi) == 'B');
    // each other coat colours the whole sleeve its own way
    const auto police = armColours(true, Held(), 0, Coat::Poliisi), soldier = armColours(true, Held(), 0, Coat::Sotilas);
    CHECK(police != soldier);
    CHECK(armPicture(true, Held(), 0, Coat::Poliisi) != armPicture(true, Held(), 0, Coat::Sotilas));  // the cuff stripe
    // a coat off the ground is a civilian one; a soldier's drop is olive
    World w;
    w.player.stats.health = 3;
    w.level.addItem(new Takki(Coord{ 51, 50 }));
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.coat == Coat::Siviili);
    w.player.stats.health = 3;
    Takki olive(Coord{ 0, 0 }, Coat::Sotilas);
    CHECK(olive.interact(&w.player));
    CHECK(w.player.coat == Coat::Sotilas);
}

TEST(arm_mirror_swaps_slashes)
{
    CHECK(mirrorRow("ab/(<[{`") == "'}]>)\\ba");
    CHECK(mirrorRow("\\)>]}'xy") == "yx`{[<(/");
    const string row = " |~,/`\\(o) _ .";
    CHECK(mirrorRow(mirrorRow(row)) == row);
}

TEST(arm_blood_only_at_stage_three)
{
    int bloody = 0;
    for (int row = 0; row < ARM_H; ++row) {
        for (int stage = 0; stage < 3; ++stage) CHECK(!armRowBloody(stage, row));
        if (armRowBloody(3, row)) ++bloody;
    }
    CHECK(bloody > 0);
    const auto pic = armPicture(true, Held(), 3);
    bool specks = false;
    for (int row = 9; row < ARM_H; ++row)
        if (armRowBloody(3, row) && pic[row].find_first_of("',.") != string::npos) specks = true;
    CHECK(specks);
}

// ---- arms in the browser ----

// the frames a real web printer sends for the World, captured instead of written to stdout
static std::vector<string> webFramesOf(World& w, PrinterWeb& web, int n)
{
    std::vector<string> sent, frames;
    common::webCapture = &sent;
    for (int i = 0; i < n; ++i) web.print(&w.level);
    common::webCapture = nullptr;
    for (const auto& s : sent) if (contains(s, "\"t\":\"frame\"")) frames.push_back(s);
    return frames;
}

TEST(web_frame_says_when_the_ak_is_up)
{
    World w;
    PrinterWeb web;
    web.player = &w.player;
    web.mask = &w.mask;
    hold(w.player.inventory, Thing::None, Thing::None);
    w.player.inventory.back = loaded(Thing::Rynkky);
    auto frames = webFramesOf(w, web, 1);
    CHECK(frames.size() == 1 && !contains(frames[0], "\"ak\":1"));
    CHECK(w.player.shoulderAk());
    frames = webFramesOf(w, web, 1);
    CHECK(frames.size() == 1 && contains(frames[0], "\"ak\":1"));
}

TEST(web_frame_sends_arms_once_per_change)
{
    World w;
    PrinterWeb web;
    web.player = &w.player;
    web.mask = &w.mask;
    hold(w.player.inventory, Thing::Vodka, Thing::Lenkki);
    w.player.inventory.pocket = Inventory::heldOf(Thing::Lipas);

    auto frames = webFramesOf(w, web, 2);
    CHECK(frames.size() == 2);
    CHECK(contains(frames[0], "\"arms\":[["));
    CHECK(contains(frames[0], "\"armc\":[["));
    CHECK(!contains(frames[1], "\"arms\""));     // nothing changed: not sent again
    CHECK(!contains(frames[1], "\"armc\""));
    // the hands are read from their captions, the rest of the slots from the list
    CHECK(contains(frames[1], "\"slots\":[[\"TASKU\",\"LIPAS\"]"));
    CHECK(!contains(frames[1], "\"inv\""));
    CHECK(contains(frames[1], "\"hands\":[\"VODKA 6/6\",\"MAKKARA\"]"));

    // a sip lowers the liquid: the pictures go again
    w.player.inventory.left.fill = 5;
    frames = webFramesOf(w, web, 2);
    CHECK(frames.size() == 2);
    CHECK(contains(frames[0], "\"arms\":[["));
    CHECK(!contains(frames[1], "\"arms\""));

    // torn sleeves at stage 3: the blood comes as R cells in the colour masks
    w.player.stats.health = 1;
    frames = webFramesOf(w, web, 1);
    CHECK(frames.size() == 1);
    const size_t at = frames.empty() ? string::npos : frames[0].find("\"armc\":");
    CHECK(at != string::npos && frames[0].find('R', at) != string::npos);
}

TEST(arm_colours_match_picture_shape)
{
    for (Coat coat : { Coat::Oma, Coat::Poliisi, Coat::Sotilas, Coat::Musta, Coat::Siviili })
    for (const Held& h : allHeldVariants())
        for (int stage = 0; stage <= 3; ++stage)
            for (bool left : { true, false }) {
                const auto pic = armPicture(left, h, stage, coat), mask = armColours(left, h, stage, coat);
                CHECK(mask.size() == pic.size());
                for (size_t r = 0; r < pic.size() && r < mask.size(); ++r) {
                    CHECK(mask[r].size() == pic[r].size());
                    for (size_t c = 0; c < pic[r].size() && c < mask[r].size(); ++c) {
                        if (pic[r][c] != ' ' && mask[r][c] == ' ') {
                            CHECK(false);
                            std::cerr << "    uncoloured '" << pic[r][c] << "' row " << r << " col " << c
                                      << " thing " << (int)h.what << " stage " << stage << (left ? " left" : " right") << "\n";
                        }
                        CHECK(string(" JSGVALCMWRYKBODN").find(mask[r][c]) != string::npos);
                    }
                }
            }
    // the liquid is vodka or beer by its colour, an empty bottle has none
    auto cellsOf = [](const std::vector<string>& pic, const std::vector<string>& mask, char liquid, char colour) {
        int n = 0, wrong = 0;
        for (size_t r = 0; r < pic.size(); ++r)
            for (size_t c = 0; c < pic[r].size(); ++c)
                if (pic[r][c] == liquid) (mask[r][c] == colour ? n : wrong)++;
        return wrong ? -1 : n;
    };
    for (bool left : { true, false }) {
        const Held vodka = heldOf(Thing::Vodka, 4), beer = heldOf(Thing::Kalja), empty = heldOf(Thing::Vodka, 0);
        CHECK(cellsOf(armPicture(left, vodka, 0), armColours(left, vodka, 0), '#', 'V') > 0);
        CHECK(cellsOf(armPicture(left, vodka, 0), armColours(left, vodka, 0), '~', 'V') == 6);
        CHECK(cellsOf(armPicture(left, beer, 0), armColours(left, beer, 0), '#', 'A') > 0);
        for (const auto& r : armColours(left, beer, 0)) CHECK(r.find('V') == string::npos);
        for (const auto& r : armColours(left, empty, 0)) CHECK(r.find('V') == string::npos);
        // mirroring keeps the letters: the label stays L, the sleeve J
        const auto vm = armColours(left, vodka, 0);
        CHECK(vm[0].find('J') != string::npos && vm[0].find_first_not_of(" J") == string::npos);
        CHECK(cellsOf(armPicture(left, heldOf(Thing::Vodka, 6), 0), armColours(left, heldOf(Thing::Vodka, 6), 0), 'K', 'L') == 1);
    }
    // stage 3 blood is R, and only there
    for (int stage = 0; stage <= 3; ++stage) {
        bool red = false;
        for (const auto& r : armColours(true, Held(), stage)) if (r.find('R') != string::npos) red = true;
        CHECK(red == (stage == 3));
    }
    // the rifle: wood and metal on both sides
    for (bool left : { true, false }) {
        string all;
        for (const auto& r : armColours(left, heldOf(Thing::Rynkky), 0)) all += r;
        CHECK(all.find('W') != string::npos && all.find('M') != string::npos);
    }
}

TEST(arm_art_rows_fit_before_padding)
{
    // every picture is built through rowsOf, which asserts no art row is wider than ARM_W:
    // build all of them, the rifle in both hands included
    for (const Held& h : allHeldVariants())
        for (int stage = 0; stage <= 3; ++stage)
            for (bool left : { true, false }) CHECK((int)armPicture(left, h, stage).size() == ARM_H);
    // the rifle grip's fist row keeps its closing edge
    const auto grip = armPicture(false, heldOf(Thing::Rynkky), 0);
    bool closed = false;
    for (const auto& r : grip) if (contains(r, "(__(__(__)|")) closed = true;
    CHECK(closed);
}

TEST(arm_rifle_magazine_sticks_out_to_the_map_side)
{
    // the right hand holds the grip: the long magazine reaches the map-side edge (column 0) on
    // the rows under the fist, in gun metal, and bends down towards the muzzle at its tip
    const auto pic = armPicture(false, heldOf(Thing::Rynkky), 0), mask = armColours(false, heldOf(Thing::Rynkky), 0);
    int reach = 0;
    for (int row = 13; row < ARM_H; ++row) if (pic[row][0] != ' ' && mask[row][0] == 'M') ++reach;
    CHECK(reach >= 3);
    CHECK(contains(pic[14], "_____|"));    // it leaves the receiver level
    CHECK(pic[17].substr(0, 3) == "\\_/"); // and ends in a rounded tip
    // the stock rises in wood along the outer side of the forearm
    for (int row = 9; row <= 12; ++row) CHECK(mask[row].find('W') != string::npos && mask[row].find('W') > 9);
    // the left hand holds the wooden handguard, the metal barrel below it
    const auto lm = armColours(true, heldOf(Thing::Rynkky), 0);
    CHECK(lm[13].find('W') != string::npos && lm[20].find('M') != string::npos);
    CHECK(lm[13].find('M') == string::npos);
}

// ---- passers-by comment on the drunk ----

TEST(drunk_stare_gets_a_comment)
{
    std::srand(7);
    World w;
    w.mask.enable();
    auto* mummo = w.spawn(new Mummo(Coord{ 53, 50 }));
    w.player.stats.promilles = 20;
    bool said = false;
    for (int i = 0; i < 200 && !said; ++i) {
        w.mask.init();
        w.player.updateTurn();
        said = w.said(mummo, "katsoo kuin");
    }
    CHECK(said);
}

TEST(sober_stare_gets_no_comment)
{
    std::srand(7);
    World w;
    w.mask.enable();
    auto* mummo = w.spawn(new Mummo(Coord{ 53, 50 }));
    w.player.stats.promilles = 5;
    bool said = false;
    for (int i = 0; i < 200; ++i) {
        w.mask.init();
        w.player.updateTurn();
        if (w.said(mummo, "katsoo kuin")) said = true;
    }
    CHECK(!said);
}

TEST(stagger_comment_from_onlooker)
{
    std::srand(7);
    World w;
    auto* mummo = w.spawn(new Mummo(Coord{ 53, 50 }));
    w.player.stats.promilles = 20;
    bool said = false;
    for (int i = 0; i < 100 && !said; ++i) {
        w.player.coord = Coord{ 50, 50 };
        w.player.stagger(&w.level);
        said = w.said(mummo, "Anttilassa");
    }
    CHECK(said);
}

// a drunk player staring at a granny; returns the turns on which she spoke
static std::vector<uint32_t> stareTurns(World& w, Mummo* m, int turns, int resetAt = -1)
{
    std::vector<uint32_t> out;
    for (int i = 0; i < turns; ++i) {
        if (i == resetAt) w.player.onNewLevel();
        w.level.animations.clear();
        w.mask.init();
        w.player.updateTurn();
        if (w.said(m, "katsoo kuin")) out.push_back(w.player.stats.turn);
    }
    return out;
}

TEST(stare_comment_waits_40_turns)
{
    std::srand(3);
    World w;
    w.mask.enable();
    auto* mummo = w.spawn(new Mummo(Coord{ 53, 50 }));
    w.player.stats.promilles = 30;
    const auto t = stareTurns(w, mummo, 120);
    CHECK(t.size() >= 2);
    for (size_t i = 1; i < t.size(); ++i) CHECK(t[i] - t[i - 1] >= 40);
}

TEST(stare_limiter_resets_on_new_level)
{
    std::srand(3);
    World w;
    w.mask.enable();
    auto* mummo = w.spawn(new Mummo(Coord{ 53, 50 }));
    w.player.stats.promilles = 30;
    // pretend a comment was just made: nothing within 40 turns
    w.player.stats.turn = 100;
    w.player.lastStareCommentTurn = 100;
    CHECK(stareTurns(w, mummo, 30).empty());
    // a new level: allowed right away (10 % per turn, so within a few dozen tries)
    w.player.onNewLevel();
    CHECK(!stareTurns(w, mummo, 39).empty());
}

// ---- the front ----

TEST(three_war_levels_lead_back_to_moscow)
{
    GameState gs;
    gs.sendToWar();
    CHECK(gs.phase == GamePhase::War && gs.warStage == 1);
    gs.advanceLevel();
    gs.advanceLevel();
    CHECK(gs.phase == GamePhase::War && gs.warStage == 3);
    CHECK(!gs.hasBeenToWar);
    gs.advanceLevel();
    CHECK(gs.phase == GamePhase::ReturnMoscow);
    CHECK(gs.hasBeenToWar);
    CHECK(gs.moscowStage == 1);
}

TEST(rear_opens_after_a_shift_at_the_front)
{
    World w(Coord{ 6, 153 });
    w.level.isWarLevel = true;
    for (uint32_t i = 1; i < Level::WAR_SHIFT_TICKS; ++i) w.level.warTick();
    CHECK(!w.level.rearOpen);
    CHECK(!w.level.reachedRear(Coord{ 0, 153 }));
    CHECK(w.printer.saw("Vuorovaihto tulee pian"));
    w.level.warTick();
    CHECK(w.level.rearOpen);
    CHECK(w.printer.saw("VUOROVAIHTO"));
    CHECK(w.level.reachedRear(Coord{ Level::REAR_X, 153 }));
    CHECK(!w.level.reachedRear(Coord{ 6, 153 }));
}

TEST(no_shift_change_in_moscow)
{
    World w;
    for (uint32_t i = 0; i < Level::WAR_SHIFT_TICKS * 2; ++i) w.level.warTick();
    CHECK(!w.level.rearOpen);
    CHECK(!w.level.reachedRear(Coord{ 0, 50 }));
}

// ---- final review fixes: dropping, reloading from hand, corpses, the thief's loot ----

static Item* firstItem(const Level& level, ItemType t)
{
    for (auto i : level.items) if (i->type == t) return i;
    return nullptr;
}

TEST(two_pistols_one_can_be_dropped)
{
    World w;
    auto& inv = w.player.inventory;
    inv.left = loaded(Thing::Pistooli);
    inv.right = loaded(Thing::Pistooli);
    CHECK(inv.freeHands() == 0);
    feedKeys("o");                                  // the right hand
    CHECK(w.player.throwItem());
    CHECK(inv.right.empty());
    CHECK(inv.left.what == Thing::Pistooli);
    CHECK(itemAt(w.level, EPistooli));              // at your feet, not flying
    CHECK(inv.give(Thing::Kalja));                  // a hand is free again
}

TEST(dropped_pistol_keeps_its_rounds)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::None, Thing::None);
    inv.right = loaded(Thing::Pistooli);
    inv.right.rounds = 3;
    CHECK(w.player.throwItem());                    // the only full hand: no question
    CHECK(inv.right.empty());
    Item* gun = firstItem(w.level, EPistooli);
    CHECK(gun != nullptr);
    if (!gun) return;
    CHECK(gun->coord.distance(w.player.coord) <= 1);
    w.player.coord = Coord{ gun->coord.x - 1, gun->coord.y };
    w.player.move(DirectionNS::right, &w.level);
    Held* h = inv.inHand(Thing::Pistooli);
    CHECK(h && h->rounds == 3);                     // the same gun, not a fresh one
    CHECK(!itemAt(w.level, EPistooli));

    // a magazine is dropped too, a rifle in hand as well
    World v;
    hold(v.player.inventory, Thing::Lipas, Thing::None);
    CHECK(v.player.throwItem());
    CHECK(v.player.inventory.left.empty());
    CHECK(itemAt(v.level, ELipas));
}

TEST(reload_from_hand_held_lipas)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::Lipas, Thing::None);
    inv.right = Inventory::heldOf(Thing::Pistooli);   // no rounds
    CHECK(inv.pocket.empty());
    CHECK(w.player.reload(Thing::Pistooli));
    CHECK(inv.right.rounds == MAKAROV_ROUNDS);
    CHECK(inv.left.empty());
    CHECK(w.player.readyWeapon() == Thing::Pistooli);

    // the pocket's magazine goes first
    inv.right.rounds = 0;
    inv.left = Inventory::heldOf(Thing::Lipas);
    inv.pocket = Inventory::heldOf(Thing::Lipas);
    CHECK(w.player.reload());
    CHECK(inv.pocket.empty());
    CHECK(inv.left.what == Thing::Lipas);

    // a spare in hand counts for Space too
    inv.right.rounds = 0;
    CHECK(w.player.readyWeapon() == Thing::Pistooli);
    CHECK(w.player.shootAt(0.0f));
    CHECK(inv.left.empty());
}

TEST(npc_walks_over_corpse)
{
    World w(Coord{ 10, 10 });
    auto* m = w.spawn(new Mummo(Coord{ 60, 60 }));
    w.level.addItem(new Ruumis(Coord{ 61, 60 }));
    CHECK(!w.level.hit(Coord{ 61, 60 }));
    std::string msg;
    m->move(DirectionNS::right, msg);
    CHECK(m->coord == (Coord{ 61, 60 }));
    w.level.addItem(new Kivi(Coord{ 62, 60 }));
    m->move(DirectionNS::right, msg);
    CHECK(m->coord == (Coord{ 61, 60 }));            // other things still block
}

TEST(a_caught_thief_gives_back_the_same_bottle)
{
    World w;
    auto* j = w.spawn(new Juoppo(Coord{ 51, 50 }));
    Held vodka = Inventory::heldOf(Thing::Vodka);
    vodka.fill = 2;
    vodka.pissSeller = "Igor";
    j->stolenItems = { vodka };
    hold(w.player.inventory, Thing::None, Thing::None);
    w.player.move(DirectionNS::right, &w.level);
    Held* h = w.player.inventory.find(Thing::Vodka);
    CHECK(h && h->fill == 2 && h->pissSeller == "Igor");
    CHECK(w.printer.saw("vodkan"));
}

TEST(cops_who_see_a_coat_stripped_chase_and_do_not_brawl)
{
    World w;
    w.player.stats.health = 2;
    auto* corpse = new Ruumis(Coord{ 51, 50 });
    w.level.addItem(corpse);
    auto* cop = w.spawn(new Cop(Coord{ 55, 50 }));
    auto* passer = w.spawn(new Kansalainen(Coord{ 51, 53 }));
    CHECK(w.player.stripCorpseCoat(corpse));
    CHECK(cop->chaseTarget == &w.player);
    CHECK(cop->brawlActions == 0);
    CHECK(passer->brawlActions > 0);
}

TEST(coat_on_the_ground_stays_at_full_health)
{
    World w;
    w.player.stats.health = PlayerNS::INITIAL_HP;
    w.level.addItem(new Takki(Coord{ 51, 50 }));
    w.player.move(DirectionNS::right, &w.level);
    CHECK(itemAt(w.level, ETakki));
    CHECK(w.printer.saw("Et tarvitse uutta takkia."));

    w.player.stats.health = 3;
    w.player.coord = Coord{ 50, 50 };
    w.player.move(DirectionNS::right, &w.level);
    CHECK(!itemAt(w.level, ETakki));
    CHECK(w.player.stats.health == PlayerNS::INITIAL_HP);
}

TEST(second_ak_says_the_back_is_taken)
{
    World w;
    hold(w.player.inventory, Thing::None, Thing::None);
    w.player.inventory.back = loaded(Thing::Rynkky);
    w.level.addItem(new Rynkky(Coord{ 51, 50 }));
    w.player.move(DirectionNS::right, &w.level);
    CHECK(itemAt(w.level, ERynkky));
    CHECK(w.printer.saw("Sel\xC3\xA4ss\xC3\xA4 on jo rynkky."));
    CHECK(!w.printer.saw(HINT_HANDS_FULL));
}

TEST(pistol_hint_says_hands_full_without_a_bottle)
{
    World w;
    auto& inv = w.player.inventory;
    inv.hasBelt = true;
    hold(inv, Thing::Lenkki, Thing::Kivi);
    inv.belt = loaded(Thing::Pistooli);
    CHECK(!w.player.drawPistol());
    CHECK(w.printer.saw(HINT_PISTOL_HANDS));
    CHECK(!w.printer.saw(HINT_PISTOL_BOTTLE));
}

TEST(e_eats_the_food_in_a_hand)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::Kalja, Thing::Lenkki);
    CHECK(w.player.foodHand(&w.level) == &inv.right);   // the only food: no question
    hold(inv, Thing::Kalja, Thing::Kivi);
    CHECK(w.player.foodHand(&w.level) == nullptr);
    hold(inv, Thing::Omppo, Thing::Lenkki);
    feedKeys("v");
    CHECK(w.player.foodHand(&w.level) == &inv.left);    // both: asked
    CHECK(w.printer.saw("Kummalla"));
}

TEST(empty_hands_throw_from_the_pocket)
{
    World w;
    auto& inv = w.player.inventory;
    hold(inv, Thing::None, Thing::None);
    inv.pocket = Inventory::heldOf(Thing::Veitsi);
    feedKeys("s");                                    // down
    CHECK(w.player.throwItem());
    CHECK(inv.pocket.empty());
}

// ---- film-style captions ----

static bool captioned(const string& part)
{
    for (const auto& line : Subtitles::history()) if (contains(line, part)) return true;
    return false;
}

TEST(caption_shows_for_a_moment_and_goes)
{
    Subtitles::reset();
    CHECK(Subtitles::say("Testi."));
    CHECK(Subtitles::current() == "Testi.");
    Subtitles::tick(Subtitles::SHOW_MS - 50);
    CHECK(Subtitles::current() == "Testi.");
    Subtitles::tick(100);
    CHECK(Subtitles::current().empty());
}

TEST(caption_of_the_same_kind_waits_for_its_cooldown)
{
    Subtitles::reset();
    CHECK(Subtitles::say("Yksi", "punch", 1500));
    CHECK(!Subtitles::say("Kaksi", "punch", 1500));
    Subtitles::tick(1600);
    CHECK(Subtitles::say("Kolme", "punch", 1500));
    CHECK(Subtitles::current() == "Kolme");
}

TEST(caption_when_throwing)
{
    World w;
    Subtitles::reset();
    w.player.executeThrowAt(6, 0.0f);   // a stone
    CHECK(captioned("KIVI"));
    Subtitles::reset();
    w.player.executeThrowAt(1, 0.0f);   // a beer bottle
    CHECK(captioned("Pullo lent"));
}

TEST(caption_when_punching_and_when_hit)
{
    World w;
    common::realtimeEnabled = false;
    Subtitles::reset();
    auto* m = w.spawn(new Myyja(Coord{ 51, 50 }, SellerWeapon::Veitsi));
    m->stats.health = 1;
    w.player.move(DirectionNS::right, &w.level);
    CHECK(m->stats.health == 0);
    CHECK(captioned("Myyj") && captioned("kaatuu"));

    Subtitles::reset();
    {
        HurtBy who(&w.level, "Politsija");
        w.player.damage(2);
    }
    CHECK(captioned("Saat osuman: Politsija"));
    common::realtimeEnabled = true;
}

TEST(caption_when_meeting_the_militia_once_a_minute)
{
    World w;
    Subtitles::reset();
    w.spawn(new Cop(Coord{ 53, 50 }));
    Subtitles::noticePeople(w.level, w.player, nullptr);
    CHECK(contains(Subtitles::current(), "Miliisi"));
    Subtitles::tick(Subtitles::SHOW_MS + 10);
    const size_t shown = Subtitles::history().size();
    Subtitles::noticePeople(w.level, w.player, nullptr);
    CHECK(Subtitles::history().size() == shown);       // not again so soon
    Subtitles::tick(61000);
    Subtitles::noticePeople(w.level, w.player, nullptr);
    CHECK(Subtitles::history().size() == shown + 1);

    // a soldier far away gets no caption
    World far;
    Subtitles::reset();
    far.spawn(new Sotilas(Coord{ 70, 50 }));
    Subtitles::noticePeople(far.level, far.player, nullptr);
    CHECK(Subtitles::history().empty());
    CHECK(!Subtitles::meetingLine(PersonType::sotilas).empty());
    CHECK(!Subtitles::meetingLine(PersonType::fsb).empty());
    CHECK(!Subtitles::meetingLine(PersonType::myyja).empty());
    CHECK(!Subtitles::meetingLine(PersonType::varas).empty());
}

TEST(caption_on_the_way_to_the_front_and_the_rifle)
{
    Subtitles::reset();
    Subtitles::toTheFront(Subtitles::WarRoute::Draft);
    CHECK(contains(Subtitles::current(), "Kutsunta"));
    Subtitles::toTheFront(Subtitles::WarRoute::Court);
    CHECK(contains(Subtitles::current(), "Oikeuden tuomio"));
    Subtitles::toTheFront(Subtitles::WarRoute::Fsb);
    CHECK(contains(Subtitles::current(), "FSB"));

    World w;
    w.level.isWarLevel = true;
    Subtitles::reset();
    w.player.issueRifle();
    CHECK(contains(Subtitles::current(), "AK"));
}

TEST(caption_for_drones_buzzing_at_the_front)
{
    World w(Coord{ 100, 100 });
    w.level.isWarLevel = true;
    w.spawn(new Sotilas(Coord{ 130, 110 }));
    Subtitles::reset();
    for (int i = 0; i < 200; ++i) fpvTick(w.level);
    CHECK(captioned("FPV-droonien"));
    size_t buzz = 0;
    for (const auto& line : Subtitles::history()) if (contains(line, "FPV-droonien")) ++buzz;
    CHECK(buzz == 1);                                   // the clock did not move: once
}

TEST(caption_queue_explains_the_hands_one_line_after_another)
{
    Subtitles::reset();
    const auto lines = Subtitles::handsTutorial(InputNS::HANDS_INTRO_KEY);
    CHECK(lines.size() >= 4);
    for (const auto& l : lines) Subtitles::queue(l, 1000);
    CHECK(Subtitles::current() == lines[0]);
    Subtitles::say("Heit\xC3\xA4t: KIVI.");                     // an action goes first
    Subtitles::tick(Subtitles::SHOW_MS);
    CHECK(Subtitles::current() == lines[1]);
    for (size_t i = 0; i < lines.size(); ++i) Subtitles::tick(1000);
    CHECK(Subtitles::history().size() == lines.size() + 1);
    CHECK(captioned("k\xC3\xA4tesi") && captioned("Hihan kunto on terveytesi") && captioned("Paina H"));
}

// ---- the hands walk-through ----

TEST(hands_intro_key_is_free_and_opens_the_intro)
{
    CHECK(InputNS::HANDS_INTRO_KEY == 'h');
    feedKeys(string(1, InputNS::HANDS_INTRO_KEY));
    CHECK(InputNS::Input::getInput() == InputNS::handsIntro);
    feedKeys("i");
    CHECK(InputNS::Input::getInput() == InputNS::up);   // i still walks up
}

TEST(hands_intro_shows_inventory_pickup_damage_captions_and_rifle)
{
    Subtitles::reset();
    HandsIntroNS::Show show;
    CHECK(show.chapter() == HandsIntroNS::Chapter::Inventory);
    CHECK(show.render().size() == common::VIEWPORT_H);
    bool slotsFilled = false, picked = false, torn = false, bloody = false, ak = false;
    int lastStage = 0;
    bool stagesOnlyWorsen = true;
    while (!show.over()) {
        show.tick(50);
        Subtitles::tick(50);
        const auto& inv = show.demo.inventory;
        const auto ch = show.chapter();
        if (ch == HandsIntroNS::Chapter::Inventory && !inv.left.empty() && !inv.right.empty() && !inv.pocket.empty() && inv.back.what == Thing::Rynkky) slotsFilled = true;
        if (ch == HandsIntroNS::Chapter::Pickup && inv.right.what == Thing::Kalja && inv.left.what == Thing::Kivi) picked = true;
        if (ch == HandsIntroNS::Chapter::Damage) {
            const int stage = sleeveStage(show.demo.stats.health, show.demo.usedCoat);
            if (stage < lastStage) stagesOnlyWorsen = false;
            lastStage = stage;
            if (stage == 1) torn = true;
            if (stage == 3) bloody = true;
        }
        if (ch == HandsIntroNS::Chapter::Rifle && inv.akRaised && inv.left.empty() && inv.right.empty() && inv.back.what == Thing::Rynkky) ak = true;
    }
    CHECK(slotsFilled && picked && torn && bloody && stagesOnlyWorsen && ak);
    CHECK(captioned("inventaariosi") && captioned("Kalja oikeaan") && captioned("Hiha") && captioned("tekstitys") && captioned("AK"));
}

// remembers what the show looked like on each frame
class IntroPrinter : public RecordingPrinter {
public:
    std::vector<string> titles;
    std::vector<Thing> rightHands;
    std::vector<bool> raised;
    std::vector<string> captions;
    void print(Level*) override {
        titles.push_back(scene && !scene->empty() ? (*scene)[1][0].first + sceneTitle : "");
        rightHands.push_back(player->inventory.right.what);
        raised.push_back(player->inventory.akRaised);
        captions.push_back(Subtitles::current());
    }
};

TEST(hands_intro_runs_over_the_map_and_gives_the_game_back)
{
    World w;
    IntroPrinter printer;
    printer.player = &w.player;
    w.level.printer = &printer;
    Subtitles::reset();
    hold(w.player.inventory, Thing::Lenkki, Thing::None);
    // one key a frame: each key skips to the next part, the last one ends the show
    feedKeys("xxxxxx");
    HandsIntroNS::run(&printer, &w.level);
    CHECK(printer.titles.size() == 5);                 // the five parts after the first, drawn once each
    CHECK(!printer.titles.empty() && contains(printer.titles[0], "K\xC3\x84""DET"));
    CHECK(std::find(printer.raised.begin(), printer.raised.end(), true) != printer.raised.end());
    CHECK(std::find(printer.rightHands.begin(), printer.rightHands.end(), Thing::Kalja) != printer.rightHands.end());
    bool anyCaption = false;
    for (const auto& c : printer.captions) if (!c.empty()) anyCaption = true;
    CHECK(anyCaption);
    // the game's own drunk, map and message are back
    CHECK(printer.player == &w.player);
    CHECK(printer.scene == nullptr);
    CHECK(printer.msg.empty());
    CHECK(w.player.inventory.left.what == Thing::Lenkki);
    CHECK(!w.player.inventory.akRaised);

    // Esc closes it at once
    IntroPrinter quick;
    quick.player = &w.player;
    feedKeys("\x1b");
    HandsIntroNS::run(&quick, &w.level);
    CHECK(quick.titles.empty());
    CHECK(quick.player == &w.player && quick.scene == nullptr);
}

// ---- trenches at the front ----

static bool ownTrenchReachesRear(const Level& l, const Coord& from)
{
    // flood through our own trench cells from `from` and see whether x = 0 is reached
    std::vector<uint8_t> seen(common::SIZEX * common::SIZEY, 0);
    std::vector<Coord> todo { from };
    while (!todo.empty()) {
        const Coord c = todo.back(); todo.pop_back();
        if (c.x >= common::SIZEX || c.y >= common::SIZEY || seen[c.y * common::SIZEX + c.x]) continue;
        if (!l.inOwnTrench(c)) continue;
        seen[c.y * common::SIZEX + c.x] = 1;
        if (c.x == 0) return true;
        todo.push_back(Coord{ c.x + 1, c.y });
        todo.push_back(Coord{ c.x - 1, c.y });
        todo.push_back(Coord{ c.x, c.y + 1 });
        todo.push_back(Coord{ c.x, c.y - 1 });
    }
    return false;
}

TEST(trenches_main_line_runs_the_whole_height)
{
    for (int seed = 1; seed <= 6; ++seed) {
        std::srand(seed);
        World w(Coord{ 6, 153 });
        w.level.isWarLevel = true;
        generateTrenches(w.level);
        bool everyRow = true;
        for (uint32_t y = 0; y < common::SIZEY; ++y) {
            bool row = false;
            for (uint32_t x = 6; x <= 22; ++x) row = row || w.level.inOwnTrench(Coord{ x, y });
            everyRow = everyRow && row;
        }
        CHECK(everyRow);
    }
}

TEST(trenches_a_communication_trench_reaches_the_rear)
{
    for (int seed = 1; seed <= 6; ++seed) {
        std::srand(seed);
        World w(Coord{ 6, 153 });
        w.level.isWarLevel = true;
        generateTrenches(w.level);
        const Coord start = nearestTrench(w.level, Coord{ common::PLAYER_START_X, common::PLAYER_START_Y }, Trench::Own);
        CHECK(w.level.inOwnTrench(start));
        CHECK(start.distance(Coord{ common::PLAYER_START_X, common::PLAYER_START_Y }) <= 10);
        CHECK(ownTrenchReachesRear(w.level, start));
    }
}

TEST(trenches_enemy_line_is_east)
{
    std::srand(7);
    World w(Coord{ 6, 153 });
    w.level.isWarLevel = true;
    generateTrenches(w.level);
    uint32_t enemy = 0, misplaced = 0;
    for (uint32_t y = 0; y < common::SIZEY; ++y)
        for (uint32_t x = 0; x < common::SIZEX; ++x)
            if (w.level.trenchAt(Coord{ x, y }) == Trench::Enemy) { ++enemy; if (x < 60 || x > 80) ++misplaced; }
    CHECK(enemy > common::SIZEY * 2);
    CHECK(misplaced == 0);
    // no man's land between them: craters and wire, but no trench of either side
    uint32_t craters = 0;
    for (uint32_t y = 0; y < common::SIZEY; ++y)
        for (uint32_t x = 30; x < 55; ++x) if (w.level.trenchAt(Coord{ x, y }) == Trench::Crater) ++craters;
    CHECK(craters > 0);
}

TEST(trenches_lookup_off_the_map_is_none)
{
    World w(Coord{ 6, 153 });
    CHECK(w.level.trenchAt(Coord{ 10, 10 }) == Trench::None);     // no layer outside the front
    w.level.isWarLevel = true;
    generateTrenches(w.level);
    CHECK(w.level.trenchAt(Coord{ common::SIZEX, 5 }) == Trench::None);
    CHECK(w.level.trenchAt(Coord{ 5, common::SIZEY + 3 }) == Trench::None);
    CHECK(!w.level.inCover(Coord{ 99999, 99999 }));
    CHECK(ownTrenchEastX(w.level, common::SIZEY + 1) == -1);
    CHECK(ownTrenchEastX(w.level, 100) >= 8);
}

TEST(trench_cell_draws_sandbags_and_dark_ground)
{
    World w(Coord{ 6, 153 });
    w.level.isWarLevel = true;
    for (uint32_t x = 10; x <= 12; ++x)
        for (uint32_t y = 20; y <= 30; ++y) w.level.setTrench(Coord{ x, y }, Trench::Own);
    w.level.setTrench(Coord{ 40, 40 }, Trench::Crater);
    w.level.setTrench(Coord{ 41, 41 }, Trench::Wire);
    string g; const char* col = "";
    CHECK(trenchCell(&w.level, Coord{ 11, 25 }, g, col));
    CHECK(contains(col, "48;5;"));                        // dug-out ground underneath
    CHECK(trenchCell(&w.level, Coord{ 10, 25 }, g, col));
    CHECK(g == "\xE2\x96\x92");                           // the edge: a row of sandbags
    CHECK(trenchCell(&w.level, Coord{ 40, 40 }, g, col) && g == "o");
    CHECK(trenchCell(&w.level, Coord{ 41, 41 }, g, col) && (g == "#" || g == "x"));
    CHECK(!trenchCell(&w.level, Coord{ 60, 60 }, g, col));
}

// ---- exposure and trench pressure ----

static World* trenchStrip(uint32_t px, uint32_t py)
{
    // a short trench along x 10..12, rows 10..60; the player wherever asked
    World* w = new World(Coord{ px, py });
    w->level.isWarLevel = true;
    w->level.warStage = 1;
    for (uint32_t y = 10; y <= 60; ++y)
        for (uint32_t x = 10; x <= 12; ++x) w->level.setTrench(Coord{ x, y }, Trench::Own);
    return w;
}

TEST(exposure_rises_outside_and_falls_in_the_trench)
{
    World* w = trenchStrip(30, 30);
    w->level.exposureTick();
    CHECK(w->player.exposure == 4);
    for (int i = 0; i < 40; ++i) w->level.exposureTick();
    CHECK(w->player.exposure == 100);                       // capped
    w->player.coord = Coord{ 11, 30 };
    w->level.exposureTick();
    CHECK(w->player.exposure == 98);
    for (int i = 0; i < 80; ++i) w->level.exposureTick();
    CHECK(w->player.exposure == 0);                         // floored
    w->level.setTrench(Coord{ 40, 40 }, Trench::Crater);
    w->player.coord = Coord{ 40, 40 };
    w->level.exposureTick();
    CHECK(w->player.exposure == 2);                         // a crater is half cover
    delete w;
}

TEST(trench_pressure_builds_when_standing_still_and_eases_when_moving)
{
    World* w = trenchStrip(11, 20);
    for (int i = 0; i < 10; ++i) w->level.exposureTick();
    CHECK(w->player.trenchPressure == 30);
    w->player.coord = Coord{ 11, 40 };                      // down the trench to another stretch
    w->level.exposureTick();
    CHECK(w->player.trenchPressure == 28);
    w->player.coord = Coord{ 30, 40 };                      // up and out: the pilots lose him
    for (int i = 0; i < 20; ++i) w->level.exposureTick();
    CHECK(w->player.trenchPressure == 0);
    delete w;
}

TEST(exposure_is_announced_once_per_rise)
{
    Subtitles::reset();
    World* w = trenchStrip(30, 30);
    for (int i = 0; i < 7; ++i) w->level.exposureTick();    // 28: past the first mark
    CHECK(contains(Subtitles::current(), "huomattu"));
    delete w;
}

// ---- Ukrainian riflemen in their trench ----

static Ukrainalainen* rifleman(World& w, Coord at)
{
    w.level.setTrench(at, Trench::Enemy);
    auto* u = w.spawn(new Ukrainalainen(at));
    u->firstMove = false;
    return u;
}

TEST(riflemen_never_shoot_a_player_in_the_trench)
{
    World* w = trenchStrip(11, 30);
    w->player.stats.health = 50;
    w->player.exposure = 100;           // even a man they have seen before is safe down there
    auto* u = rifleman(*w, Coord{ 60, 30 });
    for (int i = 0; i < 1000; ++i) u->npcAct();
    CHECK(w->player.stats.health == 50);
    delete w;
}

TEST(riflemen_shoot_a_player_in_the_open)
{
    World* w = trenchStrip(30, 30);
    common::realtimeEnabled = false;
    w->player.stats.health = 200;
    auto* u = rifleman(*w, Coord{ 50, 30 });
    for (int i = 0; i < 300; ++i) { w->player.exposure = 80; u->npcAct(); }
    CHECK(w->player.stats.health < 200);
    CHECK(w->printer.saw("Luoti viuhuu ohi") || w->printer.saw("osuu sinuun"));
    delete w;
}

TEST(lurking_rifleman_is_hidden_until_he_fires)
{
    World* w = trenchStrip(30, 30);
    w->player.stats.health = 200;
    auto* u = rifleman(*w, Coord{ 50, 30 });
    CHECK(u->hiddenFromView());
    bool shown = false;
    for (int i = 0; i < 300 && !shown; ++i) { w->player.exposure = 100; u->npcAct(); shown = !u->hiddenFromView(); }
    CHECK(shown);                       // the muzzle flash gives him away for a moment
    CHECK(!isRussianSoldier(w->level, u));
    delete w;
}

TEST(soldiers_at_the_front_stay_down_in_the_trench)
{
    World* w = trenchStrip(40, 40);
    auto* s = w->spawn(new Sotilas(Coord{ 11, 30 }));
    s->firstMove = false;
    for (int i = 0; i < 400; ++i) s->npcAct();
    CHECK(w->level.inOwnTrench(s->coord));
    delete w;
}

// ---- the drones' see-saw: the trench swarms with them, the open ground has the riflemen ----

TEST(fpv_player_share_is_high_in_the_trench_low_when_just_out)
{
    World* w = trenchStrip(11, 30);
    CHECK(fpvPlayerShare(w->level) == 50);
    w->player.trenchPressure = 80;
    CHECK(fpvPlayerShare(w->level) == 90);
    w->player.coord = Coord{ 30, 30 };
    w->player.exposure = 0;
    CHECK(fpvPlayerShare(w->level) == 10);  // the pilots are watching the trenches
    w->player.exposure = 100;
    CHECK(fpvPlayerShare(w->level) == 90);
    CHECK(fpvMax(w->level) == 7);
    World moscow;
    CHECK(fpvMax(moscow.level) == FPV_MAX);
    delete w;
}

TEST(fpv_drones_come_from_the_east)
{
    World w(Coord{ 100, 100 });
    w.level.isWarLevel = true;
    for (int i = 0; i < 300 && w.level.drones.empty(); ++i) fpvTick(w.level);
    CHECK(!w.level.drones.empty());
    bool east = true;
    for (const auto& d : w.level.drones) if (d.fpv) east = east && d.x > 100.0f;
    CHECK(east);
}

// ---- artillery (second and third front levels) and the meat assault ----

static bool animated(const Level& l, const string& text)
{
    for (auto a : l.animations) if (contains(a->text, text)) return true;
    return false;
}

TEST(no_artillery_on_the_first_front_level)
{
    World* w = trenchStrip(11, 30);
    w->player.stats.health = 1000;
    w->level.events = new EnvironmentEvents(&w->level, false);
    for (int i = 0; i < 300; ++i) { w->level.warTick(); w->level.events->update(); }
    CHECK(!w->printer.saw("TYKIST"));
    CHECK(!w->printer.saw("Ulvontaa"));
    delete w;
}

TEST(artillery_warns_before_it_lands)
{
    World* w = trenchStrip(11, 30);
    w->player.stats.health = 1000;
    w->level.warStage = 2;
    w->level.artilleryCooldown = 1;
    w->level.artilleryTick();
    CHECK(w->printer.saw("Ulvontaa"));
    CHECK(!animated(w->level, "KRUMP"));
    w->level.artilleryTick();
    CHECK(!animated(w->level, "KRUMP"));
    w->level.artilleryTick();
    CHECK(animated(w->level, "KRUMP"));
    delete w;
}

TEST(shell_burst_spares_the_trench_beyond_one_cell)
{
    World* w = trenchStrip(40, 60);
    auto* deep = w->spawn(new Sotilas(Coord{ 11, 30 }));     // in the trench, two cells off
    auto* close = w->spawn(new Sotilas(Coord{ 11, 31 }));    // in the trench, right beside it
    auto* open = w->spawn(new Sotilas(Coord{ 14, 32 }));     // up on the open ground
    w->level.shellBurst(Coord{ 11, 32 }, 3, 6, "tykist\xC3\xB6");
    CHECK(deep->stats.health == 15);
    CHECK(close->stats.health < 15);
    CHECK(open->stats.health < 15);
    // a hit leaves a crater in the open, but a trench stays a trench
    w->level.shellBurst(Coord{ 30, 30 }, 1, 1, "tykist\xC3\xB6");
    CHECK(w->level.trenchAt(Coord{ 30, 30 }) == Trench::Crater);
    CHECK(w->level.inOwnTrench(Coord{ 11, 32 }));
    delete w;
}

TEST(meat_assault_goes_over_the_top_to_the_east)
{
    World* w = trenchStrip(40, 60);
    auto* waiting = w->spawn(new Skinhead(Coord{ 11, 20 }));
    auto* charging = w->spawn(new Skinhead(Coord{ 11, 25 }));
    waiting->firstMove = charging->firstMove = false;
    charging->charging = true;
    w->player.coord = Coord{ 12, 20 };                      // right beside a comrade: no beating
    w->player.stats.health = 30;
    for (int i = 0; i < 40; ++i) { waiting->npcAct(); charging->npcAct(); }
    CHECK(w->level.inOwnTrench(waiting->coord));
    CHECK(charging->coord.x > 30);
    CHECK(w->player.stats.health == 30);
    delete w;
}

// ---- the third front level: barrage, the Bradleys, the officer who holds and our own guns ----

static World* holdWorld()
{
    std::srand(4242);
    World* w = new World(Coord{ 6, 153 });
    w->level.isWarLevel = true;
    w->level.warStage = 3;
    generateTrenches(w->level);
    w->player.coord = nearestTrench(w->level, w->player.coord, Trench::Own);
    w->player.stats.health = 100000;
    w->level.prepareHold();
    return w;
}

static size_t bradleys(const Level& l)
{
    size_t n = 0;
    for (auto v : l.vehicles) if (dynamic_cast<const Bradley*>(v)) ++n;
    return n;
}

TEST(third_level_barrage_then_assault)
{
    World* w = holdWorld();
    CHECK(w->level.holdOfficer != nullptr);
    CHECK(w->level.mgNests.size() >= 2);
    while (w->level.warTicks < 29) w->level.warTick();
    CHECK(w->level.assaultPhase == AssaultPhase::Quiet);
    w->level.warTick();
    CHECK(w->level.assaultPhase == AssaultPhase::Barrage);
    CHECK(w->printer.saw("TYKIST\xC3\x96KESKITYS"));
    while (w->level.warTicks < 55) w->level.warTick();
    CHECK(w->level.assaultPhase == AssaultPhase::Assault);
    CHECK(bradleys(w->level) >= 2 && bradleys(w->level) <= 3);
    delete w;
}

TEST(bradley_stops_short_of_the_trench_and_unloads)
{
    World* w = holdWorld();
    const uint32_t y = w->player.coord.y;
    auto* b = new Bradley(Coord{ 85, y }, bradleyStopX(w->level, y));
    w->level.addVehicle(b);
    for (int i = 0; i < 80; ++i) w->level.updateVehicles();
    const int gap = (int)b->coord.x - ownTrenchEastX(w->level, y);
    CHECK(gap >= 6 && gap <= 10);
    CHECK(b->unloaded);
    size_t assault = 0;
    for (auto p : w->level.persons)
        if (p->type == ukrainalainen && static_cast<Ukrainalainen*>(p)->mode == Ukrainalainen::Mode::Assault) ++assault;
    CHECK(assault >= 4 && assault <= 6);
    // a grenade beside it finishes it
    w->level.explode(Coord{ b->coord.x, b->coord.y + 2 }, 2, 3, nullptr);
    CHECK(b->wrecked);
    delete w;
}

TEST(rear_stays_shut_while_the_officer_lives)
{
    World* w = holdWorld();
    w->level.warTicks = 200;
    w->level.warTick();
    CHECK(!w->level.rearOpen);
    w->level.holdOfficer->stats.health = 0;
    w->level.warTick();
    CHECK(w->level.rearOpen);
    CHECK(w->printer.saw("Upseeri kaatui"));
    delete w;
}

TEST(own_machine_guns_fire_on_a_player_behind_the_officer)
{
    World* w = holdWorld();
    common::realtimeEnabled = false;    // no moment-to-react guard between hits (it is shared across tests)
    w->player.stats.health = 200;
    w->player.coord = Coord{ 2, w->level.holdOfficer->coord.y };    // well behind him, at the rear
    // the barrier stands from the start: otherwise the new man could just walk out before the barrage
    for (int i = 0; i < 200; ++i) w->level.barrierFireTick();
    CHECK(w->player.stats.health < 200);
    CHECK(w->printer.saw("konekiv"));
    delete w;
}

TEST(no_barrier_fire_once_the_officer_is_dead)
{
    World* w = holdWorld();
    w->player.stats.health = 200;
    w->player.coord = Coord{ 2, w->level.holdOfficer->coord.y };
    w->level.holdOfficer->stats.health = 0;
    w->level.assaultPhase = AssaultPhase::Assault;
    for (int i = 0; i < 200; ++i) w->level.barrierFireTick();
    CHECK(w->player.stats.health == 200);
    // nor while the player stays up at the line beside him
    World* v = holdWorld();
    v->player.stats.health = 200;
    v->player.coord = Coord{ v->level.holdOfficer->coord.x - 3, v->level.holdOfficer->coord.y };
    v->level.assaultPhase = AssaultPhase::Assault;
    for (int i = 0; i < 200; ++i) v->level.barrierFireTick();
    CHECK(v->player.stats.health == 200);
    delete w;
    delete v;
}

TEST(third_level_without_officer_opens_after_shift)
{
    World* w = holdWorld();
    w->level.holdOfficer = nullptr;            // he never made it up to the line
    while (w->level.warTicks < Level::WAR_SHIFT_TICKS + 1) w->level.warTick();
    CHECK(w->level.rearOpen);
    delete w;
}

TEST(a_new_man_starts_on_the_main_line_not_behind_it)
{
    for (int seed = 1; seed <= 6; ++seed) {
        std::srand(seed);
        World w(Coord{ 6, 153 });
        w.level.isWarLevel = true;
        generateTrenches(w.level);
        const uint32_t y = common::PLAYER_START_Y;
        const Coord start = nearestTrench(w.level, Coord{ (uint32_t)w.level.ownLineX[y], y }, Trench::Own);
        CHECK(w.level.inOwnTrench(start));
        CHECK(!w.level.behindTheLine(start));
    }
}

// ---- review fixes ----

TEST(officer_dies_before_the_barrage_and_the_assault_still_comes)
{
    World* w = holdWorld();
    while (w->level.warTicks < 5) w->level.warTick();
    w->level.holdOfficer->stats.health = 0;      // a drone found him in the trench
    w->level.cleanDead();
    while (w->level.warTicks < 60) w->level.warTick();
    CHECK(w->level.rearOpen);
    CHECK(w->level.assaultPhase == AssaultPhase::Assault);
    CHECK(bradleys(w->level) >= 2);
    delete w;
}

TEST(assault_man_at_the_west_edge_never_wraps_off_the_map)
{
    World w(Coord{ 50, 50 });
    w.level.isWarLevel = true;
    auto* s = w.spawn(new Sotilas(Coord{ 1, 20 }));
    s->stats.health = 100000;
    s->firstMove = false;
    auto* u = w.spawn(new Ukrainalainen(Coord{ 0, 20 }, Ukrainalainen::Mode::Assault));
    u->firstMove = false;
    bool onMap = true;
    for (int i = 0; i < 300 && onMap; ++i) { u->npcAct(); onMap = u->coord.x < common::SIZEX; s->coord = Coord{ u->coord.x + 1, u->coord.y }; }
    CHECK(onMap);
}

TEST(comrades_never_wander_behind_the_line)
{
    World* w = holdWorld();
    std::vector<Person*> men;
    for (int i = 0; i < 6; ++i) {
        const uint32_t y = 140 + i * 4;
        const Coord at = nearestTrench(w->level, Coord{ (uint32_t)w->level.ownLineX[y], y }, Trench::Own);
        Person* p = i % 2 ? (Person*)w->spawn(new Sotilas(at)) : (Person*)w->spawn(new Skinhead(at));
        if (i % 2) static_cast<Sotilas*>(p)->firstMove = false; else static_cast<Skinhead*>(p)->firstMove = false;
        men.push_back(p);
    }
    bool behind = false;
    for (int t = 0; t < 1500; ++t)
        for (auto p : men) { p->npcAct(); behind = behind || w->level.behindTheLine(p->coord); }
    CHECK(!behind);
    delete w;
}

// ---------- tracers: a streak of light for every shot ----------

static size_t braille(const string& s)
{
    // U+2800..U+28FF: E2 A0..A3 xx
    size_t n = 0;
    for (size_t i = 0; i + 2 < s.size(); ++i)
        if ((unsigned char)s[i] == 0xE2 && ((unsigned char)s[i + 1] & 0xFC) == 0xA0) ++n;
    return n;
}

TEST(tracer_flies_fast_and_stops_at_a_hit)
{
    World w;
    w.level.addTracer(Coord{ 10, 10 }, Coord{ 30, 10 }, true);
    CHECK(w.level.tracers.size() == 1);
    w.level.tracerFrame(50);
    float x = 0, y = 0;
    CHECK(tracerDot(w.level.tracers[0], 0, x, y));
    CHECK(x > 12.5f && x < 13.5f);             // 60 cells a second: three cells a frame
    CHECK(std::fabs(y - 10.0f) < 0.01f);
    float maxX = 0;
    for (int f = 0; f < 20 && !w.level.tracers.empty(); ++f) {
        for (uint32_t k = 0; k < TRACER_DOTS; ++k)
            if (tracerDot(w.level.tracers[0], k, x, y)) maxX = std::max(maxX, x);
        w.level.tracerFrame(50);
    }
    CHECK(maxX <= 30.01f);                      // never through the man it hit
    CHECK(maxX > 28.0f);
    CHECK(w.level.tracers.empty());             // and gone once the tail arrives
}

TEST(tracer_miss_flies_on_past_the_target)
{
    World w;
    w.level.addTracer(Coord{ 10, 10 }, Coord{ 30, 10 }, false);
    CHECK(w.level.tracers.size() == 1);
    CHECK(w.level.tracers[0].x1 >= 35.5f);
    CHECK(w.level.tracers[0].x1 <= 40.5f);
}

TEST(tracer_later_rounds_of_a_burst_wait_their_turn)
{
    World w;
    w.level.addTracer(Coord{ 10, 10 }, Coord{ 30, 10 }, true, TracerKind::Bullet, 120);
    w.level.tracerFrame(50);
    float x, y;
    CHECK(!tracerDot(w.level.tracers[0], 0, x, y));
    w.level.tracerFrame(50);
    w.level.tracerFrame(50);
    CHECK(tracerDot(w.level.tracers[0], 0, x, y));
}

TEST(tracer_none_without_animations_or_in_turns)
{
    World w;
    common::animsEnabled = false;
    w.level.addTracer(Coord{ 10, 10 }, Coord{ 30, 10 }, true);
    CHECK(w.level.tracers.empty());
    common::animsEnabled = true;
    common::realtimeEnabled = false;
    w.level.addTracer(Coord{ 10, 10 }, Coord{ 30, 10 }, true);
    CHECK(w.level.tracers.empty());
    common::realtimeEnabled = true;
}

TEST(tracer_colours_run_from_white_hot_to_dark_red)
{
    CHECK(tracerColour(0, TracerKind::Bullet) == 231);
    CHECK(tracerColour(TRACER_DOTS - 1, TracerKind::Bullet) == 88);
    for (uint32_t k = 0; k < TRACER_DOTS; ++k) CHECK(tracerColour(k, TracerKind::Bullet) != tracerColour(k + 1, TracerKind::Bullet));
}

TEST(tracer_from_every_fired_bullet)
{
    World w;
    Sotilas* s = w.spawn(new Sotilas(Coord{ 60, 50 }));
    w.level.fireBullet(s, 0.0f, 10, 1, 0);       // east, nobody there: flies its full range
    CHECK(w.level.tracers.size() == 1);
    CHECK(w.level.tracers[0].x0 > 59.5f && w.level.tracers[0].x0 < 60.5f);
    CHECK(w.level.tracers[0].x1 > 68.0f);
}

TEST(tracer_for_each_round_of_a_soldiers_burst)
{
    World w;
    Sotilas* s = w.spawn(new Sotilas(Coord{ 60, 50 }));
    string msg;
    const uint32_t before = s->ammo;
    s->fire(&w.player, msg);
    CHECK(w.level.tracers.size() == before - s->ammo);
    CHECK(w.level.tracers.size() >= 2);
}

TEST(tracer_from_ukrainian_rifles_and_bradley_cannon)
{
    World w;
    w.level.isWarLevel = true;
    Ukrainalainen* u = w.spawn(new Ukrainalainen(Coord{ 70, 50 }));
    string msg;
    u->snipe(&w.player, msg);
    CHECK(w.level.tracers.size() == 1);
    u->burst(&w.player, msg);
    CHECK(w.level.tracers.size() >= 3);
    w.level.tracers.clear();
    Bradley* b = new Bradley(Coord{ 58, 50 }, 58);
    b->level = &w.level;
    b->unloaded = true;
    b->gunCooldown = 1;
    w.level.vehicles.push_back(b);
    b->update();
    CHECK(!w.level.tracers.empty());
    CHECK(w.level.tracers.back().kind == TracerKind::Cannon);
}

TEST(tracer_drawn_as_tiny_dots_in_the_terminal)
{
    World w;
    w.level.updateCamera();
    w.level.addTracer(Coord{ 40, 50 }, Coord{ 49, 50 }, true);
    w.level.tracerFrame(50);
    string row;
    for (const auto& cell : w.printer.rowCells(&w.level, 50 - (uint32_t)w.level.cameraY)) row += cell.first + cell.second;
    CHECK(braille(row) >= 1);
    CHECK(contains(row, "38;5;231"));             // the white-hot head
}

TEST(tracer_left_to_the_browser_page_to_draw)
{
    World w;
    w.level.updateCamera();
    w.printer.tracersAsOverlay = true;
    w.level.addTracer(Coord{ 40, 50 }, Coord{ 49, 50 }, true);
    w.level.tracerFrame(50);
    string row;
    for (const auto& cell : w.printer.rowCells(&w.level, 50 - (uint32_t)w.level.cameraY)) row += cell.first;
    CHECK(braille(row) == 0);
}

// ---- Melee: what is in the hand hits harder ----

// one step into a rat with the given hands; returns the damage dealt
static uint32_t bumpRatAgain(World& w)
{
    w.player.stats.health = 20;   // the rat bites back
    w.player.coord = Coord{ 50, 50 };
    auto* rat = w.spawn(new Rotta(Coord{ 51, 50 }));
    rat->stats.health = 100;
    w.player.move(DirectionNS::right, &w.level);
    const uint32_t dealt = 100 - rat->stats.health;
    rat->stats.health = 0;
    w.level.cleanDead();
    return dealt;
}

static uint32_t bumpRat(World& w, Thing l, Thing r)
{
    hold(w.player.inventory, l, r);
    return bumpRatAgain(w);
}

TEST(melee_bare_fists_hit_as_before)
{
    World w;
    common::realtimeEnabled = false;
    CHECK(bumpRat(w, Thing::None, Thing::None) == 1);
}

TEST(melee_stone_baton_and_rifle_butt_hit_harder_in_that_order)
{
    World w;
    common::realtimeEnabled = false;
    const uint32_t fist = bumpRat(w, Thing::None, Thing::Lenkki);
    const uint32_t stone = bumpRat(w, Thing::Kivi, Thing::None);
    const uint32_t baton = bumpRat(w, Thing::None, Thing::Pamppu);
    w.player.inventory.back = Inventory::fresh(Thing::Rynkky);
    w.player.inventory.akRaised = true;
    const uint32_t butt = bumpRat(w, Thing::None, Thing::None);
    CHECK(fist == 1);
    CHECK(stone > fist);
    CHECK(baton >= stone);
    CHECK(butt > stone);
    CHECK(butt >= baton);
    // the stone and the baton stay in the hand
    w.player.inventory.akRaised = false;
    bumpRat(w, Thing::Kivi, Thing::Pamppu);
    CHECK(w.player.inventory.left.what == Thing::Kivi);
    CHECK(w.player.inventory.right.what == Thing::Pamppu);
}

TEST(melee_the_best_thing_in_either_hand_is_used)
{
    World w;
    common::realtimeEnabled = false;
    const uint32_t right = bumpRat(w, Thing::Lenkki, Thing::Kivi);
    const uint32_t left = bumpRat(w, Thing::Kivi, Thing::Lenkki);
    CHECK(right == left);
    CHECK(right > 1);
}

TEST(melee_a_bottle_often_breaks_spills_and_is_gone)
{
    World w;
    common::realtimeEnabled = false;
    int breaks = 0, whole = 0;
    uint32_t brokenDmg = 0, wholeDmg = 0;
    for (int i = 0; i < 60; ++i) {
        w.printer.log.clear();
        const uint32_t d = bumpRat(w, Thing::Kalja, Thing::None);
        if (w.player.inventory.left.empty()) {
            ++breaks;
            brokenDmg = d;
            CHECK(w.printer.saw("valuu"));
        } else {
            ++whole;
            wholeDmg = d;
            CHECK(w.player.inventory.left.what == Thing::Kalja);
        }
    }
    CHECK(breaks > 15);   // often, not always
    CHECK(whole > 5);
    CHECK(brokenDmg > wholeDmg);
    CHECK(brokenDmg > 3);
    CHECK(itemAt(w.level, ESirpaleet));
}

TEST(melee_an_empty_bottle_breaks_without_spilling)
{
    World w;
    common::realtimeEnabled = false;
    bool broke = false;
    for (int i = 0; i < 60 && !broke; ++i) {
        w.printer.log.clear();
        bumpRat(w, Thing::None, Thing::Tyhja);
        broke = w.player.inventory.right.empty();
        if (broke) CHECK(!w.printer.saw("valuu"));
    }
    CHECK(broke);
}

TEST(melee_no_weapon_swing_at_a_harmless_passer_by)
{
    World w;
    common::realtimeEnabled = false;
    hold(w.player.inventory, Thing::Kivi, Thing::Pamppu);
    auto* k = w.spawn(new Kansalainen(Coord{ 51, 50 }));
    const uint32_t before = k->stats.health;
    w.player.move(DirectionNS::right, &w.level);
    CHECK(k->stats.health == before);
}

// ---------- anti-aircraft guns at the front ----------

static World* frontWorld(uint32_t stage)
{
    World* w = holdWorld();
    w->level.warStage = stage;
    populateAAGuns(w->level, 1);
    return w;
}

static std::vector<AAGun*> guns(const Level& l)
{
    std::vector<AAGun*> out;
    for (auto v : l.vehicles) if (AAGun* g = dynamic_cast<AAGun*>(v)) out.push_back(g);
    return out;
}

TEST(front_guns_stand_just_before_our_line_from_the_second_level)
{
    World* w = frontWorld(2);
    const auto gs = guns(w->level);
    CHECK(gs.size() >= 1 && gs.size() <= 2);
    bool nearStart = false;
    for (auto g : gs) {
        const Coord c = g->centre();
        const int line = w->level.ownLineX[c.y];
        CHECK((int)c.x >= line + 2 && (int)c.x <= line + 7);
        CHECK(!w->level.behindTheLine(c));
        if (std::abs((int)c.y - (int)w->player.coord.y) <= 12) nearStart = true;
        CHECK(g->crew().empty());                       // the crew lies dead behind the sandbags
        CHECK(std::fabs(g->angle) < 0.01f);             // barrels pointing east, at them
    }
    CHECK(nearStart);
    delete w;
    World* v = frontWorld(1);
    CHECK(guns(v->level).empty());
    delete v;
}

TEST(front_gun_is_taken_without_raising_any_alarm)
{
    World* w = frontWorld(3);
    AAGun* g = guns(w->level).front();
    string msg;
    CHECK(g->approach(&w->player, msg));
    CHECK(w->player.manning == g);
    CHECK(contains(msg, "kaatun"));
    CHECK(!contains(msg, "eiv\xC3\xA4t ilahdu"));
    delete w;
}

TEST(a_manned_gun_leaves_the_gunner_out_in_the_open)
{
    World* w = frontWorld(3);
    AAGun* g = guns(w->level).front();
    string msg;
    w->player.coord = nearestTrench(w->level, g->centre(), Trench::Own);   // even down in a trench beside it
    CHECK(g->approach(&w->player, msg));
    w->player.exposure = 10;
    w->level.exposureTick();
    CHECK(w->player.exposure > 10);
    CHECK(fpvPlayerShare(w->level) >= 10 + 11 * 8 / 10);
    delete w;
}

TEST(a_bradley_takes_three_aa_shells)
{
    World* w = frontWorld(3);
    const uint32_t y = w->player.coord.y;
    auto* b = new Bradley(Coord{ 40, y }, 40);
    w->level.addVehicle(b);
    aaExplosion(w->level, b->coord, &w->player);
    CHECK(!b->wrecked);
    aaExplosion(w->level, b->coord, &w->player);
    CHECK(!b->wrecked);
    const string boom = aaExplosion(w->level, b->coord, &w->player);
    CHECK(b->wrecked);
    CHECK(contains(boom, "Bradley"));
    delete w;
}

TEST(an_aa_shell_or_grenade_silences_a_barrier_nest)
{
    World* w = frontWorld(3);
    const size_t before = w->level.mgNests.size();
    const Coord nest = w->level.mgNests.front();
    aaExplosion(w->level, Coord{ nest.x + 1, nest.y }, &w->player);
    CHECK(w->level.mgNests.size() == before - 1);
    CHECK(!w->level.isMgNest(nest));
    const Coord other = w->level.mgNests.front();
    w->level.explode(Coord{ other.x + 2, other.y }, 2, 3, &w->player);
    CHECK(w->level.mgNests.size() == before - 2);
    CHECK(w->printer.saw("pes\xC3\xA4ke"));
    delete w;
}

TEST(barrier_nests_reach_only_twenty_cells)
{
    World* w = frontWorld(3);
    common::realtimeEnabled = false;    // no moment-to-react guard between hits
    w->level.mgNests = { Coord{ 0, 60 } };
    w->player.stats.health = 200;
    w->player.coord = Coord{ 2, 90 };   // behind the line, but out of that gun's reach
    CHECK(w->level.behindTheLine(w->player.coord));
    for (int i = 0; i < 200; ++i) w->level.barrierFireTick();
    CHECK(w->player.stats.health == 200);
    w->player.coord = Coord{ 2, 70 };
    for (int i = 0; i < 200; ++i) w->level.barrierFireTick();
    CHECK(w->player.stats.health < 200);
    delete w;
}

TEST(a_deserter_through_the_gap_reaches_the_rear)
{
    World* w = frontWorld(3);
    CHECK(w->level.holdOfficer != nullptr);
    CHECK(!w->level.rearOpen);
    CHECK(w->level.reachedRear(Coord{ Level::REAR_X, 100 }));
    CHECK(w->level.deserting(Coord{ Level::REAR_X, 100 }));
    CHECK(!w->level.reachedRear(Coord{ 6, 100 }));
    // the first two levels have no barrier to slip through: the rear waits for the shift
    World* v = frontWorld(2);
    CHECK(!v->level.reachedRear(Coord{ Level::REAR_X, 100 }));
    delete w;
    delete v;
}

TEST(a_shell_landing_on_the_gun_wrecks_it)
{
    World* w = frontWorld(3);
    AAGun* g = guns(w->level).front();
    string msg;
    CHECK(g->approach(&w->player, msg));
    w->level.shellBurst(g->centre(), 1, 5, "Bradleyn konetykki");
    CHECK(g->wrecked);
    CHECK(w->player.manning == nullptr);
    delete w;
}

TEST(melee_every_thing_has_a_hitting_power_and_wears_out)
{
    const Thing all[] = { Thing::Kalja, Thing::Lonkka, Thing::Vodka, Thing::Tyhja, Thing::Lenkki, Thing::Kala,
                          Thing::Omppo, Thing::Bansku, Thing::Ketjut, Thing::Veitsi, Thing::Kivi, Thing::Pamppu,
                          Thing::Bootsit, Thing::Granata, Thing::Pistooli, Thing::Rynkky, Thing::Lipas };
    for (Thing t : all) {
        CHECK(meleePower(t) != 0);
        CHECK(isBottle(t) || meleeDurability(t) > 0);
    }
    CHECK(meleePower(Thing::Lenkki) < 0);   // soft
    CHECK(meleePower(Thing::None) == 0);
}

TEST(melee_a_sausage_softens_the_blow_but_a_free_fist_does_not)
{
    World w;
    common::realtimeEnabled = false;
    CHECK(bumpRat(w, Thing::Lenkki, Thing::None) == 1);       // the free hand punches
    w.printer.log.clear();
    CHECK(bumpRat(w, Thing::Lenkki, Thing::Bansku) == 0);     // both full of soft stuff
    CHECK(w.printer.saw("pehme"));
}

TEST(melee_a_sausage_keeps_a_weak_one_on_his_feet)
{
    World w;
    common::realtimeEnabled = false;
    hold(w.player.inventory, Thing::Lenkki, Thing::Lenkki);
    auto* rat = w.spawn(new Rotta(Coord{ 51, 50 }));
    rat->stats.health = 1;
    w.player.move(DirectionNS::right, &w.level);
    CHECK(rat->stats.health == 1);
    CHECK(w.printer.saw("pystyss"));
}

TEST(melee_things_break_after_enough_blows)
{
    World w;
    common::realtimeEnabled = false;
    hold(w.player.inventory, Thing::None, Thing::Pamppu);
    for (uint32_t i = 1; i < meleeDurability(Thing::Pamppu); ++i) bumpRatAgain(w);
    CHECK(w.player.inventory.right.what == Thing::Pamppu);
    w.printer.log.clear();
    bumpRatAgain(w);
    CHECK(w.player.inventory.right.empty());
    CHECK(w.printer.saw("katkeaa"));
}

TEST(melee_the_rifle_butt_breaks_and_the_rifle_is_gone)
{
    World w;
    common::realtimeEnabled = false;
    hold(w.player.inventory, Thing::None, Thing::None);
    w.player.inventory.back = Inventory::fresh(Thing::Rynkky);
    w.player.inventory.akRaised = true;
    for (uint32_t i = 0; i < meleeDurability(Thing::Rynkky); ++i) bumpRatAgain(w);
    CHECK(w.player.inventory.back.empty());
    CHECK(!w.player.inventory.akRaised);
}

TEST(melee_a_broken_sausage_is_eaten_for_a_little_strength)
{
    World w;
    common::realtimeEnabled = false;
    hold(w.player.inventory, Thing::Lenkki, Thing::Lenkki);
    for (uint32_t i = 1; i < meleeDurability(Thing::Lenkki); ++i) bumpRatAgain(w);
    w.player.stats.hunger = 50;
    w.player.stats.health = 10;
    w.player.inventory.left.wear = meleeDurability(Thing::Lenkki) - 1;
    auto* rat = w.spawn(new Rotta(Coord{ 51, 50 }));
    rat->stats.health = 100;
    w.printer.log.clear();
    w.player.move(DirectionNS::right, &w.level);
    CHECK(w.player.inventory.left.empty());
    CHECK(w.player.inventory.right.what == Thing::Lenkki);
    CHECK(w.printer.saw("Sy\xC3\xB6t loput"));
    CHECK(w.player.stats.hunger > 50);
    CHECK(w.player.stats.health == 10);   // the rat's bite (-1) and the sausage (+1)
}
