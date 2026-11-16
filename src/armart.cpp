#include "armart.hpp"
#include <algorithm>
#include <cassert>

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
const char* SLEEVE0_C = R"(
JJJJJJJJJJJJJ
JJJJ JJJJJJJJ
JJJJJJJJJJJJJ
JJJJJJJJJ JJJ
 JJJJJJJJJJJ
 JJJJJJJJJJJ
 JJJ JJJJJJJ
 JJJJJJJJJJJ
 JJJJJJJJJJJ
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
const char* SLEEVE1_C = R"(
JJJJ  JJJJJJJ
JJJJJJJJJJJJJ
JJJJJJJJ J JJ
JJJ JJJJJJJJJ
 JJJJJJJJJJJ
 JJJJJ   JJJ
 JJJJJJJJJJJ
 JJJJJJJJJJJ
 JJ  JJ J JJ
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
const char* SLEEVE2_C = R"(
JJJ JJJJJJJJJ
JJJJJJJJ JJJJ
JJJJJJJJJJJJJ
JJJJJJJJJJJJJ
 J JJ J JJJ
 S   S    S
 S  S   S S
  S  S   S
  S S    S
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
const char* SLEEVE3_C = R"(
JJJJJJ JJJJJ
JJ JJJ J JJ
 S  R  R   S
 S R   R  RS
 SR  R   R S
  S R RR  S
  SR   R  S
  S R  R RS
   SR R  S
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
const char* HAND_EMPTY_C = R"(
   S  S S
   S S  S
  S  S   S
 S  S  S  S
S  S   S   S
SSSSSSSSSS SS
SS SS SS SSS S
SS SS SS SS SS
SS SS SS SS
SS SS SS SS
SSSSSSSSSSS
 S  S  S SSS
          S
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
const char* HAND_BOTTLE_C = R"(
   S  S S
  S  S  SSS
S SSS SS SS S
SSSSSSSSSSSSS
  GG  GG
 G      G
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
const char* HAND_CAN_C = R"(
   S  S S
  S  S  SSS
S SSS SS SS S
SSSSSSSSSSSSS
 CCCCCCCC
 CLLLLLLC
 C CCCC C
 CCCCC CC
 CCCC CCC
 CCC CCCC
 CCCCCCCC
 CCCCCCCC
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
const char* HAND_PISTOL_C = R"(
     S S  S
    SS  S  S
   S SS SS SS
 MMMSSSSSSSSSS
M  MMSSSSSSSS
M  MM M M
M   MMM M
M   M
M   M
M   M
 MMMM
)";

// drawn for the LEFT hand: the raised rifle's handguard in the fist, the barrel on down to the
// front sight
const char* HAND_AK_FORE = R"(
   |  ' |
  /  .  ,\_
| ,__ __ __ \
|(__(__(__|_/
  |====|
  |    |
  |====|
  '-==-'
    ||
    ||
    ||
   _||_
  `-||-'
)";
const char* HAND_AK_FORE_C = R"(
   S  S S
  S  S  SSS
S SSS SS SS S
SSSSSSSSSSSSS
  WWWWWW
  W    W
  WWWWWW
  MMMMMM
    MM
    MM
    MM
   MMMM
  MMMMMM
)";

// drawn for the RIGHT hand: the fist round the pistol grip, the stock up along the forearm, the
// receiver under the fist with the long 30-round magazine out to the map side, gently curved
// towards the muzzle, then the handguard and the barrel down
const char* HAND_AK_GRIP = R"(
     | '  |%%
    _/  . \%%
   / __ __ _\%
  (__(__(__)|%
   \______/=|
  _____|====|
,'     |====|
/  ,--'|____|
\_/    |====|
       |    |
       |====|
         ||
       `-||-'
)";
const char* HAND_AK_GRIP_C = R"(
     S S  SWW
    SS  S SWW
   S SS SS SSW
  SSSSSSSSSSSW
   SSSSSSSSWW
  MMMMMMMMMMM
MM     MMMMMM
M  MMMMMMMMMM
MMM    WWWWWW
       W    W
       WWWWWW
         MM
       MMMMMM
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
const char* HAND_FIST_C = R"(
   S  S S
   S S  S
   S  S S
  S  S  SSS
S SSS SS SS S
SSSSSSSSSSSSS
 SSSSSSSSSS
)";

// the bottle body: 6 liquid rows inside columns 2-7, the label on the second
const int GLASS_TOP = 15, LABEL_ROW = 16, GLASS_ROWS = 6;

// a picture with its colour mask: one letter per cell (see armColours), space = no colour
struct Art {
    std::vector<std::string> pic, mask;
    void add(const Art& more)
    {
        pic.insert(pic.end(), more.pic.begin(), more.pic.end());
        mask.insert(mask.end(), more.mask.begin(), more.mask.end());
    }
};

std::vector<std::string> rowsOf(const char* art, size_t count)
{
    std::vector<std::string> out;
    std::string s(art);
    size_t pos = s[0] == '\n' ? 1 : 0;
    while (out.size() < count) {
        size_t nl = s.find('\n', pos);
        std::string row = nl == std::string::npos ? (pos < s.size() ? s.substr(pos) : "") : s.substr(pos, nl - pos);
        assert(row.size() <= (size_t)ARM_W);  // a wider row would lose its right edge to the padding
        row.resize(ARM_W, ' ');
        out.push_back(row);
        pos = nl == std::string::npos ? s.size() : nl + 1;
    }
    return out;
}

Art artOf(const char* pic, const char* mask, size_t count) { return { rowsOf(pic, count), rowsOf(mask, count) }; }

std::string reversed(const std::string& row) { return std::string(row.rbegin(), row.rend()); }

// rows 15-21 of a bottle holding `fill` of 6 rows, labelled; liquid cells take `liquidColour`
Art bottleRows(int fill, const std::string& label, char liquidColour)
{
    Art out;
    for (int i = 0; i < GLASS_ROWS; ++i) {
        const int row = GLASS_TOP + i;
        const bool wet = i >= GLASS_ROWS - fill;
        const char liquid = i == GLASS_ROWS - fill ? '~' : '#';
        std::string inside(6, wet ? liquid : ' ');
        std::string colour(6, wet ? liquidColour : ' ');
        if (row == LABEL_ROW && wet) {
            inside.replace(1, label.size(), label);
            colour.replace(1, label.size(), std::string(label.size(), 'L'));
        }
        std::string r = " |" + inside + "|", c = " G" + colour + "G";
        r.resize(ARM_W, ' ');
        c.resize(ARM_W, ' ');
        out.pic.push_back(r);
        out.mask.push_back(c);
    }
    std::string base = " \\______/", baseC = " GGGGGGGG";
    base.resize(ARM_W, ' ');
    baseC.resize(ARM_W, ' ');
    out.pic.push_back(base);
    out.mask.push_back(baseC);
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
Art leftHand(const Held& held)
{
    Art hand;
    switch (held.what) {
        case Thing::None: return artOf(HAND_EMPTY, HAND_EMPTY_C, 13);
        case Thing::Vodka: case Thing::Tyhja: case Thing::Kalja: {
            hand = artOf(HAND_BOTTLE, HAND_BOTTLE_C, 6);
            const bool beer = held.what == Thing::Kalja;
            const int fill = held.what == Thing::Vodka ? std::min<int>(held.fill, GLASS_ROWS) : beer ? GLASS_ROWS : 0;
            hand.add(bottleRows(fill, beer ? "PIVO" : "VODKA", beer ? 'A' : 'V'));
            return hand;
        }
        case Thing::Lonkka: return artOf(HAND_CAN, HAND_CAN_C, 13);
        case Thing::Pistooli: hand = artOf(HAND_PISTOL, HAND_PISTOL_C, 13); break;
        case Thing::Rynkky: return artOf(HAND_AK_FORE, HAND_AK_FORE_C, 13);
        default: {
            hand = artOf(HAND_FIST, HAND_FIST_C, 13);
            const std::string name = asciiName(thingName(held.what));
            const size_t at = (ARM_W - name.size()) / 2;
            hand.pic[12] = std::string(at, ' ') + name;
            hand.mask[12] = std::string(at, ' ') + std::string(name.size(), 'L');
            hand.pic[12].resize(ARM_W, ' ');
            hand.mask[12].resize(ARM_W, ' ');
            return hand;
        }
    }
    // the pistol is drawn for the right hand
    for (auto& r : hand.pic) r = mirrorRow(r);
    for (auto& r : hand.mask) r = reversed(r);
    return hand;
}

// stage 3: blood specks on the wrist and the back of the hand, inside its outline
void bloody(Art& art)
{
    static const char SPECKS[] = "',.";
    int n = 0;
    for (int row = 9; row <= 11; ++row)
        for (int c = 4; c <= 7; ++c)       // inside every wrist
            if (art.pic[row][c] == ' ' && (row * 5 + c) % 3 != 1) {
                art.pic[row][c] = SPECKS[n++ % 3];
                art.mask[row][c] = 'R';
            }
}

// a mirrored label reads backwards: turn its letters the right way again, colours with them
void readable(std::string& row, std::string& mask)
{
    for (size_t a = 0; a < row.size(); ) {
        size_t b = a;
        while (b < row.size() && row[b] >= 'A' && row[b] <= 'Z') ++b;
        if (b > a) {
            std::reverse(row.begin() + a, row.begin() + b);
            std::reverse(mask.begin() + a, mask.begin() + b);
            a = b;
        } else {
            ++a;
        }
    }
}

Art arm(bool leftArm, const Held& held, int stage)
{
    static const char* SLEEVES[] = { SLEEVE0, SLEEVE1, SLEEVE2, SLEEVE3 };
    static const char* SLEEVE_COLOURS[] = { SLEEVE0_C, SLEEVE1_C, SLEEVE2_C, SLEEVE3_C };
    if (stage < 0) stage = 0;
    if (stage > 3) stage = 3;
    Art art = artOf(SLEEVES[stage], SLEEVE_COLOURS[stage], 9);
    // the rifle shows its grip in the right hand, its handguard in the left
    if (!leftArm && held.what == Thing::Rynkky) {
        Art grip = artOf(HAND_AK_GRIP, HAND_AK_GRIP_C, 13);
        for (auto& r : grip.pic) r = mirrorRow(r);
        for (auto& r : grip.mask) r = reversed(r);
        art.add(grip);
    } else {
        art.add(leftHand(held));
    }
    if (stage == 3) bloody(art);
    if (!leftArm)
        for (size_t r = 0; r < art.pic.size(); ++r) {
            art.pic[r] = mirrorRow(art.pic[r]);
            art.mask[r] = reversed(art.mask[r]);
            readable(art.pic[r], art.mask[r]);
        }
    return art;
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
    return arm(leftArm, held, stage).pic;
}

std::vector<std::string> armColours(bool leftArm, const Held& held, int stage)
{
    return arm(leftArm, held, stage).mask;
}
