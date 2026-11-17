#ifndef SUBTITLES_HPP
#define SUBTITLES_HPP

// Film-style captions at the bottom of the screen: a short, centred line saying what just
// happened (a throw, a punch, who you ran into, why you are off to the front, the drones
// overhead). It shows for a moment and fades. It does not replace the message line above the
// map; it is the one-glance version of it.

#include <cstdint>
#include <string>
#include <vector>

class Level;
class Printer;
namespace PlayerNS { class Player; }
namespace VisionNS { class Mask; }

namespace Subtitles {

constexpr uint32_t SHOW_MS = 2600;
// turn-based play has no frames: each turn counts as this much time
constexpr uint32_t TURN_MS = 700;

// Shows `text`, replacing whatever was showing. `kind` names the sort of event: the same kind is
// not shown again until `cooldownMs` has passed (the militia met twice a second, every punch of
// a brawl). False when the cooldown kept it off the screen.
bool say(const std::string& text, const std::string& kind = "", uint32_t cooldownMs = 0, uint32_t showMs = SHOW_MS);
// shown after the current caption and the ones queued before it have run out (a few lines
// of explanation in a row); a say() in between goes first and the queue waits
void queue(const std::string& text, uint32_t showMs = SHOW_MS);
// what is on screen now, "" for nothing
const std::string& current();
// ms until it disappears
uint32_t leftMs();
// time passes: the caption ages and the cooldowns run out
void tick(uint32_t ms);
// nothing showing, no cooldowns, the history emptied (a new game, the tests)
void reset();
// every caption shown since reset, oldest first
const std::vector<std::string>& history();

// on the way to the front: the draft board, the court's sentence or the FSB's black Aurus
enum class WarRoute { Draft, Court, Fsb };
void toTheFront(WarRoute how);

// the start of a game: the hands beside the map explained in a few captions; `introKey`
// opens the animated walk-through
std::vector<std::string> handsTutorial(char introKey);

// the subtitle for meeting someone: the militia, a soldier, the FSB, a thief... "" for nobody
// worth a caption
std::string meetingLine(int personType);
// people in sight near the player get their caption, each sort once in a while
void noticePeople(Level& level, const PlayerNS::Player& player, const VisionNS::Mask* mask);

}

#endif
