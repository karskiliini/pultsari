#ifndef PRINTERTERM_HPP
#define PRINTERTERM_HPP

#include "printer.hpp"

class PrinterTerm : public Printer {
public:
    virtual void cursorHome();
    virtual void printMore();
    virtual void printHalfTab();
    virtual void printLeftBorderSpaces();
    virtual void printBorder(Level* l, bool top);
    virtual void printInventory(uint32_t y, Inventory* inventory);
    virtual void printStats(Level* l, Stats* stats);
    virtual void printChar(std::string c);
    virtual void printScore(const ScoreBoard* scoreBoard);
    virtual void printHelp();
    virtual void emptyLine(uint32_t textLen = 0);
    virtual void emptyTitleLine(uint32_t textLen = 0);
};

#endif