#include "printer.hpp"
#include "player.hpp"
#include "item.hpp"
#include "input.hpp"
#include "building.hpp"
#include "animation.hpp"
#include "mask.hpp"
#include "pathmask.hpp"
#include "level.hpp"
#include <vector>
#include <iostream>
#include <string>

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
    if (message == "") return;

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
            Input::waitKey();
        }

        msg = tokenizeStr(message);
        if (!msgShown) msgShown = true;

        cursorHome();
        printMessage();

        if (tokenize && !waited)
        {
            printMore();
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
    Animation* anim = nullptr;
    if (common::animsEnabled) {
        if (l->animation && l->animation->coord.y == y) {
            anim = l->animation;
        }
    }

    Item* thrownItem = nullptr;
    if (l->thrownItem && l->thrownItem->coord.y == y) {
        thrownItem = l->thrownItem;
    }

    // find all people that are on this line
    vector<const Person*> persons;
    for (const auto& p : l->persons) {
        if (p->coord.y == y) {
            persons.push_back(p);
        }
    }

    vector<const Item*> items;
    for (const auto& i : l->items) {
        if (i->coord.y == y) {
            items.push_back(i);
        }
    }

    for (uint32_t x = 0; x < l->sizex; ++x) {
        Coord coord { x, y };
        bool found = false;
        string c = " ";

        if (common::animsEnabled) {
            if (anim)
            {
                uint32_t len = anim->text.length();
                if ((anim->coord.x <= x) && (anim->coord.x + len > x))
                {
                    uint32_t index = x - anim->coord.x;
                    c = anim->text[index];
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

        printChar(c);
    }
}

void Printer::print(Level* level)
{
    cursorHome();
    printChar("\n");

    printBorder(level, true);

    for (uint32_t y = 0; y < level->sizey; ++y) {

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
    cursorHome();
}

void Printer::printErr(const std::string& msg)
{
    std::cerr << msg << std::endl;
}
