#ifndef PRINTERWEB_HPP
#define PRINTERWEB_HPP

#include "printerterm.hpp"

// Renders for the browser client: instead of drawing a terminal it sends each
// frame as a JSON message (map rows as glyph runs with SGR colours, HUD,
// message, heard sounds, inventory), which the page draws on a canvas.
class PrinterWeb : public PrinterTerm {
public:
    virtual void clear();
    virtual void endIntro() { }
    virtual void endGame() { }
    virtual void show_console_cursor(const bool show) { }
    virtual void cursorHome() { }
    virtual void printMore() { }
    virtual void printMessage() { }
    virtual void printChar(std::string c);
    virtual void printScore(const ScoreBoard* scoreBoard);
    virtual void printHelp();
    virtual void printerRefresh() { }
    virtual void print(Level* level);
    virtual void printInventoryOverlay(Inventory* inventory, int selectedRow, bool throwMenu = false);
    virtual void printAimLine(Level* level, uint32_t px, uint32_t py, float angle);
};

#endif
