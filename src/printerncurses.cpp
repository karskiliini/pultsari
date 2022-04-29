#ifdef USE_NCURSES

#include "printerncurses.hpp"
#include "building.hpp"
#include "common.hpp"
#include "pathmask.hpp"
#include "mask.hpp"
#include "animation.hpp"
#include "level.hpp"
#include "player.hpp"
#include "scoreboard.hpp"
#include "common.hpp"
#include "inventory.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <ncurses.h>

using std::vector;
using std::cout;
using std::endl;
using std::string;
using PlayerNS::Player;

PrinterNcurses::PrinterNcurses() {
    mainwindow = initscr();
    gamewindow = newwin(common::SIZEY + 2, common::SIZEX + 2, 5, 18);
    inventorywindow = newwin(11, 16, 5, 1);
    msgwindow = newwin(3, common::SIZEX + 2, 2, 18);
    statswindow = newwin(3, common::SIZEX + 2, 35, 18);
}

PrinterNcurses::~PrinterNcurses() {
    endwin();
}

void PrinterNcurses::clear()
{
    wclear((WINDOW*)mainwindow);
    wclear((WINDOW*)statswindow);
    wclear((WINDOW*)gamewindow);
    ::clear();
}

void PrinterNcurses::show_console_cursor(const bool show) {
    curs_set(show ? 1 : 0);
}

void PrinterNcurses::cursorHome()
{
}

void PrinterNcurses::printMore()
{
    mvwprintw((WINDOW*)msgwindow, 1, 70, "<LISAA>");
    wrefresh((WINDOW*)msgwindow);
}

void PrinterNcurses::setMessage(const std::string& message)
{
    msg = message;
}

void PrinterNcurses::printMessage()
{
    wclear((WINDOW*)msgwindow);
    mvwprintw((WINDOW*)msgwindow, 1, 1, msg.c_str());
    wrefresh((WINDOW*)msgwindow);
}

void PrinterNcurses::printLeftBorderSpaces() { }
void PrinterNcurses::printBorder(Level* l, bool top) { }
void PrinterNcurses::printVBorder(Level* l) { }

void PrinterNcurses::printInventory(uint32_t /* y */, Inventory* inventory)
{
    constexpr uint32_t x_offset = 1;
    const auto& i = *inventory;

    mvwprintw((WINDOW*)inventorywindow, 1, x_offset, (std::string("1. KALJAA    ") + inventoryAligned(i.kalja)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 2, x_offset, (std::string("2. LONKKA    ") + inventoryAligned(i.lonkka)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 3, x_offset, (std::string("3. LENKKI    ") + inventoryAligned(i.lenkki)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 4, x_offset, (std::string("4. KETJUT    ") + inventoryAligned(i.ketjut)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 5, x_offset, (std::string("5. VEITSET   ") + inventoryAligned(i.veitset)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 6, x_offset, (std::string("6. KIVET     ") + inventoryAligned(i.kivet)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 7, x_offset, (std::string("7. PAMPUT    ") + inventoryAligned(i.pamput)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 8, x_offset, (std::string("8. BOOTSIT   ") + inventoryAligned(i.bootsit)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 9, x_offset, (std::string("9. KALAT     ") + inventoryAligned(i.kalat)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 10, x_offset, (std::string("A. OMPPO     ") + inventoryAligned(i.omppo)).c_str());
    mvwprintw((WINDOW*)inventorywindow, 11, x_offset, (std::string("B. BANSKU    ") + inventoryAligned(i.bansku)).c_str());
}

void PrinterNcurses::printStats(Level* l, Stats* stats)
{
    mvwprintw((WINDOW*)statswindow, 1, 0, ("RAHAA: "      + moneyAligned(stats->money)).c_str());
    wprintw((WINDOW*)statswindow, ("  VOIMA: "    + moneyAligned(stats->health)).c_str());
    wprintw((WINDOW*)statswindow,("  PROMILLE: " + promilleAligned(stats->promilles)).c_str());
    wprintw((WINDOW*)statswindow,("  KÄPPÄILY: " + std::to_string(stats->turn) + " ").c_str());
    wprintw((WINDOW*)statswindow,("  LEVEL: "    + std::to_string(l->stage) + " ").c_str());
}

void PrinterNcurses::printChar(string c)
{
}

void PrinterNcurses::printScore(const ScoreBoard* scoreBoard)
{
    printw("                                                                \n");
    printw("                                                                \n");
    printw("                                                                \n");
    printw((std::string("Hakkaamasi mummelit  : ") + std::to_string(scoreBoard->mummot) + "              " + "\n").c_str());
    printw((std::string("Kaikki hakatut oliot : ") + std::to_string(scoreBoard->kaikki) + "              " + "\n").c_str());
    printw((std::string("------------------------------") + "              " + "\n").c_str());
    printw(("Yhteispistetilanteesi: " + std::to_string(scoreBoard->mummot + scoreBoard->kaikki) +  "              ").c_str());
    printw("Kiitos Pultsarin pelaamisesta!              \n");
    printw("                                                                ");
    printw("                                                                ");
    printw("                                                                ");
}

void PrinterNcurses::printHelp()
{
    clear();
    wclear((WINDOW*)gamewindow);

    wprintw((WINDOW*)gamewindow, "Pultsari help:                                        \n");
    wprintw((WINDOW*)gamewindow, "--los      : to enable line of sight mode             \n");
    wprintw((WINDOW*)gamewindow, "--anims    : to enable animations support             \n");
    wprintw((WINDOW*)gamewindow, "--path     : to enable advanced path finding          \n");
    wprintw((WINDOW*)gamewindow, "--all | -a : enable all features                      \n");

    printerRefresh();
}

void PrinterNcurses::emptyLine(uint32_t textLen)
{
    for (uint32_t i = 0; i < common::SIZEX - textLen; ++i) {
        printw(" ");
    }
    printw("\n");
}

void PrinterNcurses::emptyTitleLine(uint32_t textLen)
{
    for (uint32_t i = 0; i < common::SIZEX / 2 - textLen; ++i) {
        printw(" ");
    }
}

void PrinterNcurses::printerRefresh()
{
    // box((WINDOW*)mainwindow, 0, 0);
    box((WINDOW*)gamewindow, 0, 0);
    box((WINDOW*)inventorywindow, 0, 0);
    box((WINDOW*)msgwindow, 0, 0);
    box((WINDOW*)statswindow, 0, 0);

    wrefresh((WINDOW*)mainwindow);
    wrefresh((WINDOW*)gamewindow);
    wrefresh((WINDOW*)inventorywindow);
    wrefresh((WINDOW*)msgwindow);
    wrefresh((WINDOW*)statswindow);
}

void PrinterNcurses::printLine(Level* l, uint32_t y, const VisionNS::Mask* mask, PathNS::PathMask* pathmask)
{
    wmove((WINDOW*)gamewindow, y+1, 1);

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

        if (c == "█")
        {
            waddch((WINDOW*)gamewindow, ACS_BLOCK);
        }
        else if (c == "░")
        {
            waddch((WINDOW*)gamewindow, ACS_UARROW);
        }
        else if (c == "°")
        {
            waddch((WINDOW*)gamewindow, ACS_BULLET);
        }
        else if (c == "%")
        {
            waddch((WINDOW*)gamewindow, ACS_BBSS);
        }
        else if (c == "¨")
        {
            waddch((WINDOW*)gamewindow, ACS_PI);
        }
        else if (c == "¥")
        {
            waddch((WINDOW*)gamewindow, ACS_DIAMOND);
        } else {
            wprintw((WINDOW*)gamewindow, c.c_str());
        }
    }
}

void PrinterNcurses::print(Level* level)
{
    wclear((WINDOW*)msgwindow);
    wclear((WINDOW*)gamewindow);


    printInventory(0, &player->inventory);

    for (uint32_t y = 0; y < level->sizey; ++y) {
        printLine(level, y, mask, pathmask);
    }

    printStats(level, &player->stats);
    showMessage(msg, level, false);
    printerRefresh();
}

#endif // USE_NCURSES