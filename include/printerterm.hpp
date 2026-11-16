#ifndef PRINTERTERM_HPP
#define PRINTERTERM_HPP

#include "printer.hpp"

class PrinterTerm : public Printer {
public:
    const std::string TOP_LEFT = "╔";
    const std::string BOT_LEFT = "╚";
    const std::string TOP_RIGHT = "╗";
    const std::string BOT_RIGHT = "╝";
    const std::string VERTICAL = "║";
    const std::string HORIZONTAL = "═";

    virtual void show_console_cursor(const bool show);
    virtual void cursorHome();
    virtual void printMore();
    virtual void printMessage();
    virtual void printLeftBorderSpaces();
    virtual void printBorder(Level* l, bool top);
    virtual void printVBorder(Level* l);
    virtual void printInventory(uint32_t y, Inventory* inventory);
    virtual void printRightColumn(uint32_t y);
    virtual void printStats(Level* l, Stats* stats);
    virtual void printChar(std::string c);
    virtual void printScore(const ScoreBoard* scoreBoard);
    virtual void printHelp();
    virtual void emptyLine(uint32_t textLen = 0);
    virtual void emptyTitleLine(uint32_t textLen = 0);
    virtual void printerRefresh();
private:
    ArmPictures arms;       // this frame's forearms, when the terminal has room for them
    uint32_t armsShown = 0; // 0 none, 1 the right arm, 2 both
};

#endif