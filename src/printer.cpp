#include "printer.hpp"
#include "player.hpp"
#include "item.hpp"
#include "input.hpp"
#include "building.hpp"
#include "animation.hpp"
#include "mask.hpp"
#include "pathmask.hpp"
#include "level.hpp"
#include "vehicle.hpp"
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
        if (p->coord.x == x) return p;
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
const char* droneCell(const Level* l, uint32_t x, uint32_t y, bool& shadow)
{
    shadow = false;
    for (const auto& d : l->drones) {
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
        case sotilas:
        case ilmatorjunta: return "38;5;64";            // camouflage green
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
        default:           return "1;97";
    }
}

const char* buildingColor(const Building* b, const string& glyph)
{
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

vector<std::pair<string, string>> Printer::rowCells(Level* l, uint32_t y)
{
    uint32_t worldY = y + (uint32_t)l->cameraY;
    vector<std::pair<string, string>> cells;
    cells.reserve(viewW);
    auto emit = [&](const string& glyph, const string& color) { cells.emplace_back(glyph, color); };

    struct RowAnim { uint32_t x; vector<string> chars; const char* color; };
    vector<RowAnim> rowAnims;
    if (common::animsEnabled) {
        for (const auto a : l->animations) {
            if (a->coord.y != worldY) continue;
            if (!mask->visible(a->source)) continue;
            // speech bubbles fade from white to dark grey as they expire
            const char* color = "1;91";
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

    Item* thrownItem = nullptr;
    if (l->thrownItem && l->thrownItem->coord.y == worldY) {
        thrownItem = l->thrownItem;
    }

    vector<const Person*> persons;
    for (const auto& p : l->persons) {
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

    for (uint32_t vx = 0; vx < viewW; ++vx) {
        uint32_t x = vx + (uint32_t)l->cameraX;
        Coord coord { x, worldY };
        bool found = false;
        string c = " ";
        const char* color = "";

        // drones are up in the sky, above everything, fog included
        bool droneShadow = false;
        if (const char* g = droneCell(l, x, worldY, droneShadow)) {
            c = g;
            color = "1;38;5;250";
            found = true;
        }

        for (auto it = rowAnims.rbegin(); it != rowAnims.rend(); ++it) {
            if (x >= it->x && x < it->x + it->chars.size()) {
                c = it->chars[x - it->x];
                color = it->color;
                found = true;
                break;
            }
        }

        if (!found && !mask->visible(coord)) {
            // out of sight the streets still show faintly, so you can tell where you are
            const Road* h = nullptr;
            const Road* v = nullptr;
            l->roadsAt(coord, h, v);
            c = (h || v) ? " " : ".";
            color = (h || v) ? "48;5;234" : "2;90";
            found = true;
        }

        if (thrownItem && thrownItem->coord.x == x) {
            c = thrownItem->typeToChar();
            color = "1;97";
            found = true;
        }

        // walls, doors, windows and name plates; an empty interior lets people and items show
        if (!found) {
            for (auto b : l->buildings) {
                if (!b->hitBuilding(coord)) continue;
                if (b->isBreach(coord) && l->getVehicleAt(coord)) break; // the car in the hole shows
                string g = b->typeToChar(coord);
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
                    c = v->glyphAt(part);
                    color = v->colorAt(part);
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
            for (auto b : l->buildings) {
                if (b->type != EHypermarket) continue;
                Hypermarket* hm = dynamic_cast<Hypermarket*>(b);
                if (!hm || !hm->onFire) continue;
                for (const auto& s : hm->smokeSpots) {
                    if (s == coord) { c = ";"; color = "2;37"; found = true; break; }
                }
                for (const auto& f : hm->fireSpots) {
                    if (!found && f == coord) { c = "~"; color = "1;91"; found = true; break; }
                }
                if (found) break;
            }
        }

        const bool groundVisible = mask->visible(coord);
        string col = color;
        if (!found) {
            roadCell(l, coord, c, color);
            col = color;
            if (groundVisible && col.empty()) {
                for (auto b : l->buildings) {
                    if (b->walkable && b->hitBuilding(coord) && !b->hitWall(coord)) { col = "48;5;52"; break; }
                }
            }
            // drifting gas: grey exhaust, yellowish toxic smoke
            if (groundVisible) {
                const int gasKind = l->gasAt(coord);
                if (gasKind && c == " ") {
                    c = gasKind == 2 ? "\xE2\x96\x92" : "\xE2\x96\x91";
                    col = gasKind == 2 ? "38;5;143" : "38;5;245";
                    if (string(color).find("48;") != string::npos) col += string(";") + color;
                }
            }
        } else if (col.find("48;") == string::npos && col != "2;90") {
            // people, items and cars on a street keep the asphalt underneath
            const Road* h = nullptr;
            const Road* v = nullptr;
            l->roadsAt(coord, h, v);
            if (h || v) col += col.empty() ? "48;5;236" : ";48;5;236";
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
        printChar(" \n");
    }

    printBorder(level, false);
    printStats(level, &player->stats);

    if (!common::realtimeEnabled) showMessage(msg, level, false);

    printerRefresh();
}

// row order must match useInventoryItem / invIndexToThrowNum in pultsari.cpp
std::vector<OverlayItem> overlayItems(const Inventory* inventory)
{
    return {
        {"PIVO",     inventory->kalja,    true,  true,  "[E] juo  "},
        {"JAGUAR",    inventory->lonkka,   true,  true,  "[E] juo  "},
        {"VODKA",     inventory->vodka,    true,  true,  "[E] juo  "},
        {"MAKKARA",   inventory->lenkki,   true,  true,  "[E] syö  "},
        {"VOBLA",     inventory->kalat,    true,  true,  "[E] syö  "},
        {"ANTONOVKA",     inventory->omppo,    true,  true,  "[E] syö  "},
        {"BANAANI",   inventory->bansku,   true,  true,  "[E] syö  "},
        {"KETJUT",    inventory->ketjut,   false, true,  "         "},
        {"VEITSI",    inventory->veitset,  false, true,  "         "},
        {"KIVI",      inventory->kivet,    false, true,  "         "},
        {"PAMPPU",    inventory->pamput,   false, true,  "         "},
        {"VERKKARIT", inventory->bootsit,  false, true,  "         "},
        {"KRANAATTI", inventory->granata,  false, true,  "         "},
        {"PISTOOLI",  inventory->pistooli, true,  false, "[E] ammu "},
        {"AMMUKSET",  inventory->ammukset, false, false, "         "},
        {"RYNKKY",    inventory->rynkky,   true,  false, "[E] t\xC3\xA4ht\xC3\xA4\xC3\xA4"},
    };
}

void Printer::printInventoryOverlay(Inventory* inventory, int selectedRow, bool throwMenu)
{
    constexpr int OVERLAY_X = 25;
    constexpr int OVERLAY_Y = 5;

    const auto items = overlayItems(inventory);
    const int NUM_ITEMS = (int)items.size();
    const char* frame = "\033[0;41;97m";

    char buf[256];

    // all rows are 32 columns wide; box-drawing chars are 3 bytes but 1 column
    snprintf(buf, sizeof(buf), "\033[%d;%dH%s\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90 \xD0\xA2\xD0\x9E\xD0\x92\xD0\x90\xD0\xA0\xD0\xAB \xC2\xB7 TAVARAT "
                 "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90"
                 "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x97",
            OVERLAY_Y, OVERLAY_X, frame);
    cout << buf;

    for (int i = 0; i < NUM_ITEMS; ++i) {
        bool selected = (i == selectedRow);
        const char* row = selected ? "\033[0;1;30;107m" : items[i].count ? "\033[0;41;97m" : "\033[0;41;2;37m";

        snprintf(buf, sizeof(buf), "\033[%d;%dH%s\xE2\x95\x91%s%s %-9s x%-3u %s %s%s\xE2\x95\x91",
                OVERLAY_Y + 1 + i, OVERLAY_X, frame, row,
                selected ? "\xE2\x96\xB6" : " ",
                items[i].name, items[i].count,
                items[i].useLabel,
                items[i].canThrow ? "[F]" : "   ",
                frame);
        cout << buf;
    }

    snprintf(buf, sizeof(buf), "\033[%d;%dH%s\xE2\x95\x9A", OVERLAY_Y + NUM_ITEMS + 1, OVERLAY_X, frame);
    cout << buf;
    for (int i = 0; i < 30; ++i) cout << "\xE2\x95\x90";
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
    cout.flush();
}

void Printer::printErr(const std::string& msg)
{
    std::cerr << msg << std::endl;
}
