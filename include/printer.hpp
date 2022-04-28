#ifndef PRINTER_HPP
#define PRINTER_HPP

#include <string>

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
struct ScoreBoard;
struct Inventory;
struct Stats;

class Printer {
public:
    const std::string TOP_LEFT = "╔";
    const std::string BOT_LEFT = "╚";
    const std::string TOP_RIGHT = "╗";
    const std::string BOT_RIGHT = "╝";
    const std::string VERTICAL = "║";
    const std::string HORIZONTAL = "═";

    virtual void cursorHome() = 0;
    virtual void printMore() = 0;
    virtual void printHalfTab() = 0;
    virtual void printLeftBorderSpaces() = 0;
    virtual void printBorder(Level* l, bool top) = 0;
    virtual void printInventory(uint32_t y, Inventory* inventory) = 0;
    virtual void printStats(Level* l, Stats* stats) = 0;
    virtual void printChar(std::string c) = 0;
    virtual void printScore(const ScoreBoard* scoreBoard) = 0;
    virtual void printHelp() = 0;
    virtual void emptyLine(uint32_t textLen = 0) = 0;
    virtual void emptyTitleLine(uint32_t textLen = 0) = 0;

    virtual std::string inventoryAligned(uint32_t value) const;
    virtual std::string moneyAligned(uint32_t value) const;
    virtual std::string promilleAligned(uint32_t value) const;
    void setMessage(const std::string& message);
    void showMessage(std::string message, Level* level, bool waitKey = true);
    void removeMessage();

    void printLine(Level* l, uint32_t y, const VisionNS::Mask* mask, PathNS::PathMask* pathmask);
    virtual void print(Level* level);
    virtual void printErr(const std::string& msg);

    std::string msg = "";
    bool msgShown = false;

    PlayerNS::Player* player = nullptr;
    VisionNS::Mask* mask = nullptr;
    PathNS::PathMask* pathmask = nullptr;
};

#endif