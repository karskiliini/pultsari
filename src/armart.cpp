#include "armart.hpp"
#include <algorithm>

// Each picture is a raw string, one row per line, padded with spaces to ARM_W. Everything is
// drawn for the LEFT arm (outer edge on the left, thumb towards the map on the right) except the
// pistol and the rifle grip, which are drawn for the right hand. mirrorRow flips the other side.

namespace {

// ---- sleeves, rows 0-8 ----

// 0: the poster's ragged work jacket, frayed holes and a frayed cuff
const char* SLEEVE0 = R"(
|%%%:%%%;%%%|
|%%( )%%%%%%|
|%%%%'%%%%:%|
|%;%%%%%/ \%|
 |%%%%%\_/%|
 |%%:%%%%%%|
 |%( )%%;%%|
 /%%%%%%%%%\
 ',|`,|',|`,
)";

// 1: bigger holes, rags hanging from the cuff
const char* SLEEVE1 = R"(
|%%/  \%;%%%|
|%:\__/%%%%%|
|%%%%%%/ ' \|
|%/ \%%\_,_/|
 |\_/%;%%%%|
 |%%%/   \%|
 |%;%\_,_/%|
 /%\/%%\/%%\
 ;|  '| ; ||
)";

// 2: torn off at the elbow, the bare forearm below the torn edge
const char* SLEEVE2 = R"(
|%( )%%;%%%%|
|%%%%%%/ \%%|
|%;%%%%\_/%%|
|/\%/\/%%/\/|
 ' ;| ' ;|'
 |   '    |
 |  .   , |
  |  '   |
  | ,    |
)";

// 3: almost no sleeve left, the arm bloody
const char* SLEEVE3 = R"(
|%/\%/ \/%\|
|/ ;\/ ' |/
 |  ,  '   |
 | '   .  ,|
 |.  '   , |
  | , .'  |
  |'   ,  |
  | .  ' ,|
   |' ,  |
)";

// ---- hands, rows 9-21 ----

const char* HAND_EMPTY = R"(
   |  ' |
   | .  |
  /  ,   \
 /  '  .  \
|  .   '   \
|_,__,__,_ |\
|| || || ||\ \
|| || || || )/
|| || || ||
|| || || ||
(_)(_)(_)||
 '  '  ' (_)
          '
)";

// the fist round a bottle neck; the bottle body (rows 15-21) is drawn by bottleRows
const char* HAND_BOTTLE = R"(
   |  ' |
  /  .  ,\_
| ,__ __ __ \
|(__(__(__|_/
  _|  |_
 /      \
)";

const char* HAND_CAN = R"(
   |  ' |
  /  .  ,\_
| ,__ __ __ \
|(__(__(__|_/
 |======|
 |JAGUAR|
 | //// |
 |//// /|
 |/// //|
 |// ///|
 |======|
 '------'
)";

// drawn for the RIGHT hand: a Makarov hanging muzzle down
const char* HAND_PISTOL = R"(
     | '  |
    _/  .  \
   / __ __ _\
 _|=(__(__(_)|
|  =|\______/
|  =| \ \
|   |_/ /
|   |
|   |
|   |
 \_o/
)";

// drawn for the LEFT hand: the rifle's handguard in the fist, barrel down
const char* HAND_AK_FORE = R"(
   |  ' |
  /  .  ,\_
| ,__ __ __ \
|(__(__(__|_/
  |::::|
  |::::|
  '-==-'
   |=||
   |=||
   |=||
   |=|]
   |_||
   [==]
)";

// drawn for the RIGHT hand: the rifle's grip in the fist, magazine curving down
const char* HAND_AK_GRIP = R"(
 |::| | '  |
 |::|_/ .   \
 |  |/ __ __ \
 |  |(__(__(_)|
 |  |\______/
 | o||\
 |  | \ \
 |  |  \ \
 |  |   ) )
 |  |  / /
 |__| '-'
  ||
  ||
)";

// anything else: a fist, the thing's name under it
const char* HAND_FIST = R"(
   |  ' |
   | .  |
   |  , |
  /  .  ,\_
| ,__ __ __ \
|(__(__(__|_/
 \________/
)";

// the bottle body: 6 liquid rows inside columns 2-7, the label on the second
const int GLASS_TOP = 15, LABEL_ROW = 16, GLASS_ROWS = 6;

std::vector<std::string> rowsOf(const char* art, size_t count)
{
    std::vector<std::string> out;
    std::string s(art);
    size_t pos = s[0] == '\n' ? 1 : 0;
    while (out.size() < count) {
        size_t nl = s.find('\n', pos);
        std::string row = nl == std::string::npos ? (pos < s.size() ? s.substr(pos) : "") : s.substr(pos, nl - pos);
        row.resize(ARM_W, ' ');
        out.push_back(row);
        pos = nl == std::string::npos ? s.size() : nl + 1;
    }
    return out;
}

// rows 15-21 of a bottle holding `fill` of 6 rows, labelled
std::vector<std::string> bottleRows(int fill, const std::string& label)
{
    std::vector<std::string> out;
    for (int i = 0; i < GLASS_ROWS; ++i) {
        const int row = GLASS_TOP + i;
        const bool wet = i >= GLASS_ROWS - fill;
        const char liquid = i == GLASS_ROWS - fill ? '~' : '#';
        std::string inside(6, wet ? liquid : ' ');
        if (row == LABEL_ROW && wet) inside.replace(1, label.size(), label);
        std::string r = " |" + inside + "|";
        r.resize(ARM_W, ' ');
        out.push_back(r);
    }
    std::string base = " \\______/";
    base.resize(ARM_W, ' ');
    out.push_back(base);
    return out;
}

// thing names are ASCII but for the A-umlaut; never let anything else into a picture
std::string asciiName(const char* name)
{
    std::string out;
    for (const char* p = name; *p; ++p) {
        const unsigned char c = *p;
        if (c == 0xC3 && (p[1] == '\x84' || p[1] == '\xA4')) { out += 'A'; ++p; }
        else if (c == 0xC3 && (p[1] == '\x96' || p[1] == '\xB6')) { out += 'O'; ++p; }
        else if (c >= 0x20 && c < 0x7f) out += (char)c;
    }
    return out.substr(0, ARM_W);
}

// rows 9-21 of the left arm's hand
std::vector<std::string> leftHand(const Held& held)
{
    std::vector<std::string> hand;
    switch (held.what) {
        case Thing::None: return rowsOf(HAND_EMPTY, 13);
        case Thing::Vodka: case Thing::Tyhja: case Thing::Kalja: {
            hand = rowsOf(HAND_BOTTLE, 6);
            const int fill = held.what == Thing::Vodka ? std::min<int>(held.fill, GLASS_ROWS) : held.what == Thing::Kalja ? GLASS_ROWS : 0;
            for (const auto& r : bottleRows(fill, held.what == Thing::Kalja ? "PIVO" : "VODKA")) hand.push_back(r);
            return hand;
        }
        case Thing::Lonkka: return rowsOf(HAND_CAN, 13);
        case Thing::Pistooli: hand = rowsOf(HAND_PISTOL, 13); break;
        case Thing::Rynkky: return rowsOf(HAND_AK_FORE, 13);
        default: {
            hand = rowsOf(HAND_FIST, 13);
            const std::string name = asciiName(thingName(held.what));
            hand[12] = std::string((ARM_W - name.size()) / 2, ' ') + name;
            hand[12].resize(ARM_W, ' ');
            return hand;
        }
    }
    for (auto& r : hand) r = mirrorRow(r);  // the pistol is drawn for the right hand
    return hand;
}

// stage 3: blood specks on the wrist and the back of the hand, inside its outline
void bloody(std::vector<std::string>& pic)
{
    static const char SPECKS[] = "',.";
    int n = 0;
    for (int row = 9; row <= 11; ++row)
        for (int c = 4; c <= 7; ++c)       // inside every wrist
            if (pic[row][c] == ' ' && (row * 5 + c) % 3 != 1) pic[row][c] = SPECKS[n++ % 3];
}

// a mirrored label reads backwards: turn its letters the right way again
std::string readable(std::string row)
{
    for (size_t a = 0; a < row.size(); ) {
        size_t b = a;
        while (b < row.size() && row[b] >= 'A' && row[b] <= 'Z') ++b;
        if (b > a) { std::reverse(row.begin() + a, row.begin() + b); a = b; }
        else ++a;
    }
    return row;
}

} // namespace

int sleeveStage(uint32_t health, bool usedCoat)
{
    const int stage = health >= 8 ? 0 : health >= 5 ? 1 : health >= 2 ? 2 : 3;
    return usedCoat && stage < 1 ? 1 : stage;
}

std::string mirrorRow(const std::string& row)
{
    static const std::string from = "/\\()<>[]{}`'", to = "\\/)(><][}{'`";
    std::string out(row.rbegin(), row.rend());
    for (char& c : out) {
        const size_t i = from.find(c);
        if (i != std::string::npos) c = to[i];
    }
    return out;
}

bool armRowBloody(int stage, int row)
{
    return stage == 3 && row >= 2 && row <= 11;
}

std::vector<std::string> armPicture(bool leftArm, const Held& held, int stage)
{
    static const char* SLEEVES[] = { SLEEVE0, SLEEVE1, SLEEVE2, SLEEVE3 };
    if (stage < 0) stage = 0;
    if (stage > 3) stage = 3;
    std::vector<std::string> pic = rowsOf(SLEEVES[stage], 9);
    // the rifle shows its grip in the right hand, its handguard in the left
    std::vector<std::string> hand = (!leftArm && held.what == Thing::Rynkky) ? rowsOf(HAND_AK_GRIP, 13) : leftHand(held);
    if (!leftArm && held.what == Thing::Rynkky) for (auto& r : hand) r = mirrorRow(r);
    for (const auto& r : hand) pic.push_back(r);
    if (stage == 3) bloody(pic);
    if (!leftArm) for (auto& r : pic) r = readable(mirrorRow(r));
    return pic;
}
