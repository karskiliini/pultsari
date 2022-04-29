#ifndef PRINTER_HPP
#define PRINTER_HPP

#include <string>
#include <vector>

namespace PlayerNS {
    class Player;
}

namespace VisionNS {
    class Mask;
}

namespace PathNS {
    class PathMask;
}

class Level;
class Item;
class Person;
struct ScoreBoard;
struct Inventory;
struct Stats;

class Printer {
public:
    virtual void clear() { };
    virtual void show_console_cursor(const bool show) = 0;
    virtual void cursorHome() = 0;
    virtual void printMore() = 0;
    virtual void printMessage() = 0;
    virtual void printLeftBorderSpaces() = 0;
    virtual void printBorder(Level* l, bool top) = 0;
    virtual void printVBorder(Level* l) = 0;
    virtual void printInventory(uint32_t y, Inventory* inventory) = 0;
    virtual void printStats(Level* l, Stats* stats) = 0;
    virtual void printChar(std::string c) = 0;
    virtual void printScore(const ScoreBoard* scoreBoard) = 0;
    virtual void printHelp() = 0;
    virtual void emptyLine(uint32_t textLen = 0) = 0;
    virtual void emptyTitleLine(uint32_t textLen = 0) = 0;
    virtual void printerRefresh() = 0;

    const Person* findPerson(const std::vector<const Person*>& row, uint32_t x) const;
    const Item* findItem(const std::vector<const Item*>& row, uint32_t x) const;

    virtual std::string inventoryAligned(uint32_t value) const;
    virtual std::string moneyAligned(uint32_t value) const;
    virtual std::string promilleAligned(uint32_t value) const;
    virtual void setMessage(const std::string& message);
    void showMessage(std::string message, Level* level, bool waitKey = true);
    void removeMessage();

    virtual void printLine(Level* l, uint32_t y, const VisionNS::Mask* mask, PathNS::PathMask* pathmask);
    virtual void print(Level* level);
    virtual void printErr(const std::string& msg);

    std::string msg = "";
    bool msgShown = false;

    PlayerNS::Player* player = nullptr;
    VisionNS::Mask* mask = nullptr;
    PathNS::PathMask* pathmask = nullptr;
};

#endif