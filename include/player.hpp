#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "persons.hpp"
#include "inventory.hpp"
#include "direction.hpp"
#include "printer.hpp"
#include "scoreboard.hpp"
#include "stats.hpp"
#include "disease.hpp"
#include <cstdint>
#include <string>

class Level;
class Car;

namespace PlayerNS {

constexpr uint32_t INITIAL_HP = 10;
// above this many tenths of a promille you may end up crawling on all fours
constexpr uint32_t CRAWL_PROMILLES = 25;

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
    virtual bool move(DirectionNS::Direction d, Level* level);
    virtual bool interact(std::string& msg, Person* source);
    virtual bool interactThrow(Item* item, Person* source, std::string& msg);

    virtual bool drink(Level* level);
    virtual bool eat(Level* level);
    virtual bool puke(DirectionNS::Direction d, Level* level);
    virtual bool throwItem();

    // back from the front, the pultsari is now a "veteran"
    virtual std::string typeToChar() const { return stats.health == 0 ? "x" : isVeteran() ? "V" : "@"; };
    bool isVeteran() const;

    Inventory inventory;
    bool inJail = false;

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
    bool executeThrow(uint32_t itemNum, DirectionNS::Direction dir);
    bool executeThrowAt(uint32_t itemNum, float angle);
    bool shoot(DirectionNS::Direction dir);
    bool shootAt(float angle);

    bool sentToWar = false;
    bool returnedFromWar = false;
    uint32_t warLevelsCompleted = 0;
    bool hasKremlinPass = false;
    bool wonGame = false;
    bool killedPutin = false;
};

}

#endif
