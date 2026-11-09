#include "printerterm.hpp"
#include "level.hpp"
#include "player.hpp"
#include "scoreboard.hpp"
#include "common.hpp"
#include "inventory.hpp"
#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::string;
using PlayerNS::Player;

void PrinterTerm::show_console_cursor(const bool show) {
#if defined(_WIN32)
    static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(handle, &cci);
    cci.bVisible = show; // show/hide cursor
    SetConsoleCursorInfo(handle, &cci);
#elif defined(__linux__)
    std::cout << (show ? "\033[?25h" : "\033[?25l"); // show/hide cursor
#endif // Windows/Linux
}

void PrinterTerm::cursorHome()
{
    cout << "\e[f\e[K";
}

void PrinterTerm::printMore()
{
    cout << "     <LISÄÄ>";
}

void PrinterTerm::printMessage()
{
    cout << "  " << msg;
}

void PrinterTerm::printLeftBorderSpaces()
{
    cout << "             ";
}

void PrinterTerm::printBorder(Level* l, bool top)
{
    printLeftBorderSpaces();
    if (top) {
        cout << TOP_LEFT;
    } else {
        cout << BOT_LEFT;
    }

    for (uint32_t x = 0; x < common::VIEWPORT_W; ++x)
        cout << HORIZONTAL;

    if (top) {
        cout << TOP_RIGHT;
    } else {
        cout << BOT_RIGHT;
    }
    cout << endl;
}

void PrinterTerm::printVBorder(Level* l)
{
    cout << VERTICAL;
}

void PrinterTerm::printInventory(uint32_t y, Inventory* inventory)
{
    const auto& i = *inventory;
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

void PrinterTerm::printStats(Level* l, Stats* stats)
{
    cout << "RAHAA: "    << moneyAligned(stats->money);
    cout << "  VOIMA: "    << moneyAligned(stats->health);
    cout << "  PROMILLE: " << promilleAligned(stats->promilles);
    cout << "  NÄLKÄ: " << moneyAligned(stats->hunger);
    cout << endl << endl;
}

void PrinterTerm::printChar(string c)
{
    std::cout << c;
}

void PrinterTerm::printScore(const ScoreBoard* scoreBoard)
{
    cout << "                                                                " << endl;
    cout << "                                                                " << endl;
    cout << "                                                                " << endl;
    cout << "Hakkaamasi mummelit  : " << scoreBoard->mummot << "              " << endl;
    cout << "Kaikki hakatut oliot : " << scoreBoard->kaikki << "              " << endl;
    cout << "------------------------------" <<  "              " << endl;
    cout << "Yhteispistetilanteesi: " << scoreBoard->mummot + scoreBoard->kaikki <<  "              " << endl;
    cout << "Kiitos P\xD0\x9F""LTSARI 2:n pelaamisesta!" <<  "              " << endl;
    cout << "                                                                " << endl;
    cout << "                                                                " << endl;
    cout << "                                                                " << endl;
}

void PrinterTerm::printHelp()
{
    cout << "P\xD0\x9F""LTSARI 2 - The Special Drunken Operation          " << endl;
    cout << "--los          : line of sight                        " << endl;
    cout << "--anims        : animaatiot                           " << endl;
    cout << "--path         : reitinhaku                           " << endl;
    cout << "--realtime|--rt: reaaliaikamoodi                      " << endl;
    cout << "--all | -a     : kaikki ominaisuudet                  " << endl;
    cout << "Tavoite: Paase Kremlin viinakellariin!                " << endl;
}

void PrinterTerm::emptyLine(uint32_t textLen)
{
    for (uint32_t i = 0; i < common::VIEWPORT_W - textLen; ++i) {
        cout << " ";
    }
    cout << endl;
}

void PrinterTerm::emptyTitleLine(uint32_t textLen)
{
    for (uint32_t i = 0; i < common::VIEWPORT_W / 2 - textLen; ++i) {
        cout << " ";
    }
}

void PrinterTerm::printerRefresh()
{
}