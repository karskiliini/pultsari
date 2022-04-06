#include "printer.hpp"
#include "player.hpp"
#include <vector>
#include <iostream>

using std::vector;
using std::cout;
using std::endl;

void Printer::showMessage(std::string message)
{
    if (msgShown) {
        // wait for keypress
    }
    msg = message;
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
    if (row.size() == 0) return nullptr;
    for (const auto& p : row) {
        if (p->x == x) return p;
    }

    return nullptr;
}

void printBorder(Level l, bool top)
{
    if (top) {
        std::cout << "/";
    } else {
        std::cout << "\\";
    }

    for (uint32_t x = 0; x < l.sizex; ++x)
        std::cout << "-";

    if (top) {
        std::cout << "\\";
    } else {
        std::cout << "/";
    }
    std::cout << std::endl;
}

void Printer::print(Level l)
{
    // raw();

    std::cout << "turn: " << l.turn << " " << msg << endl;

    printBorder(l, true);

    for (uint32_t y = 0; y < l.sizey; ++y) {


        vector<const Person*> persons;
        for (const auto& p : l.persons) {
            if (p->y == y) {
                persons.push_back(p);
            }
        }

        std::cout << "|";

        for (uint32_t x = 0; x < l.sizex; ++x)
        {
            const Person* p = findPerson(persons, x);
            if (p) {
                char c = p->typeToChar();
                std::cout << c;
            } else {
                std::cout << " ";
            }
        }
        std::cout << "|" << std::endl;
    }
    printBorder(l, false);


    // noraw();
}
