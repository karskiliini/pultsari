#include "printer.hpp"
#include "player.hpp"
#include "item.hpp"
#include "input.hpp"
#include <vector>
#include <iostream>
#include <string>

using std::vector;
using std::cout;
using std::endl;
using InputNS::Input;
using PlayerNS::Player;
using std::string;

void Printer::showMessage(std::string message, Level& level, bool waitKey)
{
    if (msgShown && waitKey) {
        // wait for keypress
        Input::waitKey();
    }
    msg = message;
    msgShown = true;
    print(level);
}

void Printer::removeMessage()
{
    msg = "";
    msgShown = false;
}

void Printer::raw()
{
    // Set terminal to raw mode
    system("stty raw");
}

void Printer::noraw()
{
    // Set terminal to cooked mode
    system("stty -raw");
}

const Person* findPerson(vector<const Person*> row, uint32_t x)
{
    for (const auto& p : row) {
        if (p->x == x) return p;
    }

    return nullptr;
}

const Item* findItem(vector<const Item*> row, uint32_t x)
{
    for (const auto& p : row) {
        if (p->x == x) return p;
    }

    return nullptr;
}

const string TOP_LEFT = "╔";
const string BOT_LEFT = "╚";
const string TOP_RIGHT = "╗";
const string BOT_RIGHT = "╝";
const string VERTICAL = "║";
const string HORIZONTAL = "═";


void printBorder(Level& l, bool top)
{
    if (top) {
        std::cout << TOP_LEFT;
    } else {
        std::cout << BOT_LEFT;
    }

    for (uint32_t x = 0; x < l.sizex; ++x)
        std::cout << HORIZONTAL;

    if (top) {
        std::cout << TOP_RIGHT;
    } else {
        std::cout << BOT_RIGHT;
    }
    std::cout << std::endl;
}


void Printer::print(Level& l)
{
    std::cout << "turn: " << l.turn << " " << "x: " << player->x << " " << "y: " << player->y << " " << msg << endl;

    printBorder(l, true);

    for (uint32_t y = 0; y < l.sizey; ++y) {

        // find all people that are on this line
        vector<const Person*> persons;
        for (const auto& p : l.persons) {
            if (p->y == y) {
                persons.push_back(p);
            }
        }

        vector<const Item*> items;
        for (const auto& i : l.items)
        {
            if (i->y == y) {
                items.push_back(i);
            }
        }

        // left border
        std::cout << VERTICAL;

        for (uint32_t x = 0; x < l.sizex; ++x)
        {
            bool found = false;
            string c = " ";

            for (auto b : l.buildings)
            {
                if (b->hitBuilding(x, y))
                {
                    c = b->typeToChar(x, y);
                    found = true;
                    break;
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

        // right border
        std::cout << VERTICAL << std::endl;
    }
    printBorder(l, false);

    msgShown = false;
}
