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
}

void Printer::showMessage(std::string message, Level* level, bool waitKey)
{
    if (message == "") {
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
        sprintf(str, "%d", value);
        return string(str) + "  ";
    } else if (value < 100) {
        char str[10];
        sprintf(str, "%d", value);
        return string(str) + " ";
    } else {
        return ".. ";
    }
}

string Printer::moneyAligned(uint32_t value) const
{
    if (value < 10) {
        char str[10];
        sprintf(str, "%d", value);
        return string(str) + "    ";
    } else if (value < 100) {
        char str[10];
        sprintf(str, "%d", value);
        return string(str) + "   ";
    } else if (value < 1000) {
        char str[10];
        sprintf(str, "%d", value);
        return string(str) + "  ";
    } else if (value < 10000) {
        char str[10];
        sprintf(str, "%d", value);
        return string(str) + " ";
    } else if (value < 100000) {
        char str[10];
        sprintf(str, "%d", value);
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
        sprintf(str, "%d", full);
        ret = " " + string(str) + ".";
    } else if (full < 100) {
        char str[10];
        sprintf(str, "%d", full);
        ret = "" + string(str) + ".";
    } else return "KÄNNI";

    uint32_t parts = value % 10;
    {
        char str[10];
        sprintf(str, "%d", parts);
        ret += string(str);
    }
    return ret;
}

void Printer::printLine(Level* l, uint32_t y, const VisionNS::Mask* mask, PathNS::PathMask* pathmask)
{
    uint32_t worldY = y + (uint32_t)l->cameraY;

    Animation* anim = nullptr;
    if (common::animsEnabled) {
        if (l->animation && l->animation->coord.y == worldY) {
            anim = l->animation;
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

        if (common::animsEnabled) {
            if (anim)
            {
                uint32_t len = anim->text.length();
                if ((anim->coord.x <= x) && (anim->coord.x + len > x))
                {
                    uint32_t index = x - anim->coord.x;
                    if (anim->isBubble) {
                        // speech bubble: dim text for 75% opacity effect
                        c = std::string("\033[2;37m") + anim->text[index] + "\033[0m";
                    } else {
                        c = anim->text[index];
                    }
                    found = true;
                }
            }
        }

#if 0
        // debug
        if (pathmask)
        {
            auto v = pathmask->at(coord);
            if ((v > 1) && (coord != l->getPlayer()->coord)) {
                c = std::to_string(v)[0];
                found = true;
            }
        } else {
            return;
        }
#endif
        if (!found) {
            if (!mask->visible(coord)) {
                c = ".";
                found = true;
            }
        }

        if (thrownItem && thrownItem->coord.x == x)
        {
            c = thrownItem->typeToChar();
            found = true;
        }

        if (!found) {
            for (auto b : l->buildings) {
                if (b->hitBuilding(coord)) {
                    c = b->typeToChar(coord);
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            const Person* p = findPerson(persons, x);
            if (p) {
                c = p->typeToChar();
                found = true;
            }
        }

        if (!found) {
            const Item* i = findItem(items, x);
            if (i) {
                c = i->typeToChar();
                found = true;
            }
        }

        if (!found) {
            for (const auto& v : l->vehicles) {
                if (v->hitVehicle(coord)) {
                    c = v->typeToChar();
                    found = true;
                    break;
                }
            }
        }

        if (!found && l->events) {
            if (l->events->hasFire(coord)) {
                c = "~";
                found = true;
            }
        }

        if (!found) {
            for (auto b : l->buildings) {
                if (b->type == EHypermarket) {
                    Hypermarket* hm = dynamic_cast<Hypermarket*>(b);
                    if (hm && hm->onFire) {
                        for (const auto& s : hm->smokeSpots) {
                            if (s == coord) {
                                c = std::string("\033[2;37m") + ";" + "\033[0m";
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            for (const auto& f : hm->fireSpots) {
                                if (f == coord) {
                                    c = std::string("\033[91m") + "~" + "\033[0m";
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (found) break;
                }
            }
        }

        printChar(c);
    }
}

void Printer::print(Level* level)
{
    cursorHome();
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

    showMessage(msg, level, false);

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

    InvItem items[] = {
        {"KALJA",   inventory->kalja,   true,  true,  "[Q]juo"},
        {"LONKKA",  inventory->lonkka,  true,  true,  "[Q]juo"},
        {"LENKKI",  inventory->lenkki,  true,  true,  "[Q]syö"},
        {"KALA",    inventory->kalat,   true,  true,  "[Q]syö"},
        {"OMPPO",   inventory->omppo,   true,  true,  "[Q]syö"},
        {"BANSKU",  inventory->bansku,  true,  true,  "[Q]syö"},
        {"KETJUT",  inventory->ketjut,  false, true,  "      "},
        {"VEITSI",  inventory->veitset, false, true,  "      "},
        {"KIVI",    inventory->kivet,   false, true,  "      "},
        {"PAMPPU",  inventory->pamput,  false, true,  "      "},
        {"BOOTSIT", inventory->bootsit, false, true,  "      "},
        {"PISTOOLI",inventory->pistooli,true,  false, "[Q]ammu"},
        {"AMMUKSET",inventory->ammukset,false, false, "       "},
    };
    constexpr int NUM_ITEMS = 13;

    char buf[256];

    // top border
    sprintf(buf, "\033[%d;%dH\033[44;37m+------- TAVARAT --------+", OVERLAY_Y, OVERLAY_X);
    cout << buf;

    for (int i = 0; i < NUM_ITEMS; ++i) {
        int y = OVERLAY_Y + 1 + i;
        bool selected = (i == selectedRow);
        const char* arrow = selected ? ">" : " ";
        const char* color = selected ? "\033[43;30m" : "\033[44;37m";

        sprintf(buf, "\033[%d;%dH%s%s %-8s x%-3d %s %s\033[44;37m",
                y, OVERLAY_X, color, arrow,
                items[i].name, items[i].count,
                items[i].useLabel,
                items[i].canThrow ? "[E]heitä" : "");
        cout << buf;
    }

    // bottom border + help
    sprintf(buf, "\033[%d;%dH+------------------------+", OVERLAY_Y + NUM_ITEMS + 1, OVERLAY_X);
    cout << buf;
    sprintf(buf, "\033[%d;%dH W/S=selaa Q=käytä E=heitä", OVERLAY_Y + NUM_ITEMS + 2, OVERLAY_X);
    cout << buf;
    sprintf(buf, "\033[%d;%dH Tab=sulje               ", OVERLAY_Y + NUM_ITEMS + 3, OVERLAY_X);
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

        // +3 for border offset in terminal (1 newline + 1 border + 1 inventory column)
        int termX = ix + 15;
        int termY = iy + 3;

        char buf[64];
        sprintf(buf, "\033[%d;%dH\033[91m*\033[0m", termY, termX);
        cout << buf;
    }
    cout.flush();
}

void Printer::printErr(const std::string& msg)
{
    std::cerr << msg << std::endl;
}
