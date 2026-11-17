
#include "quips.hpp"
#include "player.hpp"
#include "level.hpp"
#include "persons.hpp"
#include "common.hpp"
#include "item.hpp"
#include "input.hpp"
#include "building.hpp"
#include "animation.hpp"
#include "vehicle.hpp"
#include "aagun.hpp"
#include "mask.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <exception>

namespace PlayerNS {

using common::random;
using std::string;
using InputNS::Input;
using InputNS::InputType;
using std::cout;
using std::endl;

bool Player::isVeteran() const
{
    return returnedFromWar && !(level && level->isWarLevel);
}

Player::Player() : Person(PersonType::pelaaja, {0, 0})
{
    stats.health = INITIAL_HP;
    stats.promilles = 2;
    stats.money = 20;
    stats.turn = 1;
}

void Player::updateMoney(int money)
{
    stats.money += money;

#ifdef ANIMATIONS_ENABLED
    string prefix = "+";
    if (money < 0) {
        prefix = "-";
    }

    string ptext = std::to_string(abs(money));

    string text = prefix + ptext + "\xE2\x82\xBD";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a, false);
#endif
}

void Player::updatepromilles(int p)
{
    stats.promilles += p;

#ifdef ANIMATIONS_ENABLED
    string prefix = "+";
    if (p < 0) {
        prefix = "-";
    }

    string ptext = std::to_string(abs(p)/10) + "." + std::to_string(abs(p)%10);

    string text = prefix + ptext + "\xE2\x80\xB0";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a);
#endif
}

void Player::feedHunger(uint32_t amount)
{
    stats.hunger += amount;
    if (stats.hunger > 100) stats.hunger = 100;
}

void Player::stepOnNeedle()
{
    // the dice are rolled but the result stays in the blood, not on the screen
    hiv.needleStick(stats.turn, (uint32_t)(std::rand() % 100));
}

// HIV runs on whether the game is real-time or not: one tick per turn
void Player::diseaseTick()
{
    if (!hiv.infected || stats.health == 0) return;
    const uint32_t turn = stats.turn;

    if (!hiv.fluShown && turn >= hiv.infectedTurn + Disease::FLU_TURNS) {
        hiv.fluShown = true;
        printer->showMessage("Kurkkua kirvelee ja olo on kuumeinen. Krapulaa varmaan. Tai flunssaa.", level, false);
    }

    const uint32_t stage = diseaseStage();
    if (stage == 0) return;
    if (stage > hiv.announcedStage) {
        hiv.announcedStage = stage;
        printer->showMessage(Disease::stageMessage(stage), level, false);
    }

    // the body no longer holds its strength; with AIDS it drains away
    uint32_t cap = Disease::maxHealth(stage);
    const uint32_t lost = hiv.wasted(turn);
    cap = lost >= cap ? 0 : cap - lost;
    if (stats.health > cap) {
        const bool wasting = stage >= Disease::AIDS_STAGE && turn > hiv.stageStart(stage);
        stats.health = cap;
        if (wasting) printer->showMessage(cap == 0 ? "Keho luovuttaa." : "Voimat valuvat pois. Jokainen askel on työtä.", level, false);
    }

    // a cough the neighbours hear; from the third stage food will not stay down
    if (stage >= 2 && level && std::rand() % 150 == 0) {
        level->audio.addSound(coord, SoundType::Voice, "*KÖH KÖH KÖH*", "*köh köh*", "", 10);
        printer->showMessage("Yskit verta. Nenäliinaa ei ole, joten hihaan.", level, false);
    }
    if (stage >= 3 && common::realtimeEnabled && turn % 40 == 20 && stats.hunger > 0) --stats.hunger;
}

void Player::updateCrawling()
{
    if (!crawling) {
        if (stats.promilles > CRAWL_PROMILLES && common::random(2)) {
            crawling = true;
            if (printer) printer->showMessage("Jalat pettävät. Konttaat eteenpäin nenä maata viistäen.", level, false);
            maybeDropPocket();
        }
    } else if (stats.promilles <= CRAWL_PROMILLES || common::random(3)) {
        crawling = false;
        if (printer) printer->showMessage("Kömmit takaisin jaloillesi.", level, false);
    }
}

void Player::updateTurn()
{
    ++stats.turn;
    diseaseTick();
    // the pocket emptied in a fall a few turns ago: only now you notice
    if (pocketNoticeTurns > 0 && --pocketNoticeTurns == 0 && printer)
        printer->showMessage("Hetkinen... tasku tuntuu kevyelt\xC3\xA4!", level, false);
    updateCrawling();
    drunkStareComment();

    if (common::realtimeEnabled) {
        // turns tick 2.5x per second in real-time: lose 0.1 promille every 50 s
        if (stats.turn % 125 == 0) {
            updatepromilles(-1);
        }
        // hunger decreases every 40 turns
        if (stats.turn % 40 == 0 && stats.hunger > 0) {
            --stats.hunger;
        }
        // starving: take damage
        if (stats.hunger == 0 && stats.turn % 60 == 0) {
            damage(1);
            printer->showMessage("Nälkä kalvaa sisuksiasi...", level, false);
        }
        // low hunger warnings
        if (stats.hunger == 20 && stats.turn % 40 == 1) {
            printer->showMessage("Vatsasi kurnii kovasti!", level, false);
        } else if (stats.hunger == 10 && stats.turn % 40 == 1) {
            printer->showMessage("Olet nälästä sekaisin!", level, false);
        }
    } else {
        if (stats.turn % 260 == 0) {
            updatepromilles(-1);
        }
    }
}

bool Player::interact(std::string& msg, Person* source)
{
    if (stats.health == 0) return true;

    if (source->type == yka) {
        if (common::realtimeEnabled) {
            if (inventory.take(Thing::Kalja)) {
                msg = "Yevgeni pummaa pivoa. Annat yhden.";
                return true;
            } else {
                msg = "Yevgeni taputtelee palleaasi. Nyrkillä.";
                damage(1);
                return false;
            }
        }

        msg = "Yevgeni pummaa yhtä pivoa, annatko? (k/e)";
        printer->showMessage(msg, level);
        char c = InputNS::Input::inputYka();
        switch(c) {
            default:
            case 'k':
            case 'K':
                msg = "Yevgeni kiittelee: \"Спасибо, брат!\" ja istuu alas kittaamaan pivoasi.";
                inventory.take(Thing::Kalja);
                return true;
                break;
            case 'e':
            case 'E':
                msg = "Yevgeni taputtelee palleaasi. Nyrkillä.";
                damage(1);
                return false;
                break;
        }
    }

    return false;
}

bool Player::interactThrow(Item* item, Person* source, std::string& msg)
{
    if ((item->type == EKivi) && (source->type == vanki))
    {
        msg = "Vankikarkuri nakkasi sinua kivellä!";
    } else {
        msg = "Sinua viskattiin jollain, ouch!";
    }
    damage(1);
    return true;
}

void Player::onNewLevel()
{
    // the stare-comment limiter is per level
    lastStareCommentTurn = 0;
}

void Player::resetPosition()
{
    coord = {common::PLAYER_START_X, common::PLAYER_START_Y};
    heading.reset();
}

Held* Player::drinkTarget()
{
    for (Held* h : { &inventory.left, &inventory.right })
        if (h->what == Thing::Vodka && h->fill > 0) return h;
    for (Held* h : { &inventory.left, &inventory.right })
        if (isDrink(h->what)) return h;
    return nullptr;
}

void Player::drinkHeld(Held& h, Level* level)
{
    switch (h.what) {
        case Thing::Vodka: {
            if (!h.pissSeller.empty()) {
                // half of the bottles sold on the street are piss; you find out on the first sip
                const std::string seller = h.pissSeller;
                h = Inventory::heldOf(Thing::Tyhja);
                printer->showMessage(seller + " myi sinulle n\xC3\xA4k\xC3\xB6j\xC3\xA4\xC3\xA4n kusta pullossa, hyi! Oksennat.", level, false);
                level->audio.addSound(coord, SoundType::Vomit,
                    "BL\xC3\x84\xC3\x84\xC3\x84RGH! Oksennat kaaressa!", "Joku oksentaa", "...ykin\xC3\xA4\xC3\xA4...", 15);
                HurtBy who(level, seller + " (kusipullo)");
                damage(2);
                level->addItem(new Oksennus(coord));
                break;
            }
            if (h.fill > 0) --h.fill;
            const uint8_t fill = h.fill > 5 ? 5 : h.fill;
            if (h.fill == 0) h = Inventory::heldOf(Thing::Tyhja);
            printer->showMessage(VODKA_SIP_LINES[fill], level, false);
            common::sound("glug");
            updatepromilles(1);
            feedHunger(1);
            break;
        }
        case Thing::Kalja:
            h = Inventory::heldOf(Thing::Tyhja);
            printer->showMessage(pickQuip(BEER_LINES), level, false);
            common::sound("glug");
            updatepromilles(2);
            feedHunger(2);
            break;
        case Thing::Lonkka:
            h = Held();
            printer->showMessage("Jaguar! Kofeiinia ja viinaa. Syd\xC3\xA4n hakkaa.", level, false);
            common::sound("glug");
            updatepromilles(4);
            feedHunger(1);
            break;
        default: break;
    }
}

bool Player::drink(Level* level)
{
    Held* h = drinkTarget();
    if (!h) {
        printer->showMessage("Ei oo ehtaa tavaraa.", level);
        return false;
    }
    drinkHeld(*h, level);
    return true;
}

uint32_t Player::takeThrowable(Held& h)
{
    const uint32_t n = thingToThrowNum(h.what);
    if (n != 0) h = Held();
    return n;
}

void Player::eatHeld(Held& h, Level* level)
{
    const Thing t = h.what;
    switch (t) {
        case Thing::Lenkki:
            h = Held(); powerup(1);
            printer->showMessage("Mamamamakkaraa.", level, false);
            feedHunger(25);
            break;
        case Thing::Kala:
            h = Held();
            if (random(70)) { damage(2); printer->showMessage("Syomasi kala sisalsi matoja...", level, false); }
            else { powerup(4); printer->showMessage("Kaloreja kalasta.", level, false); feedHunger(30); }
            break;
        case Thing::Omppo:
            h = Held();
            if (random(50)) { damage(1); printer->showMessage("Antonovka oli pilaantunut.", level, false); }
            else { powerup(2); printer->showMessage("orb.", level, false); feedHunger(15); }
            break;
        case Thing::Bansku:
            h = Held();
            if (random(50)) { printer->showMessage("Banaani ei vaikuttanut.", level, false); feedHunger(5); }
            else { powerup(1); printer->showMessage("Babababananiii", level, false); feedHunger(10); }
            break;
        default: break;
    }
}

// the hand "e" eats from: the only one with food, or ask (v/o) when both have some; null = none/cancel
Held* Player::foodHand(Level* level)
{
    Held* l = &inventory.left;
    Held* r = &inventory.right;
    if (isFood(l->what) && isFood(r->what)) return chooseHand(level);
    return isFood(l->what) ? l : isFood(r->what) ? r : nullptr;
}

Held* Player::chooseHand(Level* level)
{
    Held* l = &inventory.left;
    Held* r = &inventory.right;
    if (l->empty() && r->empty()) return nullptr;
    if (l->empty()) return r;
    if (r->empty()) return l;
    printer->showMessage("Kummalla k\xC3\xA4""dell\xC3\xA4? (v/o)", level, false);
    const char c = InputNS::Input::getHand();
    return c == 'v' ? l : c == 'o' ? r : nullptr;
}

bool Player::eat(Level* level)
{
    const bool kalat = inventory.count(Thing::Kala) > 0, omppo = inventory.count(Thing::Omppo) > 0;
    const bool bansku = inventory.count(Thing::Bansku) > 0, lenkki = inventory.count(Thing::Lenkki) > 0;
    if (!kalat && !omppo && !bansku && !lenkki) {
        printer->showMessage("Ei syötävää.", level);
    } else if (!kalat && !omppo && !lenkki) {
        inventory.take(Thing::Bansku);
        if (random(50)) {
            printer->showMessage("Ei vaikutusta.", level);
        } else {
            powerup(1);
            printer->showMessage("Banaani teki teraa !!!", level);
        }
        feedHunger(10);
    } else if (!kalat && !bansku && !lenkki) {
        inventory.take(Thing::Omppo);
        if (random(50)) {
            damage(1);
            printer->showMessage("Antonovka oli pilaantunut,oksennat.", level);
        } else {
            powerup(2);
            printer->showMessage("Omena oli herkullinen !!!", level);
            feedHunger(15);
        }
    } else if (!omppo && !bansku && !lenkki) {
        inventory.take(Thing::Kala);
        if (random(70)) {
            damage(2);
            printer->showMessage("Syomasi kala sisalsi matoja...", level);
        } else {
            powerup(4);
            printer->showMessage("Kala antaa sinulle uutta puhtia !!!", level);
            feedHunger(30);
        }
    } else if (!kalat && !omppo && !bansku) {
        powerup(1);
        inventory.take(Thing::Lenkki);
        printer->showMessage("Oispa edes samogonia kaveriksi.", level);
        feedHunger(25);
    } else {
        printer->showMessage("Mitas...1=makkara 2=kala 3=antonovka 4=banaani ???", level);
        auto type = InputNS::Input::getFood();

        switch(type) {
            case InputNS::foodlenkki:
                if (!inventory.take(Thing::Lenkki)) {
                    printer->showMessage("Ei ole niitä.", level, false);
                } else {
                    printer->showMessage("Mamamamakkaraa.", level, false);
                    feedHunger(25);
                }
                break;
            case InputNS::foodkala:
                if (!inventory.take(Thing::Kala)) {
                    printer->showMessage("Kalat loppu.", level, false);
                } else {
                    if (random(70)) {
                        damage(2);
                        printer->showMessage("Syomasi kala sisalsi matoja...", level, false);
                    } else {
                        powerup(4);
                        printer->showMessage("Kaloreja kalasta.", level, false);
                        feedHunger(30);
                    }
                }
                break;
            case InputNS::foodomppo:
                if (!inventory.take(Thing::Omppo)) {
                    printer->showMessage("EE OO OMMPPOJA.", level, false);
                } else {
                    if (random(50)) {
                        damage(1);
                        printer->showMessage("Joku oli kaytellyt keski keppiaan antonovkaasi.", level, false);
                    } else {
                        powerup(2);
                        printer->showMessage("orb.", level, false);
                        feedHunger(15);
                    }
                }
                break;
            case InputNS::foodbansku:
                if (!inventory.take(Thing::Bansku)) {
                    printer->showMessage("Bailu ilman banaania?", level, false);
                } else {
                    if (random(50)) {
                        printer->showMessage("Banaani ei vaikuttanut.", level, false);
                    } else {
                        powerup(1);
                        printer->showMessage("Babababananiii", level, false);
                    }
                    feedHunger(10);
                }
                break;
            default:
                printer->showMessage("Päätit olla syömättä mitään.", level, false);
                return false;

        }
    }
    return true;
}

bool Player::puke(DirectionNS::Direction d, Level* level)
{
    common::vomitEvent(coord, d == DirectionNS::right ? 1 : d == DirectionNS::left ? -1 : 0,
                       d == DirectionNS::down ? 1 : (d == DirectionNS::up || d == DirectionNS::none) ? -1 : 0);
    common::sound("vomit", 1.0f);
    Coord check = coord;
    switch(d)
    {
        default:
        case DirectionNS::up:
            --check.y;
            break;
        case DirectionNS::right:
            ++check.x;
            break;
        case DirectionNS::down:
            ++check.y;
            break;
        case DirectionNS::left:
            --check.x;
            break;
    }

    bool ret = false;
    auto p = level->getPerson(check);
    if (p)
    {
        ret = p->interactpuke(this);
    }

    if (!ret) {
        auto puke = Item::createItem(12, check);
        level->addItem(puke);
    }

    return ret;
}

static InputType validateDirection(InputType in) {
    switch(in) {
        case InputNS::up:
        case InputNS::right:
        case InputNS::down:
        case InputNS::left:
            return in;
        default:
            return InputNS::inputpending;
    }
}

static DirectionNS::Direction getDirection(Level* level, Printer* printer)
{
    printer->showMessage("Anna suunta: ", level, false);

    InputType direction = InputNS::inputpending;
    while(direction == InputNS::inputpending) {
        direction = Input::getInput();
        direction = validateDirection(direction);
    }

    switch(direction) {
        default:
        case InputNS::up:
            return DirectionNS::up;
        case InputNS::right:
            return DirectionNS::right;
        case InputNS::down:
            return DirectionNS::down;
        case InputNS::left:
            return DirectionNS::left;
    }
}

bool Player::decrementInventory(uint32_t index)
{
    const Thing t = throwNumToThing(index);
    if (t == Thing::None) return false;
    if (inventory.take(t)) return true;
    const char* missing = "";
    switch (t) {
        case Thing::Kalja:   missing = "Ei ole pivoa..."; break;
        case Thing::Lonkka:  missing = "Ei ole Jaguaria..."; break;
        case Thing::Lenkki:  missing = "Ei ole makkaraa..."; break;
        case Thing::Ketjut:  missing = "Ei ole kettinki\xC3\xA4..."; break;
        case Thing::Veitsi:  missing = "Ei ole puukkoja..."; break;
        case Thing::Kivi:    missing = "Ei ole kivi\xC3\xA4..."; break;
        case Thing::Pamppu:  missing = "Ei ole pamppuja..."; break;
        case Thing::Bootsit: missing = "Ei ole verkkareita..."; break;
        case Thing::Kala:    missing = "Ei ole voblaa..."; break;
        case Thing::Omppo:   missing = "Ei ole omenia..."; break;
        case Thing::Bansku:  missing = "Ei ole banaaneja..."; break;
        case Thing::Vodka:   missing = "Ei ole vodkaa..."; break;
        case Thing::Granata: missing = "Ei ole kranaatteja..."; break;
        case Thing::Tyhja:   missing = "Ei ole tyhj\xC3\xA4\xC3\xA4 pulloa..."; break;
        default: break;
    }
    printer->showMessage(missing, level, false);
    return false;
}

bool Player::receive(Thing t, const std::string& pissSeller)
{
    return receive(Inventory::fresh(t, pissSeller));
}

// the map item for something carried, as it was in the hand (rounds, sips)
static Item* heldItem(const Held& h, const Coord& c)
{
    Item* i = Item::createThing(h.what, c);
    if (i) i->held = h;
    return i;
}

bool Player::receive(const Held& h)
{
    if (h.empty()) return false;
    if (inventory.give(h)) return true;
    dropNear(heldItem(h, coord));
    return false;
}

// what cannot fly (a gun, a magazine) is put down next to you as it is, rounds and all
bool Player::dropHeld(Held& h)
{
    if (h.empty()) return false;
    const std::string name = thingName(h.what);
    dropNear(heldItem(h, coord));
    if (&h == &inventory.back) inventory.akRaised = false;
    h = Held();
    if (printer) printer->showMessage("Lasket maahan: " + name + ".", level, false);
    return true;
}

// a free cell next to you, or your own if there is none
void Player::dropNear(Item* item)
{
    if (!item) return;
    item->coord = coord;
    if (!level) { delete item; return; }
    const int d[8][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1} };
    for (const auto& o : d) {
        const Coord c { (uint32_t)((int)coord.x + o[0]), (uint32_t)((int)coord.y + o[1]) };
        if (c.x >= common::SIZEX || c.y >= common::SIZEY) continue;
        // not on a corpse either: the body would be stepped on, not the thing
        if (level->hit(c) || level->getItem(c) || level->getVehicleAt(c)) continue;
        item->coord = c;
        break;
    }
    level->addItem(item);
}

static Coordinate<int> dirToCoord(DirectionNS::Direction d) {
    switch(d)
    {
        case DirectionNS::up:
            return { 0, -1 };
        case DirectionNS::right:
            return { 1, 0 };
        case DirectionNS::down:
            return { 0, 1 };
        case DirectionNS::left:
            return { -1, 0 };
        default:
            throw std::out_of_range("invalid direction!");
    }
}

bool Player::throwItem()
{
    Held* hand = chooseHand(level);
    // empty hands: a small thing from the pocket (a grenade, a knife) goes as well
    if (!hand && inventory.left.empty() && inventory.right.empty() && !inventory.pocket.empty())
        hand = &inventory.pocket;
    if (!hand) {
        printer->showMessage(inventory.freeHands() == 2 ? "Ei mit\xC3\xA4n heitett\xC3\xA4v\xC3\xA4\xC3\xA4." : "Et heit\xC3\xA4 mit\xC3\xA4\xC3\xA4n.", level, false);
        return false;
    }
    // a gun or a magazine does not fly: it is put down, and the hand is free
    if (thingToThrowNum(hand->what) == 0) return dropHeld(*hand);
    const uint32_t number = takeThrowable(*hand);

    auto dir = getDirection(level, printer);
    return executeThrow(number, dir);
}

bool Player::executeThrow(uint32_t itemNum, DirectionNS::Direction dir)
{
    if (level->isWarLevel && std::rand() % 100 < 15) {
        printer->showMessage("Heitto ep\xC3\xA4""onnistuu! Liukkaat k\xC3\xA4""det!", level, false);
        damage(1);
        return true;
    }

    Coordinate<int> c { static_cast<int>(coord.x), static_cast<int>(coord.y) };
    auto d = dirToCoord(dir);

    Item* i = Item::createItem(itemNum, coord);
    i->thrown = true;
    i->throwVec = d;
    level->addThrownItem(i);
    level->actThrow(this);

    return true;
}

static float dirAngle(DirectionNS::Direction d)
{
    switch (d) {
        case DirectionNS::right: return 0.0f;
        case DirectionNS::down:  return 1.5708f;
        case DirectionNS::left:  return 3.14159f;
        default:                 return -1.5708f;
    }
}

bool Player::executeThrowAt(uint32_t itemNum, float angle)
{
    if (level->isWarLevel && std::rand() % 100 < 15) {
        printer->showMessage("Heitto ep\xC3\xA4""onnistuu! Liukkaat k\xC3\xA4""det!", level, false);
        damage(1);
        return true;
    }
    Item* i = Item::createItem(itemNum, coord);
    i->thrown = true;
    i->angled = true;
    i->fx = coord.x + 0.5f;
    i->fy = coord.y + 0.5f;
    i->fdx = std::cos(angle);
    i->fdy = std::sin(angle);
    level->addThrownItem(i);
    level->actThrow(this);
    return true;
}

bool Player::shoot(DirectionNS::Direction dir)
{
    return shootAt(dirAngle(dir));
}

bool Player::shootAt(float angle)
{
    // the pistol has to be in a hand: off the belt first, if a hand is free
    if (!inventory.find(Thing::Pistooli)) {
        printer->showMessage("Ei pistoolia.", level, false);
        return false;
    }
    if (!drawPistol()) return false;
    Held* gun = inventory.inHand(Thing::Pistooli);
    if (gun->rounds == 0 && !reload(Thing::Pistooli)) {
        printer->showMessage("Ei ammuksia!", level, false);
        return false;
    }
    --gun->rounds;
    // the last round out: the spare goes in by itself
    if (gun->rounds == 0) reload(Thing::Pistooli);

    if (level->isWarLevel) {
        int roll = std::rand() % 100;
        if (roll < 20) {
            printer->showMessage("*KLIK* Ase jumittaa! Ruosteinen romu!", level, false);
            return true;
        } else if (roll < 30) {
            printer->showMessage("PANG! Ase r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4 k\xC3\xA4""dess\xC3\xA4""si!", level, false);
            damage(std::rand()%3 + 2);
            return true;
        }
    }

    level->audio.addSound(coord, SoundType::Gunshot,
        "PAM! Pistooli pamahtaa!", "Pistoolinlaukaus", "...pamaus kaukaa...", 30);
    level->alertCops(this, 30);

    Person* target = level->fireBullet(this, angle, 20, 5, 90);
    if (!target) printer->showMessage("PAM! Huti.", level, false);
    else printer->showMessage(target->stats.health == 0 ? "PAM! Osuma! Kohde kaatuu maahan." : "PAM! Osuma!", level, false);
    return true;
}

bool Player::drawPistol()
{
    if (inventory.inHand(Thing::Pistooli)) return true;
    if (inventory.belt.what != Thing::Pistooli) return false;
    Held* hand = inventory.freeHands() == 0 ? nullptr : inventory.right.empty() ? &inventory.right : &inventory.left;
    if (!hand) {
        // a bottle can be drunk and thrown away; anything else, the hands are just full
        const bool bottle = inventory.bottlesInHands() > 0;
        if (printer) printer->showMessage(bottle ? HINT_PISTOL_BOTTLE : HINT_PISTOL_HANDS, level, false);
        return false;
    }
    *hand = inventory.belt;
    inventory.belt = Held();
    return true;
}

void Player::holsterPistol()
{
    Held* hand = inventory.inHand(Thing::Pistooli);
    if (!hand || !inventory.hasBelt || !inventory.belt.empty()) return;
    inventory.belt = *hand;
    *hand = Held();
}

bool Player::shoulderAk()
{
    if (inventory.back.what != Thing::Rynkky) return false;
    if (inventory.akRaised) return true;
    if (inventory.freeHands() != 2) {
        if (printer) printer->showMessage(HINT_AK_HANDS, level, false);
        return false;
    }
    inventory.akRaised = true;
    return true;
}

void Player::lowerAk()
{
    inventory.akRaised = false;
}

void Player::issueRifle()
{
    const bool hadSomething = !inventory.left.empty() || !inventory.right.empty();
    for (Held* h : { &inventory.left, &inventory.right }) {
        if (h->empty()) continue;
        dropNear(heldItem(*h, coord));
        *h = Held();
    }
    if (inventory.back.what == Thing::Rynkky) inventory.back.rounds = AK_ROUNDS;
    else inventory.back = Inventory::fresh(Thing::Rynkky);
    inventory.akRaised = true;
    if (printer) printer->showMessage(hadSomething ? AK_ISSUED_MSG + " KÃ¤sistÃ¤ putosi kaikki muu." : AK_ISSUED_MSG, level, false);
}

bool Player::reload(Thing gun)
{
    // the pocket's spare first, then one in a hand
    Held* mag = inventory.pocket.what == Thing::Lipas ? &inventory.pocket : inventory.inHand(Thing::Lipas);
    if (!mag) return false;
    Held* g = nullptr;
    if (gun == Thing::None) {
        // the gun in hand, else whichever is not full
        g = inventory.inHand(Thing::Pistooli);
        if (!g && inventory.akRaised) g = &inventory.back;
        if (!g) {
            Held* p = inventory.find(Thing::Pistooli);
            Held* ak = inventory.find(Thing::Rynkky);
            if (p && p->rounds < MAKAROV_ROUNDS) g = p;
            else if (ak && ak->rounds < AK_ROUNDS) g = ak;
        }
    } else if (gun == Thing::Pistooli || gun == Thing::Rynkky) {
        g = inventory.find(gun);
    }
    if (!g) return false;
    const uint8_t full = g->what == Thing::Rynkky ? AK_ROUNDS : MAKAROV_ROUNDS;
    if (g->rounds >= full) return false;
    g->rounds = full;
    *mag = Held();
    if (printer) printer->showMessage("Vaihdat lippaan.", level, false);
    return true;
}

Thing Player::readyWeapon()
{
    const bool spare = inventory.find(Thing::Lipas) != nullptr;
    auto canFire = [&](const Held* g) { return g && (g->rounds > 0 || spare); };
    const Held* inHand = inventory.inHand(Thing::Pistooli);
    const Held* pistol = inventory.find(Thing::Pistooli);
    const Held* ak = inventory.find(Thing::Rynkky);
    if (canFire(inHand)) return Thing::Pistooli;
    if (inventory.akRaised && ak) return Thing::Rynkky;
    if (canFire(ak) && inventory.freeHands() == 2) return Thing::Rynkky;
    if (canFire(pistol)) return Thing::Pistooli;
    if (canFire(ak)) return Thing::Rynkky;   // up it goes, or the hint says why not
    if (pistol) return Thing::Pistooli;      // "Ei ammuksia."
    if (ak) return Thing::Rynkky;
    return Thing::None;
}

void Player::maybeDropPocket(bool force)
{
    if (inventory.pocket.empty()) return;
    if (!force && !common::random(POCKET_DROP_PERCENT)) return;
    Item* i = heldItem(inventory.pocket, coord);
    inventory.pocket = Held();
    if (i) {
        i->coord = coord;
        if (level) level->addItem(i);
        else delete i;
    }
    pocketNoticeTurns = POCKET_NOTICE_TURNS;
}

uint32_t Player::coatCap() const
{
    const uint32_t stage = diseaseStage();
    return stage > 0 ? std::min(INITIAL_HP, Disease::maxHealth(stage)) : INITIAL_HP;
}

void Player::wearCoat(uint32_t hp, Coat kind)
{
    const uint32_t to = std::min(hp, coatCap());
    if (stats.health < to) stats.health = to;
    usedCoat = false;
    coat = kind;
}

// these run after chaseTarget and arrest; a brawl would only stop them
static bool chasesLikeCops(const Person* p)
{
    return p->type == poliisi || p->type == vartija || p->type == myyja || p->type == fsb || p->type == sotilas;
}

bool Player::stripCorpseCoat(Ruumis* corpse)
{
    if (!corpse || corpse->coatTaken) return false;
    corpse->coatTaken = true;
    wearCoat(INITIAL_HP, corpse->coat);
    if (printer) printer->showMessage(corpse->coat == Coat::Poliisi
            ? "Riisut poliisin takin ja puet sen p\xC3\xA4\xC3\xA4llesi. Ehj\xC3\xA4 ja sininen, viel\xC3\xA4 l\xC3\xA4mmin."
            : "Riisut ruumiin takin ja puet sen p\xC3\xA4\xC3\xA4llesi. Viel\xC3\xA4 l\xC3\xA4mmin.", level, false);
    if (!level) return true;
    // a disgrace: everyone who saw it comes for you
    for (auto p : level->persons) {
        if (p == this || p->type == pelaaja || p->stats.health == 0) continue;
        if (!p->canSeeTarget(coord)) continue;
        p->chaseTarget = this;
        p->alerted = true;
        p->lastKnownPos = coord;
        p->alertCooldown = 15;
        // the ones who do not chase like cops come at you with their fists
        if (!chasesLikeCops(p)) p->brawlActions = 15;
        p->say(common::random(50) ? "Ruumiinry\xC3\xB6st\xC3\xA4j\xC3\xA4!" : "Paskiainen!");
    }
    return true;
}

// asked, so it is never done by accident; not asked when a coat would give nothing
void Player::offerCorpseCoat(Ruumis* corpse)
{
    if (!corpse || corpse->coatTaken || stats.health >= coatCap()) return;
    printer->showMessage("Riisutko takin? (k/e)", level, false);
    const char c = InputNS::Input::getFreshMenuKey();
    if (c == 'k' || c == 'K') stripCorpseCoat(corpse);
}

static Building* checkDoor(Level* level, const Coord& c)
{
    for (auto& b : level->buildings) {
        if (b->hitDoor(c)) {
            return b;
        }
    }
    return nullptr;
}

static Building* checkWalls(Level* level, const Coord& c)
{
    for (auto& b : level->buildings) {
        if (b->hitWall(c)) {
            return b;
        }
    }
    return nullptr;
}

// what lies on a corpse is picked up before the body is looked at
static Item* checkItems(Level* level, const Coord& c)
{
    Item* corpse = nullptr;
    for (auto& i : level->items) {
        if (i->coord != c) continue;
        if (i->type != ERuumis) return i;
        if (!corpse) corpse = i;
    }
    return corpse;
}

void Player::drunkStareComment()
{
    static const uint32_t STARE_COMMENT_GAP = 40;
    if (!level || !printer || !printer->mask) return;
    if (stats.promilles < 15 || stats.health == 0) return;
    // the remembered person may be gone: only trust one still in the level's list
    const Person* who = printer->mask->staredAt;
    if (!who || who == this) return;
    if (std::find(level->persons.begin(), level->persons.end(), who) == level->persons.end()) return;
    if (who->stats.health == 0 || who->coord.distance(coord) > 6) return;
    if (lastStareCommentTurn != 0 && stats.turn - lastStareCommentTurn < STARE_COMMENT_GAP) return;
    if (!common::random(10)) return;
    lastStareCommentTurn = stats.turn;
    const_cast<Person*>(who)->say("Seh\xC3\xA4n katsoo kuin ryss\xC3\xA4 rahaa.");
}

bool Player::stagger(Level* level)
{
    printer->showMessage("Horjahtelet...", level, false);

    // the nearest onlooker who can see you remarks on it
    if (common::random(15)) {
        Person* onlooker = nullptr;
        uint32_t best = 9;
        for (auto o : level->persons) {
            if (o == this || o->stats.health == 0) continue;
            const uint32_t d = o->coord.distance(coord);
            if (d > 8 || d >= best) continue;
            if (!o->canSeeTarget(coord)) continue;
            onlooker = o;
            best = d;
        }
        if (onlooker) onlooker->say("Seh\xC3\xA4n py\xC3\xB6rii kuin ryss\xC3\xA4 Anttilassa.");
    }

    DirectionNS::Direction d;
    if (common::random(50)) {
        d = (common::random(50) ? DirectionNS::left : DirectionNS::left);
    } else {
        d = (common::random(50) ? DirectionNS::up : DirectionNS::down);
    }

    return move(d, level);
}

bool Player::move(DirectionNS::Direction d, Level* level)
{
    // spend turn
    bool ret = true;
    // prevent movement
    bool blocked = false;

    Coord check { coord.x, coord.y };

    switch(d)
    {
        case DirectionNS::down:
            check += { 0, 1 };
            break;
        case DirectionNS::up:
            check -= { 0, 1 };
            break;
        case DirectionNS::right:
            check += { 1, 0 };
            break;
        case DirectionNS::left:
            check -= { 1, 0 };
            break;
        case DirectionNS::none:
            return false;
    }

    // facing turns towards the step even if the way is blocked
    heading.step((int)check.x - (int)coord.x, (int)check.y - (int)coord.y);

    // in the FSB's grip: a step anywhere but into someone (a punch) is a struggle to break free
    if (FSBAgent* holder = level->fsbHolder()) {
        if (!level->getPerson(check)) {
            const uint32_t chance = stats.promilles >= 25 ? 5 : 30 - stats.promilles;
            if (!common::random(chance)) {
                printer->showMessage("FSB-agentti pit\xC3\xA4\xC3\xA4 sinusta kiinni! Riuhdo, ly\xC3\xB6 tai ammu.", level);
                return true;
            }
            holder->holding = false;
            holder->stun = 2;
            printer->showMessage("Riuhtaiset itsesi irti!", level);
        }
    }

    string msg;
    // someone standing in a doorway is bumped (punched), not the door
    const bool someoneThere = level->hitPerson(check);
    Building* b = someoneThere ? nullptr : checkDoor(level, check);
    if (b && b->walkable) {
        // walkable buildings are entered through the door; the way out is always open
        const bool leaving = b->hitBuilding(coord);
        if (leaving || b->getEnterMsg(this, msg)) {
            if (!leaving) {
                printer->showMessage(msg, level, false);
                if (b->getInteractMsg(this, msg)) printer->showMessage(msg, level, false);
                msg.clear();
                b->interact(this, msg);
                if (!msg.empty()) printer->showMessage(msg, level, false);
                // a seller who caught you stealing now stands in the doorway
                if (level->hitPerson(check)) return ret;
            }
            coord = check;
            return ret;
        }
        printer->showMessage(msg, level, false);
        return ret;
    }
    if (b) {
        blocked = true;

        // check whether can enter
        bool entered = b->getEnterMsg(this, msg);
        printer->showMessage(msg, level, entered);
        if (entered)
        {
            // step into building
            b->getInteractMsg(this, msg);
            printer->showMessage(msg, level, true);

            b->interact(this, msg);
            printer->showMessage(msg, level, false);
        }
    }

    if (!blocked) {
        if (!b && !someoneThere) {
            b = checkWalls(level, check);
            if (b)
            {
                if (!b->bumpWall(this, check, msg)) msg = b->getWalkMsg();
                printer->showMessage(msg, level, true);
                blocked = true;
            }
        }
    }

    if (!blocked) {
        Car* getaway = dynamic_cast<Car*>(level->getVehicleAt(check));
        if (getaway && getaway->loot) {
            // the robbers left the glovebox unlocked
            getaway->loot = false;
            // what does not fit is left on the ground next to the car
            msg = "Penkot ry\xC3\xB6st\xC3\xA4jien autoa: lipas";
            bool fits = receive(Thing::Lipas);
            if (inventory.count(Thing::Pistooli) == 0 || common::random(40)) { fits &= receive(Thing::Pistooli); msg += ", pistooli"; }
            if (common::random(40)) { fits &= receive(Thing::Granata); msg += ", kranaatti"; }
            if (common::random(25)) { fits &= receive(Thing::Rynkky); msg += ", RYNKKY"; }
            msg += "!";
            if (!fits) msg += string(" ") + HINT_HANDS_FULL;
            printer->showMessage(msg, level, false);
            blocked = true;
        } else if (AAGun* gun = dynamic_cast<AAGun*>(level->getVehicleAt(check))) {
            // bribe, sneak past a sleeping crew or take it over: see AAGun::approach
            gun->approach(this, msg);
            printer->showMessage(msg, level, false);
            blocked = true;
        } else if (level->getVehicleAt(check) && level->getVehicleAt(check)->wrecked) {
            msg = level->getVehicleAt(check)->name() + " on palanut romu. Pelti on viel\xC3\xA4 kuuma.";
            printer->showMessage(msg, level, false);
            blocked = true;
        } else if (Vehicle* car = level->getVehicleAt(check)) {
            Car* c = dynamic_cast<Car*>(car);
            if (c && common::realtimeEnabled) {
                // walking into a car is trying the door
                enterCar(this, level, c);
            } else {
                msg = car->name() + " on tiell\xC3\xA4. Kolautat p\xC3\xA4\xC3\xA4si sen kylkeen.";
                printer->showMessage(msg, level, false);
            }
            blocked = true;
        }
        Person* p = blocked ? nullptr : level->checkPerson(check);
        if (p && p->hasLoot()) {
            // caught the pickpocket: money, bottles and whatever else he grabbed
            msg = "Sait kiinni! Otat takaisin:";
            if (p->stolenMoney > 0) msg += " " + std::to_string(p->stolenMoney) + " ruplaa,";
            if (p->stolenBottles > 0) msg += " " + std::to_string(p->stolenBottles) + (p->stolenBottles == 1 ? " pullon," : " pulloa,");
            for (const Held& h : p->stolenItems) msg += string(" ") + stealableName(h.what) + ",";
            msg.back() = '.';
            // what does not fit the hands is dropped next to you
            for (uint32_t k = 0; k < p->stolenBottles; ++k) receive(Thing::Tyhja);
            for (const Held& h : p->stolenItems) receive(h);
            stats.money += p->stolenMoney;
            p->stolenMoney = 0;
            p->stolenBottles = 0;
            p->stolenItems.clear();
            p->fleeActions = 0;
            p->brawlActions = 0;
            p->say("Ei ollut minun! Ei ollut minun!");
            printer->showMessage(msg, level);
            blocked = true;
            p = nullptr;
        }
        if (p)
        {
            blocked = p->interact(msg, this);
            printer->showMessage(msg, level);

            if (p->stats.health == 0) {
                Item* i = p->dropItem();
                if (i) {
                    level->addItem(i);
                }
            }
        }
    }

    if (!blocked) {
        Item* i = nullptr;
        if (!b) {
            i = checkItems(level, check);
            if (i) {
                msg = i->getMsg();
                printer->showMessage(msg, level, false);
                auto consume = i->interact(this);

                if (consume) {
                    level->removeItem(i);
                }
            }
        }
    }

    if (!blocked) {
        coord = check;
    }

    level->playAnimation();

    if (common::checkBounds(coord)) {
        printer->showMessage("Ei karata pelialueelta !!", level, true);
        ret = false;
    }

    return ret;
}

}