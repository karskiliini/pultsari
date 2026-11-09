#ifndef AAGUN_HPP
#define AAGUN_HPP

#include "vehicle.hpp"
#include "persons.hpp"
#include <string>
#include <vector>

class Level;
class Printer;

namespace PlayerNS {
    class Player;
}

class ITMies;

// A twin-barrelled anti-aircraft gun (ZU-23-2) behind sandbags: "▒Ж▒". It never moves,
// so it lives among the vehicles: drawn, blocking the way and blown up like them.
// Whoever mans it can shoot at anything in the city until the shells run out.
class AAGun : public Vehicle {
public:
    static constexpr uint32_t AMMO = 100;
    static constexpr uint32_t RANGE = 45;        // cells; the shell bursts in the air beyond
    static constexpr uint32_t ARMING = 4;        // cells before the fuze arms: it flies over heads
    static constexpr uint32_t SHOT_MS = 350;     // between shells, Space held down

    explicit AAGun(const Coord& left);
    virtual void update() { }
    virtual void frame(uint32_t) { }
    virtual std::string name() const { return "IT-tykki"; }
    virtual std::string glyphAt(uint32_t index) const;
    virtual const char* colorAt(uint32_t index) const;

    // the gun itself, between the sandbags; shells leave from here
    Coord centre() const { return Coord{ coord.x + 1, coord.y }; }
    // crew still alive, and those of them awake
    std::vector<ITMies*> crew() const;
    std::vector<ITMies*> awakeCrew() const;

    // the player walks into the gun or its crew. The gun is his if nobody awake is
    // guarding it, or if the crew takes a bottle of vodka each. True if he now mans it.
    bool approach(PlayerNS::Player* player, std::string& msg);
    void leave();
    // one shell along `angle`; false (and a click) when the shells have run out
    bool fire(std::string& msg);

    uint32_t ammo = AMMO;
    float angle = -1.5708f;                 // barrels pointing north
    PlayerNS::Player* gunner = nullptr;
};

// One of the gun's crew: fires at the sky (and at drones) while awake, and drinks on duty.
class ITMies : public ITYksikko {
public:
    ITMies(const Coord& pos, AAGun* gun);
    virtual void npcAct();
    virtual bool interact(std::string& msg, Person* source);
    virtual std::string typeToChar() const { return asleep() ? "z" : "A"; }

    bool asleep() const { return passedOut || sleepActs > 0; }
    void doze(uint32_t acts) { sleepActs = acts; }

    AAGun* gun;
    uint32_t sleepActs = 0;     // dozing: actions until he wakes up
    bool passedOut = false;     // drank the bribe: out for good
};

// What a shell hit, for the gunner's message.
struct ShellHit {
    Coord at;
    std::string what;           // "" for an air burst
    bool drone = false;
};

// Flies a shell from `from` along `angle` and returns where it bursts, the way the aim line
// shows it: walls stop it at once, people, cars and drones only once the fuze has armed.
ShellHit traceShell(Level& level, const Coord& from, float angle, const Vehicle* self);

// A 23 mm high-explosive shell bursts: people torn up, cars wrecked and burning, walls
// blown open, buildings set ablaze, drones knocked out of the sky, shrapnel all round.
// Returns a short description of the worst of it for the gunner.
std::string aaExplosion(Level& level, const Coord& at, Person* source);

// Anti-aircraft guns with their crews: around the Kremlin, and by the refineries later on.
void populateAAGuns(Level& level, uint32_t moscowStage);

// While the player mans a gun every key goes here (A/D turn, Space fires, Esc leaves).
void gunnerKey(char c, PlayerNS::Player& player, Level& level, Printer& printer, uint32_t& msSinceShot);

#endif
