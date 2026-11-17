#include "printer.hpp"
#include "translate.hpp"
#include <chrono>
#include "player.hpp"
#include "item.hpp"
#include "input.hpp"
#include "building.hpp"
#include "animation.hpp"
#include "mask.hpp"
#include "pathmask.hpp"
#include "level.hpp"
#include "vehicle.hpp"
#include "aagun.hpp"
#include "armart.hpp"
#include "events.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <cmath>

using std::vector;
using std::cout;
using std::endl;
using InputNS::Input;
using PlayerNS::Player;
using std::string;

static string tokenizeStr(string& str)
{
    auto it = str.find("\n");
    if (it != std::string::npos) {
        string token = str.substr(0, it);
        str = str.substr(it+1);
        return token;
    }
    return str;
}

void Printer::setMessage(const std::string& message)
{
    msg = message;
    msgTtl = MSG_FRAMES;
}

void Printer::showMessage(std::string message, Level* level, bool waitKey)
{
    message = withTranslation(message);
    if (message == "") {
        printerRefresh();
        return;
    }

    // in realtime nothing may block on a keypress: show the latest message on one line
    if (common::realtimeEnabled) {
        for (auto& ch : message) if (ch == '\n') ch = ' ';
        msg = message;
        msgShown = true;
        msgTtl = MSG_FRAMES;
        cursorHome();
        printMessage();
        printerRefresh();
        return;
    }

    bool tokenize = true;
    uint32_t limit = 3;
    bool waited = false;

    while (tokenize && limit-- > 0) {
        tokenize = false;

        // see if message is multiline
        auto it = message.find("\n");
        if (it != std::string::npos) {
            tokenize = true;
        }

        if (msgShown && waitKey && !waited) {
            printMore();
            printerRefresh();
            Input::waitKey();
        }

        msg = tokenizeStr(message);
        if (!msgShown) msgShown = true;

        cursorHome();
        printMessage();

        if (tokenize && !waited)
        {
            printMore();
            printerRefresh();
            Input::waitKey();
            waited = true;
        }
    }
    printerRefresh();
}

void Printer::removeMessage()
{
    if (common::realtimeEnabled && msgTtl > 0) {
        --msgTtl;
        return;
    }
    msg = "";
    if (msgShown) msgShown = false;
}

const Person* Printer::findPerson(const vector<const Person*>& row, uint32_t x) const
{
    for (const auto& p : row) {
        if (p->coord.x == x && !p->hiddenFromView()) return p;
    }

    return nullptr;
}

const Item* Printer::findItem(const vector<const Item*>& row, uint32_t x) const
{
    for (const auto& p : row) {
        if (p->coord.x == x) return p;
    }

    return nullptr;
}

string Printer::inventoryAligned(uint32_t value) const
{
    if (value < 10) {
        char str[10];
        snprintf(str, sizeof(str), "%d", value);
        return string(str) + "  ";
    } else if (value < 100) {
        char str[10];
        snprintf(str, sizeof(str), "%d", value);
        return string(str) + " ";
    } else {
        return ".. ";
    }
}

string Printer::moneyAligned(uint32_t value) const
{
    if (value < 10) {
        char str[10];
        snprintf(str, sizeof(str), "%d", value);
        return string(str) + "    ";
    } else if (value < 100) {
        char str[10];
        snprintf(str, sizeof(str), "%d", value);
        return string(str) + "   ";
    } else if (value < 1000) {
        char str[10];
        snprintf(str, sizeof(str), "%d", value);
        return string(str) + "  ";
    } else if (value < 10000) {
        char str[10];
        snprintf(str, sizeof(str), "%d", value);
        return string(str) + " ";
    } else if (value < 100000) {
        char str[10];
        snprintf(str, sizeof(str), "%d", value);
        return string(str) + "";
    } else {
        return " oo  ";
    }
}

string Printer::promilleAligned(uint32_t value) const
{
    string ret = "";
    uint32_t full = value / 10;
    if (full == 0)
    {
        ret = "  .";
    } else if (full < 10)
    {
        char str[10];
        snprintf(str, sizeof(str), "%d", full);
        ret = " " + string(str) + ".";
    } else if (full < 100) {
        char str[10];
        snprintf(str, sizeof(str), "%d", full);
        ret = "" + string(str) + ".";
    } else return "KÄNNI";

    uint32_t parts = value % 10;
    {
        char str[10];
        snprintf(str, sizeof(str), "%d", parts);
        ret += string(str);
    }
    return ret;
}

// The front: dug-out earth in the trenches with sandbags along the lips, craters and wire.
bool trenchCell(const Level* l, const Coord& c, string& glyph, const char*& color)
{
    const Trench t = l->trenchAt(c);
    switch (t) {
        case Trench::None:
            return false;
        case Trench::Crater:
            glyph = "o";
            color = "38;5;94;48;5;235";
            return true;
        case Trench::Wire:
            glyph = (c.x + c.y) % 2 ? "#" : "x";
            color = "38;5;247";
            return true;
        case Trench::Own:
        case Trench::Enemy: {
            const bool own = t == Trench::Own;
            // a lip: any side opens onto the ground above
            bool lip = false;
            const int dirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
            for (const auto& d : dirs) {
                const int x = (int)c.x + d[0], y = (int)c.y + d[1];
                if (x < 0 || y < 0) continue;
                const Trench n = l->trenchAt(Coord{ (uint32_t)x, (uint32_t)y });
                if (n != Trench::Own && n != Trench::Enemy) { lip = true; break; }
            }
            if (lip) {
                glyph = "\xE2\x96\x92";
                color = own ? "38;5;144;48;5;58" : "38;5;101;48;5;237";
            } else {
                // duckboards along the bottom now and then
                glyph = (c.x * 7 + c.y * 13) % 11 == 0 ? "=" : " ";
                color = own ? "38;5;94;48;5;58" : "38;5;94;48;5;237";
            }
            return true;
        }
    }
    return false;
}

namespace {

// Streets: grey asphalt, a yellow centre line, zebra crossings next to junctions
// and the street name painted along the kerb lane.
bool roadCell(const Level* l, const Coord& c, string& glyph, const char*& color)
{
    const Road* h = nullptr;
    const Road* v = nullptr;
    l->roadsAt(c, h, v);
    if (!h && !v) return false;

    glyph = " ";
    color = "48;5;236";
    if (h && v) return true; // junction

    const Road& r = h ? *h : *v;
    const uint32_t along = r.horizontal ? c.x : c.y;
    const uint32_t across = (r.horizontal ? c.y : c.x) - r.pos;

    for (const auto& other : l->roads) {
        if (other.horizontal == r.horizontal) continue;
        if (along + 1 == other.pos || along == other.pos + 3) {
            glyph = r.horizontal ? "\xE2\x96\x88" : "\xE2\x96\x80";
            color = (across % 2 == 0) ? "97;48;5;236" : "37;48;5;236";
            return true;
        }
    }
    if (across == 1) {
        if (along % 4 < 2) { glyph = r.horizontal ? "\xE2\x94\x80" : "\xE2\x94\x82"; color = "33;48;5;236"; }
        return true;
    }
    if (across == 0) {
        const uint32_t idx = along % 70;
        if (idx >= 3 && idx - 3 < r.name.size()) {
            glyph = string(1, r.name[idx - 3]);
            color = "37;48;5;236";
        }
    }
    return true;
}

// Drone sprites: a small aircraft, nose pointing where it flies.
const vector<vector<const char*>>& droneSprite(const Drone& d)
{
    static const vector<vector<const char*>> east = {
        { " ", "\xE2\x96\x99", " ", " " },
        { "\xE2\x96\x90", "\xE2\x96\x88", "\xE2\x96\x88", "\xE2\x96\xB6" },
        { " ", "\xE2\x96\x9B", " ", " " } };
    static const vector<vector<const char*>> west = {
        { " ", " ", "\xE2\x96\x9F", " " },
        { "\xE2\x97\x80", "\xE2\x96\x88", "\xE2\x96\x88", "\xE2\x96\x8C" },
        { " ", " ", "\xE2\x96\x9C", " " } };
    static const vector<vector<const char*>> north = {
        { " ", "\xE2\x96\xB2", " " },
        { "\xE2\x97\xA2", "\xE2\x96\x88", "\xE2\x97\xA3" },
        { " ", "\xE2\x96\x80", " " } };
    static const vector<vector<const char*>> south = {
        { " ", "\xE2\x96\x84", " " },
        { "\xE2\x97\xA5", "\xE2\x96\x88", "\xE2\x97\xA4" },
        { " ", "\xE2\x96\xBC", " " } };
    if (std::fabs(d.vx) >= std::fabs(d.vy)) return d.vx >= 0 ? east : west;
    return d.vy >= 0 ? south : north;
}

// glyph of any drone covering (x, y), with `shadow` set for its shadow on the ground
// and `fpv` for a small FPV quadcopter (2 when it is diving)
const char* droneCell(const Level* l, uint32_t x, uint32_t y, bool& shadow, int& fpv)
{
    shadow = false;
    fpv = 0;
    for (const auto& d : l->drones) {
        if (d.fpv) {
            // one cell: the X of the four rotor arms, flying low with its shadow right under
            const int dx = (int)std::lround(d.x), dy = (int)std::lround(d.y);
            if ((int)x == dx && (int)y == dy) { fpv = d.diving ? 2 : 1; return "\xE2\x95\xB3"; }   // ╳
            if ((int)x == dx + 1 && (int)y == dy + 1) shadow = true;
            continue;
        }
        const auto& sprite = droneSprite(d);
        const int w = sprite[0].size();
        const int left = (int)std::lround(d.x) - w / 2;
        const int top = (int)std::lround(d.y) - 1;
        const int sx = (int)x - left, sy = (int)y - top;
        if (sy >= 0 && sy < 3 && sx >= 0 && sx < w && sprite[sy][sx][0] != ' ') return sprite[sy][sx];
        // the shadow falls a little south-east of the drone
        const int hx = sx - 2, hy = sy - 1;
        if (hy >= 0 && hy < 3 && hx >= 0 && hx < w && sprite[hy][hx][0] != ' ') shadow = true;
    }
    return nullptr;
}

// A person's colour is their clothing or skin (xterm 256-colour palette).
const char* personColor(const Person* p)
{
    if (p->clothing) return p->clothing;
    switch (p->type) {
        case pelaaja:
            if (p->stats.health == 0) return "1;31";   // the body
            return static_cast<const Player*>(p)->isVeteran()
                ? "1;38;5;107"          // faded front-line khaki
                : "1;38;5;180";         // the pultsari's shabby beige coat
        case putin:        return "1;38;5;223;48;5;17"; // pale face, navy suit
        case medvedev:     return "1;38;5;203";         // drink-flushed red face
        case shoigu:
        case gerasimov:    return "1;38;5;71";          // general's green with gold
        case fsb:          return "1;38;5;245";         // black suit, grey in the dark
        case poliisi:      return "1;38;5;68";          // politsija blue-grey
        case sotilas:      return static_cast<const Sotilas*>(p)->officer
                               ? "1;38;5;106"           // officer: brighter green, bold
                               : "38;5;64";             // camouflage green
        case ilmatorjunta: return "38;5;64";
        case veteraani:    return "38;5;143";           // old khaki
        case vartija:      return "38;5;250";           // grey security uniform
        case mummo:        return "38;5;168";           // pink headscarf
        case juoppo:       return "38;5;137";           // grubby brown
        case yka:          return "38;5;180";           // another shabby coat
        case jengi:        return "1;38;5;27";          // blue Adidas tracksuit
        case skinhead:     return "38;5;223";           // pale shaved head
        case ryostaja:     return "38;5;240";           // black hoodie
        case varas:        return "38;5;246";           // grey hoodie
        case vanki:        return "38;5;101";           // worn prison black-olive
        case diileri:      return "38;5;135";           // shiny purple tracksuit
        case avustaja:     return "38;5;216";           // flushed drunk face
        case kotiaiti:     return "38;5;175";           // pink velour tracksuit
        case lapsi:        return "1;38;5;214";         // bright orange puffer jacket
        case rotta:        return "38;5;246";           // grey fur
        case pankkirosvo:  return p->armor ? "1;38;5;94" : "1;38;5;160"; // red balaclava; brown vest
        case rekryytti:    return "38;5;101";  // a uniform that does not fit
        case korealainen:  return "38;5;65";   // dark green, pressed
        case myyja:        return "1;38;5;31"; // blue shop coat
        case ukrainalainen: return "1;38;5;220";       // the yellow tape on a green sleeve
        default:           return "37";
    }
}

// Things you can carry are white; scenery keeps its natural colour.
const char* itemColor(ItemType t)
{
    switch (t) {
        case EPaska:
        case EOksennus:    return "2;32";
        case ENextLevel:   return "1;97;44";
        case EViinaKellari:return "1;97;41";
        case EPensas:      return "32";
        case ERoskis:      return "90";
        case EBusStop:     return "1;93;44";
        case EPutinPoyta:  return "37";
        case EKartta:      return "1;33";
        case ERoska:       return "38;5;94";
        case ENeula:       return "2;37";
        case ESirpaleet:   return "96";
        case ERuumis:      return "31";
        default:           return "1;97";
    }
}

const char* buildingColor(const Building* b, const string& glyph)
{
    if (b->type == EGasPump) return "1;31";   // red pumps
    if (glyph == "\xE2\x96\x88") return b->type == EKremlin ? "31" : "90";
    if (glyph == "\xE2\x96\x9E") return "37";
    if (glyph == "\xE2\x96\x91") return "33";
    if (glyph == "\xE2\x96\x92") return "2;36";
    if (glyph == "~") return "1;91";
    if (glyph == ";") return "2;37";
    return "1;97";
}

}

void Printer::printLine(Level* l, uint32_t y, const VisionNS::Mask* mask, PathNS::PathMask* pathmask)
{
    string line;
    string current;
    for (const auto& cell : rowCells(l, y)) {
        if (current != cell.second) {
            line += "\033[0m";
            if (!cell.second.empty()) line += "\033[" + cell.second + "m";
            current = cell.second;
        }
        line += cell.first;
    }
    line += "\033[0m";
    printChar(line);
}

// a flickering flame: glyph and colour change every ~0.1 s, differently in each cell
static void flameAt(uint32_t x, uint32_t y, string& c, const char*& color)
{
    static const char* flame[] = { "^", "*", "~", "\"", "^", "'", "*", "\xE2\x96\xB2" };
    static const char* heat[] = { "1;91", "1;93", "91", "33", "1;31", "1;93", "1;97", "91" };
    const uint32_t t = (uint32_t)(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() / 110);
    const uint32_t h = (x * 73856093u) ^ (y * 19349663u) ^ (t * 83492791u);
    c = flame[(h >> 3) % 8];
    color = heat[(h >> 7) % 8];
}

vector<std::pair<string, string>> Printer::rowCells(Level* l, uint32_t y)
{
    if (scene) {
        vector<std::pair<string, string>> cells = y < scene->size() ? (*scene)[y] : vector<std::pair<string, string>>();
        cells.resize(viewW, std::make_pair(string(" "), string()));
        return cells;
    }
    uint32_t worldY = y + (uint32_t)l->cameraY;
    vector<std::pair<string, string>> cells;
    cells.reserve(viewW);
    auto emit = [&](const string& glyph, const string& color) { cells.emplace_back(glyph, color); };

    struct RowAnim { uint32_t x; vector<string> chars; const char* color; };
    vector<RowAnim> rowAnims;
    if (common::animsEnabled) {
        for (const auto a : l->animations) {
            if (a->coord.y != worldY) continue;
            if (a->isBubble && bubblesAsOverlay) continue;
            if (!mask->visible(a->source)) continue;
            // speech bubbles fade from white to dark grey as they expire
            const char* color = a->color ? a->color : "1;91";
            if (a->isBubble) {
                const uint32_t left = a->frames ? a->ttl * 100 / a->frames : 0;
                color = left > 60 ? "1;97" : left > 35 ? "37" : left > 15 ? "90" : "2;90";
            }
            RowAnim r { a->coord.x, {}, color };
            for (size_t i = 0; i < a->text.size();) {
                const unsigned char b = a->text[i];
                size_t n = b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : b >= 0xC0 ? 2 : 1;
                r.chars.push_back(a->text.substr(i, n));
                i += n;
            }
            rowAnims.push_back(std::move(r));
        }
    }

    // burning hypermarkets have their own fire and smoke
    vector<Hypermarket*> burningMarkets;
    for (auto b : l->buildings) {
        if (b->type != EHypermarket) continue;
        Hypermarket* hm = dynamic_cast<Hypermarket*>(b);
        if (hm && hm->onFire) burningMarkets.push_back(hm);
    }

    // the building the player is standing in: from inside you don't see its name plate
    const Building* playerIn = nullptr;
    if (player) {
        for (auto b : l->buildings) {
            if (b->walkable && b->hitBuilding(player->coord) && !b->hitWall(player->coord)) { playerIn = b; break; }
        }
    }

    Item* thrownItem = nullptr;
    if (l->thrownItem && l->thrownItem->coord.y == worldY) {
        thrownItem = l->thrownItem;
    }

    vector<const Person*> persons;
    for (const auto& p : l->persons) {
        // at the wheel you are inside the car, not on top of it
        if (p == (const Person*)player && player->driving) continue;
        if (p->coord.y == worldY) {
            persons.push_back(p);
        }
    }

    vector<const Item*> items;
    for (const auto& i : l->items) {
        if (i->coord.y == worldY) {
            items.push_back(i);
        }
    }

    // tracers: tiny braille dots, 2x4 to a cell; a cell takes the colour of its hottest dot
    struct TracerCell { int x; uint8_t bits; uint32_t k; TracerKind kind; };
    vector<TracerCell> rowTracers;
    if (common::animsEnabled && !tracersAsOverlay) {
        static const uint8_t DOT[2][4] = { { 0x01, 0x02, 0x04, 0x40 }, { 0x08, 0x10, 0x20, 0x80 } };
        for (const auto& t : l->tracers) {
            for (uint32_t k = 0; k < TRACER_DOTS; ++k) {
                float fx, fy;
                if (!tracerDot(t, k, fx, fy)) continue;
                const long cx = std::lround(fx), cy = std::lround(fy);
                if (cx < 0 || cy != (long)worldY) continue;
                const int col = fx - (cx - 0.5f) >= 0.5f ? 1 : 0;
                const int row = std::max(0, std::min(3, (int)((fy - (cy - 0.5f)) * 4)));
                auto it = std::find_if(rowTracers.begin(), rowTracers.end(), [&](const TracerCell& tc) { return tc.x == (int)cx; });
                if (it == rowTracers.end()) rowTracers.push_back(TracerCell{ (int)cx, DOT[col][row], k, t.kind });
                else {
                    it->bits |= DOT[col][row];
                    if (k < it->k) { it->k = k; it->kind = t.kind; }
                }
            }
        }
    }
    string tracerColor;

    for (uint32_t vx = 0; vx < viewW; ++vx) {
        uint32_t x = vx + (uint32_t)l->cameraX;
        Coord coord { x, worldY };
        bool found = false;
        string c = " ";
        const char* color = "";

        // drones are up in the sky, above everything, fog included
        bool droneShadow = false;
        int fpv = 0;
        if (const char* g = droneCell(l, x, worldY, droneShadow, fpv)) {
            c = g;
            color = fpv == 2 ? "1;38;5;196" : fpv ? "1;38;5;214" : "1;38;5;250";
            found = true;
        }

        if (!found && !rowTracers.empty() && mask->visible(coord)) {
            for (const auto& tc : rowTracers) {
                if (tc.x != (int)x) continue;
                const uint32_t cp = 0x2800 + tc.bits;
                c = string{ (char)(0xE0 | (cp >> 12)), (char)(0x80 | ((cp >> 6) & 0x3F)), (char)(0x80 | (cp & 0x3F)) };
                tracerColor = "1;38;5;" + std::to_string(tracerColour(tc.k, tc.kind));
                color = tracerColor.c_str();
                found = true;
                break;
            }
        }

        for (auto it = rowAnims.rbegin(); it != rowAnims.rend(); ++it) {
            if (x >= it->x && x < it->x + it->chars.size()) {
                c = it->chars[x - it->x];
                color = it->color;
                found = true;
                break;
            }
        }

        // fire glows and smoke rises above the roofs: both show from a distance, even out of sight
        if (!found && !mask->visible(coord) && player && coord.distance(player->coord) <= 40) {
            if (l->events && l->events->hasFire(coord)) {
                flameAt(x, worldY, c, color);
                found = true;
            } else if (l->gasAt(coord) == 3) {
                c = "\xE2\x96\x92";
                color = (x + worldY) % 3 ? "38;5;240" : "38;5;243";
                found = true;
            }
        }

        if (!found && !mask->visible(coord)) {
            // a building's name plate shows whole as soon as any of its walls is in sight
            for (Building* b = l->buildingAt(coord); b; b = nullptr) {
                if (b->hitWall(coord) || coord.y != (b->wallTop + b->wallBot) / 2) continue;
                const string g = b->typeToChar(coord);
                if (g == " ") break;
                bool wallSeen = false;
                for (uint32_t wx = b->wallLeft; wx <= b->wallRight && !wallSeen; ++wx)
                    wallSeen = mask->visible(Coord{ wx, b->wallTop }) || mask->visible(Coord{ wx, b->wallBot });
                for (uint32_t wy = b->wallTop; wy <= b->wallBot && !wallSeen; ++wy)
                    wallSeen = mask->visible(Coord{ b->wallLeft, wy }) || mask->visible(Coord{ b->wallRight, wy });
                if (wallSeen) { c = g; color = buildingColor(b, c); found = true; }
                break;
            }
        }
        if (!found && !mask->visible(coord)) {
            // out of sight the streets still show faintly, so you can tell where you are
            const Road* h = nullptr;
            const Road* v = nullptr;
            l->roadsAt(coord, h, v);
            const Trench tr = l->trenchAt(coord);
            c = (h || v) ? " " : tr == Trench::Wire ? "x" : tr == Trench::Crater ? "o" : ".";
            color = (h || v) ? "48;5;234" : (tr == Trench::Own || tr == Trench::Enemy) ? "38;5;238;48;5;234" : "2;90";
            found = true;
        }

        if (thrownItem && thrownItem->coord.x == x) {
            c = thrownItem->typeToChar();
            color = "1;97";
            found = true;
        }

        // flames flicker every frame, over walls too: a burning building looks like one
        if (!found && l->events && l->events->hasFire(coord)) {
            flameAt(x, worldY, c, color);
            found = true;
        }

        // walls, doors, windows and name plates; an empty interior lets people and items show
        if (!found) {
            for (Building* b = l->buildingAt(coord); b; b = nullptr) {
                if (b->isBreach(coord) && l->getVehicleAt(coord)) break; // the car in the hole shows
                string g = b->typeToChar(coord);
                if (b == playerIn && !b->hitWall(coord) && !b->isBreach(coord)) g = " ";
                if (g != " " || !b->walkable) {
                    c = g;
                    color = buildingColor(b, c);
                    found = true;
                }
                break;
            }
        }

        if (!found) {
            const Person* p = findPerson(persons, x);
            if (p) {
                c = p->typeToChar();
                color = personColor(p);
                found = true;
            }
        }

        if (!found) {
            const Item* i = findItem(items, x);
            if (i) {
                c = i->typeToChar();
                color = itemColor(i->type);
                found = true;
            }
        }

        if (!found) {
            for (const auto& v : l->vehicles) {
                const int part = v->cellIndex(coord);
                if (part >= 0) {
                    // a blown-up car is a burnt-out shell; the gun draws its own wreck
                    const bool shell = v->wrecked && !dynamic_cast<const AAGun*>(v);
                    c = shell ? "\xE2\x96\x93" : v->glyphAt(part);
                    color = shell ? "38;5;240" : v->colorAt(part);
                    found = true;
                    break;
                }
            }
        }

        if (!found && l->events && l->events->hasFire(coord)) {
            c = "~";
            color = ((x + worldY) & 1) ? "1;91" : "1;93";
            found = true;
        }

        if (!found) {
            for (auto hm : burningMarkets) {
                for (const auto& s : hm->smokeSpots) {
                    if (s == coord) { c = ";"; color = "2;37"; found = true; break; }
                }
                for (const auto& f : hm->fireSpots) {
                    if (!found && f == coord) { flameAt(x, worldY, c, color); found = true; break; }
                }
                if (found) break;
            }
        }

        const bool groundVisible = mask->visible(coord);
        string col = color;
        // our own machine guns at the rear edge, pointing the wrong way
        if (!found && l->isMgNest(coord)) {
            c = "K";
            color = "1;38;5;196;48;5;52";
            found = true;
        }

        if (!found) {
            if (!trenchCell(l, coord, c, color)) roadCell(l, coord, c, color);
            col = color;
            if (groundVisible && col.empty()) {
                const Building* b = l->buildingAt(coord);
                if (b && b->walkable && !b->hitWall(coord)) col = "48;5;52";
            }
            // drifting gas: grey exhaust, yellowish toxic smoke
            if (groundVisible) {
                const int gasKind = l->gasAt(coord);
                if (gasKind && c == " ") {
                    c = gasKind == 1 ? "\xE2\x96\x91" : "\xE2\x96\x92";
                    col = gasKind == 2 ? "38;5;143" : gasKind == 3 ? ((x + worldY) % 3 ? "38;5;240" : "38;5;243") : "38;5;245";
                    if (string(color).find("48;") != string::npos) col += string(";") + color;
                }
            }
        } else if (col.find("48;") == string::npos && col != "2;90") {
            // people, items and cars on a street keep the asphalt underneath
            const Road* h = nullptr;
            const Road* v = nullptr;
            l->roadsAt(coord, h, v);
            if (h || v) col += col.empty() ? "48;5;236" : ";48;5;236";
            else if (l->trenchAt(coord) == Trench::Own) col += col.empty() ? "48;5;58" : ";48;5;58";
            else if (l->trenchAt(coord) == Trench::Enemy) col += col.empty() ? "48;5;237" : ";48;5;237";
        }

        if (droneShadow && groundVisible && col.find("48;") == string::npos) {
            col += col.empty() ? "48;5;233" : ";48;5;233";
        }

        emit(c, col);
    }
    return cells;
}

void Printer::print(Level* level)
{
    cursorHome();
    if (common::realtimeEnabled) {
        // draw the message first and erase only what follows it, so it never flickers
        printMessage();
        printChar("\033[K");
    }
    printChar("\n");

    printBorder(level, true);

    for (uint32_t y = 0; y < common::VIEWPORT_H; ++y) {

        printInventory(y, &player->inventory);

        // left border
        printVBorder(level);

        printLine(level, y, mask, pathmask);

        // right border
        printVBorder(level);

        // BUG: sometimes right border gets drawn shifter by 1, clean it up from terminal
        printChar(" ");
        printRightColumn(y);
        printChar("\n");
    }

    printBorder(level, false);
    printStats(level, &player->stats);
    printSubtitle();

    if (!common::realtimeEnabled) showMessage(msg, level, false);

    printerRefresh();
}

// what a slot holds, as the overlay shows it
static std::string heldText(const Held& h)
{
    std::string t = thingName(h.what);
    if (h.what == Thing::Vodka) t += " " + std::to_string(h.fill) + "/" + std::to_string(VODKA_SIPS);
    if (isWeapon(h.what)) t += " " + std::to_string(h.rounds);
    return t;
}

static const char* useLabel(Thing t)
{
    if (isDrink(t)) return "[E] juo  ";
    switch (t) {
        case Thing::Lenkki: case Thing::Kala: case Thing::Omppo: case Thing::Bansku: return "[E] sy\xC3\xB6  ";
        case Thing::Pistooli: return "[E] ammu ";
        case Thing::Rynkky: return "[E] t\xC3\xA4ht\xC3\xA4\xC3\xA4";
        default: return "         ";
    }
}

// row order is what useInventoryItem in pultsari.cpp reads: the selected row's thing
std::vector<OverlayItem> overlayItems(const Inventory* inventory)
{
    std::vector<std::pair<const char*, const Held*>> slots = {
        { "VASEN", &inventory->left }, { "OIKEA", &inventory->right }, { "TASKU", &inventory->pocket },
    };
    if (inventory->hasBelt) slots.push_back({ "VY\xC3\x96", &inventory->belt });
    slots.push_back({ "SELK\xC3\x84", &inventory->back });

    std::vector<OverlayItem> rows;
    for (const auto& s : slots) {
        const Held& h = *s.second;
        rows.push_back({ s.first, h.empty() ? std::string() : heldText(h), h.what, h.empty() ? 0u : 1u,
                         useLabel(h.what)[0] == '[',
                         !h.empty(), useLabel(h.what) });   // what cannot fly is put down with F
    }
    return rows;
}

ArmPictures playerArms(const PlayerNS::Player* player)
{
    ArmPictures a;
    const Inventory& inv = player->inventory;
    const Held& l = inv.akRaised ? inv.back : inv.left;
    const Held& r = inv.akRaised ? inv.back : inv.right;
    const int stage = sleeveStage(player->stats.health, player->usedCoat);
    const Coat coat = player->coat;
    a.left = armPicture(true, l, stage, coat);
    a.right = armPicture(false, r, stage, coat);
    a.leftColours = armColours(true, l, stage, coat);
    a.rightColours = armColours(false, r, stage, coat);
    a.leftText = l.empty() ? std::string() : heldText(l);
    a.rightText = r.empty() ? std::string() : heldText(r);
    return a;
}

// visible columns of a UTF-8 string, padded or cut to `width`
static std::string padColumns(const std::string& s, uint32_t width)
{
    std::string out;
    uint32_t cols = 0;
    for (size_t i = 0; i < s.size(); ) {
        size_t len = 1;
        const unsigned char c = s[i];
        if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
        if (cols == width) break;
        out += s.substr(i, len);
        ++cols;
        i += len;
    }
    for (; cols < width; ++cols) out += ' ';
    return out;
}

void Printer::printInventoryOverlay(Inventory* inventory, int selectedRow, bool throwMenu)
{
    constexpr int OVERLAY_X = 25;
    constexpr int OVERLAY_Y = 5;

    const auto items = overlayItems(inventory);
    const int NUM_ITEMS = (int)items.size();
    const char* frame = "\033[0;41;97m";

    char buf[256];

    // all rows are 38 columns wide; box-drawing chars are 3 bytes but 1 column
    snprintf(buf, sizeof(buf), "\033[%d;%dH%s\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90 \xD0\xA2\xD0\x9E\xD0\x92\xD0\x90\xD0\xA0\xD0\xAB \xC2\xB7 TAVARAT "
                 "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90"
                 "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90"
                 "\xE2\x95\x97",
            OVERLAY_Y, OVERLAY_X, frame);
    cout << buf;

    for (int i = 0; i < NUM_ITEMS; ++i) {
        bool selected = (i == selectedRow);
        const char* row = selected ? "\033[0;1;30;107m" : items[i].count ? "\033[0;41;97m" : "\033[0;41;2;37m";

        snprintf(buf, sizeof(buf), "\033[%d;%dH%s\xE2\x95\x91%s%s %s %s %s %s%s\xE2\x95\x91",
                OVERLAY_Y + 1 + i, OVERLAY_X, frame, row,
                selected ? "\xE2\x96\xB6" : " ",
                padColumns(items[i].name, 5).c_str(),
                padColumns(items[i].count ? items[i].thing : "-", 14).c_str(),
                items[i].useLabel,
                items[i].canThrow ? "[F]" : "   ",
                frame);
        cout << buf;
    }

    snprintf(buf, sizeof(buf), "\033[%d;%dH%s\xE2\x95\x9A", OVERLAY_Y + NUM_ITEMS + 1, OVERLAY_X, frame);
    cout << buf;
    for (int i = 0; i < 36; ++i) cout << "\xE2\x95\x90";
    cout << "\xE2\x95\x9D";
    snprintf(buf, sizeof(buf), "\033[%d;%dH\033[0;1;97;40m W/S selaa \xC2\xB7 E k\xC3\xA4yt\xC3\xA4 \xC2\xB7 F heit\xC3\xA4 \xC2\xB7 Q sulje ",
            OVERLAY_Y + NUM_ITEMS + 2, OVERLAY_X);
    cout << buf;

    cout << "\033[0m";
    cout.flush();
}

std::vector<std::pair<int, int>> aimCells(Level* level, uint32_t px, uint32_t py, float angle, int maxSteps)
{
    std::vector<std::pair<int, int>> cells;
    float dx = cos(angle);
    float dy = sin(angle);
    float fx = (float)px;
    float fy = (float)py;

    for (int step = 1; step < maxSteps; ++step) {
        fx += dx;
        fy += dy;
        int ix = (int)(fx + 0.5f);
        int iy = (int)(fy + 0.5f);

        if (ix < 0 || iy < 0 || ix >= (int)level->sizex || iy >= (int)level->sizey)
            break;

        Coord c { (uint32_t)ix, (uint32_t)iy };
        if (level->hitWall(c)) break;

        int vx = ix - level->cameraX;
        int vy = iy - level->cameraY;
        if (vx < 0 || vy < 0 || vx >= (int)common::VIEWPORT_W || vy >= (int)common::VIEWPORT_H)
            break;
        cells.emplace_back(vx, vy);
    }
    return cells;
}

void Printer::printAimLine(Level* level, uint32_t px, uint32_t py, float angle)
{
    for (const auto& c : aimCells(level, px, py, angle, aimLength)) {
        // map starts below the message line and top border, right of the inventory column and border
        char buf[64];
        snprintf(buf, sizeof(buf), "\033[%d;%dH\033[91m*\033[0m", c.second + 3, c.first + 15);
        cout << buf;
    }
    if (aimRisk >= 0.0f) {
        const int filled = (int)(aimRisk * 10.0f + 0.5f);
        std::string bar(filled, '#');
        bar += std::string(10 - filled, '.');
        char buf[96];
        snprintf(buf, sizeof(buf), "\033[2;20H\033[1;%dm LAUKAISUHERKKYYS [%s] %3d%% \033[0m",
                 aimRisk < 0.4f ? 92 : aimRisk < 0.75f ? 93 : 91, bar.c_str(), (int)(aimRisk * 100.0f + 0.5f));
        cout << buf;
    }
    if (gunAmmo >= 0) {
        // the gunner's shells left, on the title bar
        const int filled = (gunAmmo * 20 + (int)AAGun::AMMO - 1) / (int)AAGun::AMMO;
        std::string bar(filled, '#');
        bar += std::string(20 - filled, '.');
        char buf[128];
        snprintf(buf, sizeof(buf), "\033[2;20H\033[1;%dm IT-TYKKI  AMMUKSET %3d/%u [%s] \033[0m",
                 gunAmmo > 30 ? 92 : gunAmmo > 0 ? 93 : 91, gunAmmo, AAGun::AMMO, bar.c_str());
        cout << buf;
    }
    cout.flush();
}

void Printer::printErr(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

std::string Printer::locationName(const Level* l) const
{
    if (scene) return sceneTitle;
    if (l && player) {
        const std::string here = l->locationAt(player->coord);
        if (!here.empty()) return withTranslation(here);
    }
    return withTranslation(levelName);
}
