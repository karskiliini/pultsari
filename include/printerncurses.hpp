#ifndef PRINTERNCURSES_HPP
#define PRINTERNCURSES_HPP

#include "printer.hpp"

class PrinterNcurses : public Printer {
public:
    const std::string TOP_LEFT = "╔";
    const std::string BOT_LEFT = "╚";
    const std::string TOP_RIGHT = "╗";
    const std::string BOT_RIGHT = "╝";
    const std::string VERTICAL = "║";
    const std::string HORIZONTAL = "═";

    PrinterNcurses();
    ~PrinterNcurses();

    virtual void clear();
    virtual void endIntro();
    virtual void endGame();
    virtual void show_console_cursor(const bool show);
    virtual void cursorHome();
    virtual void printMore();
    virtual void setMessage(const std::string& message);
    virtual void printMessage();
    virtual void printLeftBorderSpaces();
    virtual void printBorder(Level* l, bool top);
    virtual void printVBorder(Level* l);
    virtual void printInventory(uint32_t y, Inventory* inventory);
    virtual void printStats(Level* l, Stats* stats);
    virtual void printChar(std::string c);
    virtual void printScore(const ScoreBoard* scoreBoard);
    virtual void printHelp();
    virtual void emptyLine(uint32_t textLen = 0);
    virtual void emptyTitleLine(uint32_t textLen = 0);
    virtual void printerRefresh();

    virtual void printLine(Level* l, uint32_t y, const VisionNS::Mask* mask, PathNS::PathMask* pathmask);
    virtual void print(Level* level);

    void *mainwindow = nullptr;
    void *gamewindow = nullptr;
    void *inventorywindow = nullptr;
    void *msgwindow = nullptr;
    void *statswindow = nullptr;
    void *engameWindow = nullptr;
};

#endif