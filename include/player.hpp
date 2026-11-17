#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "persons.hpp"
#include "inventory.hpp"
#include "direction.hpp"
#include "printer.hpp"
#include "scoreboard.hpp"
#include "stats.hpp"
#include "disease.hpp"
#include "gaze.hpp"
#include <cstdint>
#include <string>

class Level;
class Car;
class AAGun;
class Metro;

namespace PlayerNS {

constexpr uint32_t INITIAL_HP = 10;
// above this many tenths of a promille you may end up crawling on all fours
constexpr uint32_t CRAWL_PROMILLES = 25;
// a fall may shake the pocket empty; you notice a few turns later
constexpr uint32_t POCKET_DROP_PERCENT = 25;
constexpr uint32_t POCKET_NOTICE_TURNS = 4;
// the pawnshop's worn coat: a little strength, never past full; 30 roubles
constexpr uint32_t USED_COAT_HP = 3;
constexpr uint32_t USED_COAT_PRICE = 30;

class Player : public Person {
public:
    Player();
    virtual ~Player() = default;
    virtual void updatepromilles(int p);
    virtual void updateMoney(int money);
    void updateTurn();
    // very drunk: drop to all fours now and then, get up again later
    void updateCrawling();
    // crawling puts your eyes down at knee height
    virtual uint32_t height() const { return (crawling && stats.health > 0) ? HEIGHT_LOW : Person::height(); }
    bool crawling = false;
    void feedHunger(uint32_t amount);

    // a used needle went in; maybe HIV, the player is not told
    void stepOnNeedle();
    void diseaseTick();
    // 0 healthy or incubating, 1-4 HIV symptoms, 5 AIDS
    uint32_t diseaseStage() const { return hiv.stage(stats.turn); }
    uint32_t hearingPercent() const { return Disease::hearingPercent(diseaseStage()); }
    bool diedOfAids() const { return stats.health == 0 && diseaseStage() >= Disease::AIDS_STAGE; }
    Disease::Hiv hiv;

    void resetPosition();

    virtual void npcAct() { }

    bool stagger(Level* level);
    // passers-by remark on a drunk who stares at them (called every turn)
    void drunkStareComment();
    virtual bool move(DirectionNS::Direction d, Level* level);
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);

    virtual bool drink(Level* level);
    // the drink in hand a plain "q" goes for: a vodka with something left, then the left, then the right
    Held* drinkTarget();
    // one sip, one go: whatever the hand holds is drunk (the shared path of "q" and the inventory overlay)
    void drinkHeld(Held& h, Level* level);
    // which hand a throw or use is meant for: the only full hand, or ask (v/o) when both are; null = none/cancel
    Held* chooseHand(Level* level);
    // the hand real-time "e" eats from: the only one with food, or ask (v/o) when both have some
    Held* foodHand(Level* level);
    // eat what the hand holds (the overlay row and "e" share this)
    void eatHeld(Held& h, Level* level);
    // the hand lets go of what it holds, to be thrown: its throw number (0 and untouched if it cannot fly)
    uint32_t takeThrowable(Held& h);
    virtual bool eat(Level* level);
    virtual bool puke(DirectionNS::Direction d, Level* level);
    virtual bool throwItem();

    // back from the front, the pultsari is now a "veteran"
    virtual std::string typeToChar() const { return stats.health == 0 ? "x" : isVeteran() ? "V" : "@"; };
    bool isVeteran() const;

    Inventory inventory;
    bool inJail = false;
    // the FSB has pushed him into a black car: off to the front
    bool conscripted = false;

    // weighted walking direction, steers where the player looks on foot
    VisionNS::Heading heading;

    // at the wheel of a car, and where the eyes are
    Car* driving = nullptr;
    float gaze = 0.0f;           // radians, clockwise from east, on screen proportions
    float gazeOffset = 0.0f;     // how far the eyes have strayed from the road
    float glanceTarget = 0.0f;
    uint32_t glanceMs = 0;
    uint32_t gazeClockMs = 0;
    float camLeadX = 0.0f, camLeadY = 0.0f;

    ScoreBoard scoreBoard;

    bool decrementInventory(uint32_t index);
    // into the hands (or pocket, belt, back); what does not fit is left on the ground next to you
    bool receive(Thing t, const std::string& pissSeller = "");
    bool receive(const Held& h);
    void dropNear(Item* item);
    // throwing what cannot fly: it is put down next to you as it is (rounds kept); false if empty
    bool dropHeld(Held& h);
    bool executeThrow(uint32_t itemNum, DirectionNS::Direction dir);
    bool executeThrowAt(uint32_t itemNum, float angle);
    bool shoot(DirectionNS::Direction dir);
    bool shootAt(float angle);

    // the pistol from the belt into a free hand (already in a hand counts); false + a hint if no hand is free
    bool drawPistol();
    // back onto the belt, if there is one; without a belt it stays in the hand
    void holsterPistol();
    // the rifle off the back to the shoulder: both hands must be empty (false + a hint otherwise)
    bool shoulderAk();
    void lowerAk();
    // sent to the front: a loaded AK at the shoulder (the one on the back refilled, if any);
    // whatever was in the hands is put down next to you. Nothing takes it away on the way back.
    void issueRifle();
    // a spare magazine (the pocket's, else one in a hand) fills a gun (None: the one in hand, else whichever needs it)
    bool reload(Thing gun = Thing::None);
    // the gun Space raises: the one in hand first; an empty rifle never blocks a loaded pistol
    Thing readyWeapon();
    // a fall: with POCKET_DROP_PERCENT (always with force) the pocket's thing lands at your feet
    void maybeDropPocket(bool force = false);
    uint32_t pocketNoticeTurns = 0;

    // the most a coat can bring the strength to: full, or less with the disease
    uint32_t coatCap() const;
    // a coat restores strength up to `hp` (never above coatCap(), never down)
    // the coat is worn from then on: `kind` colours the sleeves beside the map
    void wearCoat(uint32_t hp, Coat kind = Coat::Siviili);
    // the coat off a corpse: full strength, and everyone who sees it goes for you
    bool stripCorpseCoat(Ruumis* corpse);
    // walking onto a corpse: "Riisutko takin? (k/e)", only if a coat would help
    void offerCorpseCoat(Ruumis* corpse);
    // the last coat was a worn one from the pawnshop: the sleeves stay ragged
    bool usedCoat = false;
    // whose coat is on: the poster's own until another is put on
    Coat coat = Coat::Oma;

    bool sentToWar = false;
    bool returnedFromWar = false;
    uint32_t warLevelsCompleted = 0;
    bool hasKremlinPass = false;
    bool wonGame = false;
    bool killedPutin = false;
    // the anti-aircraft gun the player sits behind, if any
    AAGun* manning = nullptr;

    // through the metro's doors: the main loop plays the trip as a mini-game
    Metro* metroFrom = nullptr;
    Metro* metroTrip = nullptr;     // the station the trip is to
    bool metroDodged = false;       // jumped the turnstiles

    // limiter for the stare comment: the turn of the last one (0: none yet on this level)
    uint32_t lastStareCommentTurn = 0;
    // call when a new level begins
    void onNewLevel();
};

}

#endif
