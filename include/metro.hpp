#ifndef METRO_HPP
#define METRO_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace PlayerNS { class Player; }
class Level;

// The metro is a dangerous place, and every trip is a mini-game in three parts:
//  1. the service tunnel down to the platform, past trains and the live rail,
//  2. onto the train through the crowd without punching anyone,
//  3. the ride among drunks, gopniks, dealers, hotheads, cops and an FSB recruiter.
// MetroGame is the game itself, with no input or output of its own: keys and time
// go in, a 80x28 grid of glyphs comes out. The printers draw that grid where the
// city map usually is, so the terminal and the browser show the same thing.
namespace MetroNS {

using Cell = std::pair<std::string, std::string>;  // glyph, SGR colour
using Rows = std::vector<std::vector<Cell>>;

enum class Phase { Tunnel, Platform, Ride, Over };
enum class Outcome { None, Arrived, Overshot, TurnedBack, Arrested, Recruited, Dead };
enum class Kind { Citizen, Babushka, Drunk, Gopnik, Dealer, Hothead, Cop, Fsb, Pickpocket, Rat };

enum Tile : uint8_t {
    TVoid, TWall, TWalk, TLive, TCover, TRail, TSleeper, TGravel, TPillar, TJunk, TExit,
    TFloor, TEdge, TSeat, TPole, TDoor, TCarWall,
};

struct Npc {
    Kind kind = Kind::Citizen;
    int x = 0, y = 0;
    std::string glyph;
    std::string color;
    int cooldown = 0;       // ms until it acts again
    bool seated = false;
    bool boarding = false;  // platform: heading for the doors
    bool exiting = false;   // platform: just got off the train
    bool gone = false;
    int angryMs = 0;        // chases and hits the player while > 0
    int stunMs = 0;         // knocked down, or backing off
    int nearMs = 0;         // how long the player has stood too close
    bool talked = false;    // has had its say
    int refusals = 0;
    int waitMs = 0;         // until it comes over to talk
    bool chasing = false;   // a cop coming to arrest the player
    int chaseMs = 0;        // in the carriage a cop gives up when this runs out
    uint32_t stolen = 0;    // a pickpocket's takings
};

// a train in the tunnel or at the platform
struct Train {
    float x = 0;            // left end
    int len = 36;
    int dir = 1;            // +1 moves right, -1 left
    bool active = false;
    int nextMs = 0;         // until it comes out of the dark
    bool warned = false;
};

// a question someone in the carriage puts to the player; keys 1-3 answer it
struct Prompt {
    int id = 0;             // 0: no question
    int npc = -1;
    std::string text;
    std::vector<std::string> options;
};

class MetroGame {
public:
    static constexpr int W = 80;
    static constexpr int H = 28;
    static constexpr int RIDE_MS = 42000;
    static constexpr int DOORS_MS = 7000;

    MetroGame(PlayerNS::Player* player, const std::string& from, const std::string& to,
              const std::string& after, bool dodgedFare);

    void key(char c);
    void tick(int ms);
    Rows render() const;
    std::string title() const;
    bool over() const { return phase == Phase::Over; }

    // each part can be started on its own; a new game starts in the tunnel
    void startTunnel();
    void startPlatform();
    void startRide();

    // ---- state, public for the tests ----
    PlayerNS::Player* player;
    std::string from, to, after;
    bool dodged;
    Phase phase = Phase::Tunnel;
    Outcome outcome = Outcome::None;
    std::string cause;          // how it ended, for the death card or the summary
    std::string msg;            // the latest event, on the message line
    int msgMs = 0;
    int px = 0, py = 0;
    int t = 0;                  // ms in this part
    int sinceStep = 1000;
    char queued = 0;
    int stunMs = 0;             // after a shock nothing obeys
    int sinceShock = 100000;
    int shake = 0;              // the screen shakes while a train thunders past
    uint32_t hits = 0;          // people the player has punched
    uint32_t startMoney = 0;
    std::vector<std::vector<uint8_t>> tiles;
    std::vector<Npc> npcs;
    Train trains[2];            // the tunnel's two tracks; the platform uses trains[0]
    Prompt prompt;

    // platform: the train's comings and goings
    enum class Stop { Waiting, Arriving, Unloading, Boarding, Closing, Leaving };
    Stop stop = Stop::Waiting;
    int stopMs = 0;
    int missed = 0;             // trains that left without the player

    // ride
    int rideLeft = RIDE_MS;     // until the next station
    int doorsMs = 0;            // > 0 while the doors are open
    bool overshot = false;      // slept past the stop
    int joltMs = 9000;

    // ---- helpers, public for the tests ----
    bool walkable(int x, int y) const;
    int npcAt(int x, int y) const;
    bool trainAt(int x, int y) const;
    bool doorsOpen() const;
    // the player tries to step; walking into someone is a punch, as everywhere in the game
    void step(int dx, int dy);
    void hurt(uint32_t dmg, const std::string& why);
    void hitNpc(int i);
    void answer(int option);
    void finish(Outcome o, const std::string& why);
    void say(const std::string& text);
    uint32_t money() const;
    void pay(uint32_t amount);
    static const char* kindName(Kind k);

private:
    void tickTunnel(int ms);
    void tickPlatform(int ms);
    void tickRide(int ms);
    void moveNpcs(int ms);
    void npcTowards(Npc& n, int tx, int ty);
    bool npcStep(Npc& n, int dx, int dy);
    void openPrompt(int npc, int id);
    void addNpc(Kind k, int x, int y, bool seated = false);
    int stationDoorX(int x) const;
    int trainX() const;
    bool near(const Npc& n, int d) const;
    void jolt();
};

struct Result {
    Outcome outcome = Outcome::None;
    std::string cause;
    std::string summary;
};

// Plays a whole trip on the level's printer and returns how it went.
Result runMetroMinigame(PlayerNS::Player* player, Level* level, const std::string& from,
                        const std::string& to, const std::string& after, bool dodgedFare);

// a one-line account of the trip for the message line afterwards
std::string summarize(const MetroGame& g);

} // namespace MetroNS

#endif
