#include "printerterm.hpp"
#include "subtitles.hpp"
#include "vehicle.hpp"
#include "level.hpp"
#include "player.hpp"
#include "scoreboard.hpp"
#include "common.hpp"
#include "inventory.hpp"
#include "version.hpp"
#include "armart.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

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

namespace {

constexpr const char* FRAME = "\033[0;31m";
constexpr const char* RESET = "\033[0m";
constexpr uint32_t SIDE_W = 13;
constexpr uint32_t LINE_W = SIDE_W + common::VIEWPORT_W + 2;

// visible columns of a UTF-8 string without escape sequences
uint32_t columns(const string& s)
{
    uint32_t n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
    return n;
}

string truncateColumns(const string& s, uint32_t maxCols)
{
    uint32_t n = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (((unsigned char)s[i] & 0xC0) != 0x80 && n++ == maxCols)
            return s.substr(0, i);
    }
    return s;
}

string pad(const string& s, uint32_t width)
{
    string out = truncateColumns(s, width);
    for (uint32_t n = columns(out); n < width; ++n) out += ' ';
    return out;
}

}

void PrinterTerm::cursorHome()
{
    cout << "\e[f\e[K";
}

void PrinterTerm::printMore()
{
    cout << "\033[1;33m <LIS\xC3\x84\xC3\x84>\033[0m";
}

void PrinterTerm::printMessage()
{
    if (msg.empty()) return;
    cout << "\033[1;31m \xC2\xBB \033[1;97m" << truncateColumns(msg, LINE_W - 12) << RESET;
}

void PrinterTerm::printLeftBorderSpaces()
{
    cout << string(SIDE_W, ' ');
}

void PrinterTerm::printBorder(Level* l, bool top)
{
    printLeftBorderSpaces();
    cout << FRAME << (top ? TOP_LEFT : BOT_LEFT);

    string left = top ? " GRAND TYR MOSCOW \xC2\xB7 \xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80 " : "";
    const string place = top ? locationName(l) : "";
    string right = !place.empty() ? " " + truncateColumns(place, 36) + " " : "";
    uint32_t used = columns(left) + columns(right) + 4 + (right.empty() ? 2 : 0);
    uint32_t fill = common::VIEWPORT_W > used ? common::VIEWPORT_W - used : 0;

    if (top) {
        cout << HORIZONTAL << HORIZONTAL << "\033[1;97;41m" << left << FRAME;
        for (uint32_t x = 0; x < fill; ++x) cout << HORIZONTAL;
        cout << "\033[1;33m" << right << FRAME;
        if (right.empty()) cout << HORIZONTAL << HORIZONTAL;
        cout << HORIZONTAL << HORIZONTAL;
    } else {
        // the version sits in the bottom-right corner
        const string ver = string(" ") + gameVersion() + " ";
        const uint32_t verW = columns(ver);
        const uint32_t line = common::VIEWPORT_W > verW + 2 ? common::VIEWPORT_W - verW - 2 : 0;
        for (uint32_t x = 0; x < line; ++x) cout << HORIZONTAL;
        cout << "\033[2;37m" << ver << FRAME << HORIZONTAL << HORIZONTAL;
    }

    cout << (top ? TOP_RIGHT : BOT_RIGHT) << RESET << "\033[K" << endl;
}

void PrinterTerm::printVBorder(Level* l)
{
    cout << FRAME << VERTICAL << RESET;
}

void PrinterTerm::printInventory(uint32_t y, Inventory* inventory)
{
    if (y == 0) {
        cout << "\033[1;31m" << pad(" \xD0\xA2\xD0\x9E\xD0\x92\xD0\x90\xD0\xA0\xD0\xAB", SIDE_W) << RESET;
        return;
    }

    // each slot takes two rows: its name, then what is in it
    const auto slots = overlayItems(inventory);
    const uint32_t row = y - 1;
    if (row < slots.size() * 2) {
        const auto& s = slots[row / 2];
        if (row % 2 == 0) cout << "\033[2;37m" << pad(s.name, SIDE_W) << RESET;
        else cout << (s.count ? "\033[37m" : "\033[2;37m") << pad(" " + (s.count ? s.thing : string("-")), SIDE_W) << RESET;
        return;
    }
    if (inventory->propusk && row == slots.size() * 2) {
        cout << "\033[37m" << pad("PROPUSK", SIDE_W) << RESET;
        return;
    }
    printLeftBorderSpaces();
}

// an arm row in the 256 colours of its mask letters (see armColours)
static string armRow(const ArmPictures& a, bool left, uint32_t row)
{
    const string& r = (left ? a.left : a.right)[row];
    const string& mask = (left ? a.leftColours : a.rightColours)[row];
    static const string letters = "JSGVALCMWRYKBODN";
    static const char* codes[] = { "101", "180", "108", "195", "178", "255", "250", "245", "94", "160", "220", "64",
                                   "67", "100", "241", "246" };
    string out;
    char current = ' ';
    for (size_t i = 0; i < r.size(); ++i) {
        const char m = i < mask.size() ? mask[i] : ' ';
        if (m != current) {
            const size_t k = letters.find(m);
            out += k == string::npos ? string(RESET) : string("\033[0;38;5;") + codes[k] + "m";
            current = m;
        }
        out += r[i];
    }
    return out + RESET;
}

void PrinterTerm::printRightColumn(uint32_t y)
{
    // the forearms right of the map, only when the terminal is wide enough: the right arm
    // needs 15 more columns, both arms 30
    if (y == 0) {
        winsize ws{};
        const uint32_t cols = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 ? ws.ws_col : 0;
        armsShown = !player || cols < LINE_W + 1 + ARM_W ? 0 : cols < LINE_W + 2 * (1 + ARM_W) ? 1 : 2;
        if (armsShown) arms = playerArms(player);
    }
    if (armsShown && y < (uint32_t)ARM_H) {
        if (armsShown == 2) cout << armRow(arms, true, y) << ' ';
        cout << armRow(arms, false, y);
    } else if (armsShown && y == (uint32_t)ARM_H) {
        // what each hand holds, under its picture
        if (armsShown == 2) cout << "\033[37m" << pad(arms.leftText.empty() ? "-" : arms.leftText, ARM_W) << ' ';
        cout << "\033[37m" << pad(arms.rightText.empty() ? "-" : arms.rightText, ARM_W) << RESET;
    }
    cout << "\033[K";
}

void PrinterTerm::printStats(Level* l, Stats* stats)
{
    const char* promilleColor = stats->promilles < 10 ? "92" : stats->promilles < 30 ? "93" : "1;91";
    const char* healthColor = stats->health < 5 ? "1;91" : "97";
    const char* bellyColor = stats->hunger <= 20 ? "1;91" : stats->hunger <= 50 ? "93" : "97";

    printLeftBorderSpaces();
    cout << " \033[2;37mRUPLAT\033[0;1;93m " << moneyText(*stats) << " \xE2\x82\xBD"
         << "   \033[2;37mVOIMA\033[0;1;" << healthColor << "m " << stats->health
         << "   \033[2;37mPROMILLE\033[0;1;" << promilleColor << "m " << promilleAligned(stats->promilles) << " \xE2\x80\xB0"
         << "   \033[2;37mVATSA\033[0;1;" << bellyColor << "m " << stats->hunger << "%";
    const uint32_t sick = player ? player->diseaseStage() : 0;
    if (sick) cout << "   \033[2;37mTERVEYS\033[0;1;91m " << Disease::hudLabel(sick);
    if (l && l->isWarLevel && player) {
        // five pips: how much of him the other side can see
        const uint32_t pips = (player->exposure + 19) / 20;
        const char* c = pips >= 4 ? "1;91" : pips >= 2 ? "1;93" : "92";
        cout << "   \033[2;37mALTISTUS\033[0;" << c << "m ";
        for (uint32_t i = 0; i < 5; ++i) cout << (i < pips ? "\xE2\x96\xAE" : "\xE2\x96\xAF");
    }
    cout
         << RESET << "\033[K" << endl;

    printLeftBorderSpaces();
    if (player && player->driving) {
        // at the wheel the gauges take the line the sounds would use
        cout << " " << drivingHud(player->driving);
    } else if (player) {
        auto heard = l->audio.getHeardSounds(player->coord, player->hearingPercent());
        uint32_t used = 9;
        bool first = true;
        if (!heard.empty()) cout << " \033[2;36mKUULET:\033[0m";
        for (const auto& s : heard) {
            if (s.text.empty()) continue;
            uint32_t w = columns(s.text) + 3;
            if (used + w > common::VIEWPORT_W) break;
            bool loud = s.type == SoundType::Gunshot || s.type == SoundType::Explosion || s.type == SoundType::Siren;
            cout << (first ? " " : " \033[2;36m|\033[0m ") << (loud ? "\033[1;91m" : "\033[36m") << s.text << RESET;
            used += w;
            first = false;
        }
    }
    cout << "\033[K" << endl;
}

void PrinterTerm::printSubtitle()
{
    const string& line = Subtitles::current();
    if (line.empty()) return;
    // centred on the map's second-to-last row, like a film's subtitle; it dims as it goes
    const string text = "  " + truncateColumns(line, common::VIEWPORT_W - 6) + "  ";
    const uint32_t w = columns(text);
    const uint32_t col = SIDE_W + 2 + (common::VIEWPORT_W - w) / 2;
    const uint32_t row = common::VIEWPORT_H + 1;   // map row y is screen row y + 3
    const char* style = Subtitles::leftMs() < 600 ? "\033[0;2;37;40m" : "\033[0;1;93;40m";
    cout << "\0337\033[" << row << ";" << col << "H" << style << text << RESET << "\0338";
}

void PrinterTerm::printChar(string c)
{
    std::cout << c;
}

void PrinterTerm::printScore(const ScoreBoard* scoreBoard)
{
    const char* pad = "                    ";
    cout << "\033[J" << endl << endl;
    cout << pad << "\033[1;97;41m  \xD0\x9F\xD0\xA0\xD0\x9E\xD0\xA2\xD0\x9E\xD0\x9A\xD0\x9E\xD0\x9B \xC2\xB7 P\xC3\x96YT\xC3\x84KIRJA  \033[0m" << endl << endl;
    cout << pad << "\033[2;37mHakatut babuškat     \033[0;1;97m" << scoreBoard->mummot << RESET << endl;
    cout << pad << "\033[2;37mKaikki hakatut     \033[0;1;97m" << scoreBoard->kaikki << RESET << endl;
    cout << pad << "\033[31m\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"
                   "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"
                   "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80" << RESET << endl;
    cout << pad << "\033[1;93mPISTEET            " << scoreBoard->mummot + scoreBoard->kaikki << RESET << endl << endl;
    cout << pad << "\033[2;37m\xD0\x92\xD1\x81\xD1\x91 \xD0\xB8\xD0\xB4\xD1\x91\xD1\x82 \xD0\xBF\xD0\xBE \xD0\xBF\xD0\xBB\xD0\xB0\xD0\xBD\xD1\x83. (Kaikki menee suunnitelman mukaan.) Kiitos pelaamisesta.\033[0m" << endl << endl;
}

void PrinterTerm::printHelp()
{
    cout << "GRAND TYR MOSCOW \xC2\xB7 \xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80" << endl << endl;
    cout << "  --rt, --realtime   reaaliaikatila (suositeltu)" << endl;
    cout << "  --los              n\xC3\xA4k\xC3\xB6kentt\xC3\xA4" << endl;
    cout << "  --anims            animaatiot" << endl;
    cout << "  --path             NPC:iden reitinhaku" << endl;
    cout << "  --car              auto avaimineen heti vieress\xC3\xA4 (ajamisen kokeiluun)" << endl;
    cout << "  --front            aloita suoraan rintamalta (FPV-droonien kokeiluun)" << endl;
    cout << "  --front=N          aloita rintaman tasolta N (1-3; 3 = Bradley-hyökkäys)" << endl;
    cout << "  --version          pelin versio" << endl;
    cout << "  --all, -a          kaikki yll\xC3\xA4 olevat" << endl << endl;
    cout << "Reaaliaikatilan n\xC3\xA4pp\xC3\xA4imet:" << endl;
    cout << "  W A S D / I J K L  liiku (k\xC3\xA4vele p\xC3\xA4in = ly\xC3\xB6 / poimi / avaa)" << endl;
    cout << "  Q                  juo (huikka tai pullo kerralla)" << endl;
    cout << "  E                  sy\xC3\xB6 k\xC3\xA4""dest\xC3\xA4 (kysyy kumpi, jos molemmissa ruokaa)" << endl;
    cout << "  Tab / F            tavarat / heit\xC3\xA4: W/S valitse, E k\xC3\xA4yt\xC3\xA4, F heit\xC3\xA4 (ase tai lipas lasketaan maahan)" << endl;
    cout << "  T\xC3\xA4ht\xC3\xA4ys            WASD k\xC3\xA4\xC3\xA4nn\xC3\xA4 45\xC2\xB0, v\xC3\xA4lily\xC3\xB6nti heit\xC3\xA4/ammu, Esc peru" << endl;
    cout << "  H                  k\xC3\xA4sien esittely: k\xC3\xA4""det inventaariona, hiha terveyten\xC3\xA4, tekstitykset" << endl;
    cout << "  Z                  restart" << endl << endl;
    cout << "Tavoite: p\xC3\xA4\xC3\xA4se Vovan viinakellariin Kremliss\xC3\xA4." << endl;
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