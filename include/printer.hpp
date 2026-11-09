#ifndef PRINTER_HPP
#define PRINTER_HPP

#include <cstdint>
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

struct OverlayItem {
    const char* name;
    uint32_t count;
    bool canUse;
    bool canThrow;
    const char* useLabel;
};
// the inventory overlay rows, in the order the game's inventory keys expect
std::vector<OverlayItem> overlayItems(const Inventory* inventory);
// viewport cells of the aiming line
std::vector<std::pair<int, int>> aimCells(Level* level, uint32_t px, uint32_t py, float angle, int maxSteps = 20);

class Printer {
public:
    virtual void clear() { };
    virtual void endIntro() { };
    virtual void endGame() { };
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
    // one map row as (glyph, SGR colour) cells; the renderers turn it into ANSI or JSON
    std::vector<std::pair<std::string, std::string>> rowCells(Level* l, uint32_t y);
    virtual void print(Level* level);
    virtual void printErr(const std::string& msg);
    virtual void printInventoryOverlay(Inventory* inventory, int selectedRow, bool throwMenu = false);
    virtual void printAimLine(Level* level, uint32_t px, uint32_t py, float angle);

    std::string msg = "";
    bool msgShown = false;
    static constexpr uint32_t MSG_FRAMES = 60;
    uint32_t msgTtl = 0;
    std::string levelName;
    // the title bar's place name: where the player stands, else the level's name
    std::string locationName(const Level* l) const;
    // accidental-discharge risk while aiming a pistol (0..1), or negative when not shooting
    float aimRisk = -1.0f;
    // map view size in cells; grows when the camera zooms out after death
    uint32_t viewW = 80;
    uint32_t viewH = 28;
    int aimLength = 20;     // a lowered pistol's laser only reaches a few steps

    // the browser draws speech bubbles itself, translucent, over the map
    bool bubblesAsOverlay = false;

    PlayerNS::Player* player = nullptr;
    VisionNS::Mask* mask = nullptr;
    PathNS::PathMask* pathmask = nullptr;
};

#endif