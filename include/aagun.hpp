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

// What a shell hit, for the gunner's message.
struct ShellHit {
    Coord at;
    std::string what;           // "" for an air burst
    bool drone = false;
};

// An anti-aircraft gun in a ring of sandbags. It never moves, so it lives among the vehicles:
// blocking the way and blown up like them. The mount (a 3x3 core) and its legs and barrels
// turn as a whole with the aim, 12 degrees a frame; the ring has a gap to the west, where the
// trench is. Whoever mans it can shoot at anything until the ammunition runs out, each barrel
// firing in its turn.
class AAGun : public Vehicle {
public:
    // the makes found at the front: a single-barrel 23 mm, the twin "Sergei" (ZU-23-2, the
    // Kremlin's), and a quad of 7.62 mm machine guns on a Flakvierling mount
    enum class Model { Zu23, Sergei, Vierling };
    static constexpr uint32_t AMMO = 100;
    static constexpr uint32_t RANGE = 45;        // cells; the shell bursts in the air beyond
    static constexpr uint32_t ARMING = 4;        // cells before the fuze arms: it flies over heads
    static constexpr uint32_t SHOT_MS = 350;     // between shells, Space held down

    static constexpr float TURN_STEP = 0.20943951f;   // 12 degrees: the mount turns this much a frame
    static constexpr uint32_t FLASH_MS = 90;          // a barrel's muzzle flash and recoil
    static constexpr uint32_t VIERLING_GAP_MS = 35;   // between the four barrels of one burst
    static constexpr float RING_X = 6.0f, RING_Y = 3.2f;  // the sandbag ring's radii (cells are tall)

    explicit AAGun(const Coord& left, Model model = Model::Sergei);
    virtual void update() { }
    // the mount turns toward the aim; muzzle flashes come and go
    virtual void frame(uint32_t ms);
    // the 3x3 core and the sandbags
    virtual std::vector<Coord> cells() const;
    virtual std::string name() const { return "IT-tykki"; }
    virtual std::string glyphAt(uint32_t index) const;
    virtual const char* colorAt(uint32_t index) const;

    // the gun itself, between the sandbags; shells leave from here
    Coord centre() const { return Coord{ coord.x + 1, coord.y }; }
    // the mount (3x3 around the centre), not the sandbags round it
    bool isCore(const Coord& c) const;
    // a ring of sandbags some four cells out, with a gap to the west; cells that are blocked,
    // a road or a trench get no bag
    void laySandbags(const Level& level);
    std::vector<Coord> sandbags;

    // barrels: how many, how far each sits from the mount's axis, how long they are
    uint32_t barrels() const;
    float barrelOffset(uint32_t k) const;
    float barrelLength() const;
    // the muzzle of barrel k with the mount turned to `at` (world cell coordinates)
    void barrelTip(uint32_t k, float& x, float& y, float at) const;
    // a barrel fires: flash and recoil, after `delayMs`
    void flashBarrel(uint32_t k, uint32_t delayMs = 0);
    bool flashing(uint32_t k) const { return k < 4 && flashDelay[k] <= 0 && flashMs[k] > 0; }

    // the mount drawn on the map's cells: legs, body, barrels at the shown angle in 12-degree steps
    struct Glyph { int x, y; std::string g; const char* color; };
    std::vector<Glyph> sprite() const;
    // crew still alive, and those of them awake
    std::vector<ITMies*> crew() const;
    std::vector<ITMies*> awakeCrew() const;

    // the player walks into the gun or its crew. The gun is his if nobody awake is
    // guarding it, or if the crew takes a bottle of vodka each. True if he now mans it.
    bool approach(PlayerNS::Player* player, std::string& msg);
    void leave();
    // turn the barrels onto the nearest drone in range; false if there is none
    bool aimAtDrone();
    // a shell along `a`, no ammo counted (the crew has crates of it); `boom` says what it did
    ShellHit shoot(float a, Person* source, std::string& boom);
    // one shell along `angle` (the Flakvierling: four rounds); false (and a click) when the
    // ammunition has run out
    bool fire(std::string& msg);
    // between shots, Space held down: the Sergei's two barrels take turns, twice the ZU-23's rate
    uint32_t shotMs() const;
    uint32_t maxAmmo() const;
    const char* modelName() const;

    Model model;
    uint32_t ammo = AMMO;
    float angle = -1.5708f;                 // barrels pointing north
    float shown = -1.5708f;                 // where the mount points on screen, turning toward `angle`
    uint32_t nextBarrel = 0;                // the one that fires next
    int32_t flashMs[4] = { 0, 0, 0, 0 };
    int32_t flashDelay[4] = { 0, 0, 0, 0 };
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

// Flies a shell from `from` along `angle` and returns where it bursts, the way the aim line
// shows it: walls stop it at once, people, cars and drones only once the fuze has armed.
ShellHit traceShell(Level& level, const Coord& from, float angle, const Vehicle* self);

// A 23 mm high-explosive shell bursts: people torn up, cars wrecked and burning, walls
// blown open, buildings set ablaze, drones knocked out of the sky, shrapnel all round.
// Returns a short description of the worst of it for the gunner.
std::string aaExplosion(Level& level, const Coord& at, Person* source);

// Anti-aircraft guns with their crews: around the Kremlin, and by the refineries later on.
void populateAAGuns(Level& level, uint32_t moscowStage);

// Soldiers, air-defence men and the Kremlin guards out under the open sky open fire on any drone flying close by. A hit brings it
// down in flames; a miss flies on and may hit whoever stands in the way, the player too.
// Called every world tick.
void antiDroneFire(Level& level);
// a drone shot down: it falls where it is and goes up in a fireball
void downDrone(Level& level, size_t index);

// A drone on a petrol station (the kiosk or a pump): a fireball over the forecourt, the
// cars queueing at the pumps blow up one after another. Returns the station's name.
std::string petrolStationBlast(Level& level, Building* hit);

// While the player mans a gun every key goes here (A/D turn, Space fires, Esc leaves).
void gunnerKey(char c, PlayerNS::Player& player, Level& level, Printer& printer, uint32_t& msSinceShot);

#endif
