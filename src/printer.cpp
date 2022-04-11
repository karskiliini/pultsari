#include "printer.hpp"
#include "player.hpp"
#include "item.hpp"
#include "input.hpp"
#include "building.hpp"
#include "animation.hpp"
#include <vector>
#include <iostream>
#include <string>

using std::vector;
using std::cout;
using std::endl;
using InputNS::Input;
using PlayerNS::Player;
using std::string;

static void cursorHome()
{
    cout << "\e[f\e[K";
}

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

void Printer::setMessage(std::string message)
{
    msg = message;
}

void Printer::showMessage(std::string message, Level& level, bool waitKey)
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
            cout << "     <LISÄÄ>";
            Input::waitKey();
        }

        msg = tokenizeStr(message);
        if (!msgShown) msgShown = true;

        cursorHome();
        cout << "  " << msg;

        if (tokenize && !waited)
        {
            cout << "     <LISÄÄ>";
            Input::waitKey();
            waited = true;
        }
    }
}

void Printer::removeMessage()
{
    msg = "";
    if (msgShown) msgShown = false;
}

const Person* findPerson(vector<const Person*> row, uint32_t x)
{
    for (const auto& p : row) {
        if (p->coord.x == x) return p;
    }

    return nullptr;
}

const Item* findItem(vector<const Item*> row, uint32_t x)
{
    for (const auto& p : row) {
        if (p->coord.x == x) return p;
    }

    return nullptr;
}

const string TOP_LEFT = "╔";
const string BOT_LEFT = "╚";
const string TOP_RIGHT = "╗";
const string BOT_RIGHT = "╝";
const string VERTICAL = "║";
const string HORIZONTAL = "═";

void printLeftBorderSpaces()
{
    cout << "             ";
}

void printBorder(Level& l, bool top)
{
    printLeftBorderSpaces();
    if (top) {
        cout << TOP_LEFT;
    } else {
        cout << BOT_LEFT;
    }

    for (uint32_t x = 0; x < l.sizex; ++x)
        cout << HORIZONTAL;

    if (top) {
        cout << TOP_RIGHT;
    } else {
        cout << BOT_RIGHT;
    }
    cout << endl;
}

string inventoryAligned(uint32_t value)
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

void printInventory(uint32_t y, Player* player)
{
    auto& i = player->inventory;
    constexpr uint32_t ROW = 1;

    switch(y) {
        case ROW +  0: cout << "KALJAA  = " << inventoryAligned(i.kalja); break;
        case ROW +  1: cout << "LONKKA  = " << inventoryAligned(i.lonkka); break;
        case ROW +  2: cout << "LENKKI  = " << inventoryAligned(i.lenkki); break;
        case ROW +  3: cout << "KETJUT  = " << inventoryAligned(i.ketjut); break;
        case ROW +  4: cout << "VEITSET = " << inventoryAligned(i.veitset); break;
        case ROW +  5: cout << "KIVET   = " << inventoryAligned(i.kivet); break;
        case ROW +  6: cout << "PAMPUT  = " << inventoryAligned(i.pamput); break;
        case ROW +  7: cout << "BOOTSIT = " << inventoryAligned(i.bootsit); break;
        case ROW +  8: cout << "KALAT   = " << inventoryAligned(i.kalat); break;
        case ROW +  9: cout << "OMPPO   = " << inventoryAligned(i.omppo); break;
        case ROW + 10: cout << "BANSKU  = " << inventoryAligned(i.bansku); break;
        default: printLeftBorderSpaces(); break;
    }
}

string moneyAligned(uint32_t value)
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

string promilleAligned(uint32_t value)
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

void printStats(Level& l, Player* player)
{
    cout << "RAHAA: "    << moneyAligned(player->money);
    cout << "  VOIMA: "    << moneyAligned(player->health);
    cout << "  PROMILLE: " << promilleAligned(player->promilles);
    cout << "  KÄPPÄILY: " << player->turn << " ";
    cout << "  LEVEL: "  << l.stage << " ";
    cout << endl << endl;
}

static void printLine(Level& l, uint32_t y)
{
    Animation* anim = nullptr;
    if (l.animation && l.animation->coord.y == y) {
        anim = l.animation;
    }

    Item* thrownItem = nullptr;
    if (l.thrownItem && l.thrownItem->coord.y == y) {
        thrownItem = l.thrownItem;
    }

    // find all people that are on this line
    vector<const Person*> persons;
    for (const auto& p : l.persons) {
        if (p->coord.y == y) {
            persons.push_back(p);
        }
    }

    vector<const Item*> items;
    for (const auto& i : l.items) {
        if (i->coord.y == y) {
            items.push_back(i);
        }
    }

    for (uint32_t x = 0; x < l.sizex; ++x) {
        Coord coord { x, y };
        bool found = false;
        string c = " ";

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

        if (thrownItem && thrownItem->coord.x == x)
        {
            c = thrownItem->typeToChar();
            found = true;
        }

        if (!found) {
            for (auto b : l.buildings) {
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
        cout << c;
    }
}

void Printer::print(Level& level)
{
    cursorHome();
    cout << endl;

    printBorder(level, true);

    for (uint32_t y = 0; y < level.sizey; ++y) {

        printInventory(y, player);

        // left border
        cout << VERTICAL;

        printLine(level, y);

        // right border
        cout << VERTICAL << " " << endl;
    }
    printBorder(level, false);
    printStats(level, player);

    showMessage(msg, level, false);
}
