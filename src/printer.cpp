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

// A person's colour is their clothing or skin (xterm 256-colour palette).
const char* personColor(const Person* p)
{
    switch (p->type) {
        case pelaaja:
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
        default:           return "1;97";
    }
}

const char* buildingColor(const Building* b, const string& glyph)
{
    if (glyph == "\xE2\x96\x88") return b->type == EKremlin ? "31" : "90";
    if (glyph == "\xE2\x96\x91") return "33";
    if (glyph == "\xE2\x96\x92") return "2;36";
    if (glyph == "~") return "1;91";
    if (glyph == ";") return "2;37";
    return "1;97";
}

}

void Printer::printLine(Level* l, uint32_t y, const VisionNS::Mask* mask, PathNS::PathMask* pathmask)
{
    uint32_t worldY = y + (uint32_t)l->cameraY;
    string line;
    string current;
    auto emit = [&](const string& glyph, const char* color) {
        if (current != color) {
            line += "\033[0m";
            if (*color) line += string("\033[") + color + "m";
            current = color;
        }
        line += glyph;
    };

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

    for (uint32_t vx = 0; vx < common::VIEWPORT_W; ++vx) {
        uint32_t x = vx + (uint32_t)l->cameraX;
        Coord coord { x, worldY };
        bool found = false;
        string c = " ";
        const char* color = "";

        for (auto it = rowAnims.rbegin(); it != rowAnims.rend(); ++it) {
            if (x >= it->x && x < it->x + it->chars.size()) {
                c = it->chars[x - it->x];
                color = it->color;
                found = true;
                break;
            }
        }

        if (!found && !mask->visible(coord)) {
            c = ".";
            color = "2;90";
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
                string g = b->typeToChar(coord);
                if (g != " " || b->type != EKremlin) {
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
                if (v->hitVehicle(coord)) {
                    c = v->typeToChar();
                    color = "1;93;40";
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

        emit(c, color);
    }
    line += "\033[0m";
    printChar(line);
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

void Printer::printInventoryOverlay(Inventory* inventory, int selectedRow)
{
    constexpr int OVERLAY_X = 25;
    constexpr int OVERLAY_Y = 5;

    struct InvItem {
        const char* name;
        uint32_t count;
        bool canUse;
        bool canThrow;
        const char* useLabel;
    };

    // row order must match useInventoryItem / invIndexToThrowNum in pultsari.cpp
    InvItem items[] = {
        {"KALJA",     inventory->kalja,    true,  true,  "[Q] juo  "},
        {"JAGUAR",    inventory->lonkka,   true,  true,  "[Q] juo  "},
        {"VODKA",     inventory->vodka,    true,  true,  "[Q] juo  "},
        {"MAKKARA",   inventory->lenkki,   true,  true,  "[Q] syö  "},
        {"VOBLA",     inventory->kalat,    true,  true,  "[Q] syö  "},
        {"OMPPO",     inventory->omppo,    true,  true,  "[Q] syö  "},
        {"BANAANI",   inventory->bansku,   true,  true,  "[Q] syö  "},
        {"KETJUT",    inventory->ketjut,   false, true,  "         "},
        {"VEITSI",    inventory->veitset,  false, true,  "         "},
        {"KIVI",      inventory->kivet,    false, true,  "         "},
        {"PAMPPU",    inventory->pamput,   false, true,  "         "},
        {"VERKKARIT", inventory->bootsit,  false, true,  "         "},
        {"KRANAATTI", inventory->granata,  false, true,  "         "},
        {"PISTOOLI",  inventory->pistooli, true,  false, "[Q] ammu "},
        {"AMMUKSET",  inventory->ammukset, false, false, "         "},
    };
    constexpr int NUM_ITEMS = sizeof(items) / sizeof(items[0]);
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
                items[i].canThrow ? "[E]" : "   ",
                frame);
        cout << buf;
    }

    snprintf(buf, sizeof(buf), "\033[%d;%dH%s\xE2\x95\x9A", OVERLAY_Y + NUM_ITEMS + 1, OVERLAY_X, frame);
    cout << buf;
    for (int i = 0; i < 30; ++i) cout << "\xE2\x95\x90";
    cout << "\xE2\x95\x9D";
    snprintf(buf, sizeof(buf), "\033[%d;%dH\033[0;1;97;40m W/S selaa \xC2\xB7 Q k\xC3\xA4yt\xC3\xA4 \xC2\xB7 E heit\xC3\xA4 \xC2\xB7 Tab sulje ",
            OVERLAY_Y + NUM_ITEMS + 2, OVERLAY_X);
    cout << buf;

    cout << "\033[0m";
    cout.flush();
}

void Printer::printAimLine(Level* level, uint32_t px, uint32_t py, float angle)
{
    float dx = cos(angle);
    float dy = sin(angle);
    float fx = (float)px;
    float fy = (float)py;

    for (int step = 1; step < 20; ++step) {
        fx += dx;
        fy += dy;
        int ix = (int)(fx + 0.5f);
        int iy = (int)(fy + 0.5f);

        if (ix < 0 || iy < 0 || ix >= (int)level->sizex || iy >= (int)level->sizey)
            break;

        Coord c { (uint32_t)ix, (uint32_t)iy };
        if (level->hitBuilding(c)) break;

        int vx = ix - level->cameraX;
        int vy = iy - level->cameraY;
        if (vx < 0 || vy < 0 || vx >= (int)common::VIEWPORT_W || vy >= (int)common::VIEWPORT_H)
            break;

        // map starts below the message line and top border, right of the inventory column and border
        int termX = vx + 15;
        int termY = vy + 3;

        char buf[64];
        snprintf(buf, sizeof(buf), "\033[%d;%dH\033[91m*\033[0m", termY, termX);
        cout << buf;
    }
    cout.flush();
}

void Printer::printErr(const std::string& msg)
{
    std::cerr << msg << std::endl;
}
