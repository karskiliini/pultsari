#include "printerterm.hpp"
#include "level.hpp"
#include "player.hpp"
#include "scoreboard.hpp"
#include "common.hpp"
#include "inventory.hpp"
#include <cstdio>
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

struct SideEntry { const char* name; uint32_t count; };

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
        for (uint32_t x = 0; x < common::VIEWPORT_W; ++x) cout << HORIZONTAL;
    }

    cout << (top ? TOP_RIGHT : BOT_RIGHT) << RESET << "\033[K" << endl;
}

void PrinterTerm::printVBorder(Level* l)
{
    cout << FRAME << VERTICAL << RESET;
}

void PrinterTerm::printInventory(uint32_t y, Inventory* inventory)
{
    const auto& i = *inventory;
    const SideEntry all[] = {
        {"PIVO", i.kalja}, {"JAGUAR", i.lonkka}, {"VODKA", i.vodka},
        {"MAKKARA", i.lenkki}, {"VOBLA", i.kalat}, {"ANTONOVKA", i.omppo},
        {"BANAANI", i.bansku}, {"KETJUT", i.ketjut}, {"VEITSET", i.veitset},
        {"KIVET", i.kivet}, {"PAMPUT", i.pamput}, {"VERKKARIT", i.bootsit},
        {"KRANAATIT", i.granata}, {"PISTOOLI", i.pistooli}, {"AMMUKSET", i.ammukset},
        {"PROPUSK", i.propusk},
    };

    if (y == 0) {
        cout << "\033[1;31m" << pad(" \xD0\xA2\xD0\x9E\xD0\x92\xD0\x90\xD0\xA0\xD0\xAB", SIDE_W) << RESET;
        return;
    }

    uint32_t row = 0;
    for (const auto& e : all) {
        if (e.count == 0) continue;
        if (++row != y) continue;
        char buf[32];
        snprintf(buf, sizeof(buf), "%-9s%2u  ", e.name, e.count > 99 ? 99 : e.count);
        cout << "\033[37m" << buf << RESET;
        return;
    }
    if (row == 0 && y == 1) {
        cout << "\033[2;37m" << pad(" (tyhj\xC3\xA4)", SIDE_W) << RESET;
        return;
    }
    printLeftBorderSpaces();
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
    cout
         << RESET << "\033[K" << endl;

    printLeftBorderSpaces();
    if (player) {
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
    cout << "  --all, -a          kaikki yll\xC3\xA4 olevat" << endl << endl;
    cout << "Reaaliaikatilan n\xC3\xA4pp\xC3\xA4imet:" << endl;
    cout << "  W A S D / I J K L  liiku (k\xC3\xA4vele p\xC3\xA4in = ly\xC3\xB6 / poimi / avaa)" << endl;
    cout << "  Q (tai Tab) / F     tavarat / heit\xC3\xA4: W/S valitse, E k\xC3\xA4yt\xC3\xA4, F heit\xC3\xA4" << endl;
    cout << "  T\xC3\xA4ht\xC3\xA4ys            A/D k\xC3\xA4\xC3\xA4nn\xC3\xA4, v\xC3\xA4lily\xC3\xB6nti heit\xC3\xA4/ammu, Esc peru" << endl;
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