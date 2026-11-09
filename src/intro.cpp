#include "intro.hpp"
#include "printer.hpp"
#include "input.hpp"
#include "common.hpp"
#include "introart.hpp"
#include <chrono>
#include <string>
#include <iostream>
#include <vector>
#include <sys/ioctl.h>
#include <unistd.h>

namespace IntroNS {

using std::string;
using std::cout;
using std::endl;


Intro::~Intro()
{
    printer->endIntro();
}

// Space skips the intro; any other key pressed meanwhile is swallowed, not echoed
static bool skipped = false;
static void animatePicture();

static void wait(uint32_t ms)
{
    while (!skipped && ms > 0) {
        if (InputNS::Input::getRawCharNonBlocking() == ' ') { skipped = true; return; }
        animatePicture();
        const uint32_t slice = ms < 20 ? ms : 20;
        common::sleep(slice);
        ms -= slice;
    }
}

// The game ad as the background picture: the largest version that fits the
// terminal, centred under the title text. Nothing is drawn if none fits.
static const IntroArt::Picture* picture = nullptr;
static int picX = 1, picY = 1;   // screen position of its top left cell (1-based)
static std::vector<bool> covered; // picture cells under the intro's text: never animated
static std::chrono::steady_clock::time_point animStart;
static int animFrame = -1;

static void placePicture()
{
    picture = nullptr;
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0)
        return;
    for (int i = 0; i < IntroArt::PICTURE_COUNT; ++i) {
        const auto& p = IntroArt::PICTURES[i];
        // keep the last row free so the terminal never scrolls
        if (p.cols <= ws.ws_col && p.rows < ws.ws_row) {
            picture = &p;
            break;
        }
    }
    if (!picture)
        return;
    constexpr int TEXT_CENTER = 55; // middle of the title and its vomit
    picX = TEXT_CENTER - picture->cols / 2;
    if (picX + picture->cols - 1 > ws.ws_col) picX = ws.ws_col - picture->cols + 1;
    if (picX < 1) picX = 1;
    picY = 1;
    covered.assign(picture->rows * picture->cols, false);
    animFrame = -1;
}

// Text goes over the picture at screen row y, columns x .. x + width - 1.
static void cover(int y, int x, int width)
{
    if (!picture)
        return;
    for (int i = 0; i < width; ++i) {
        const int py = y - picY, px = x + i - picX;
        if (py >= 0 && px >= 0 && py < picture->rows && px < picture->cols)
            covered[py * picture->cols + px] = true;
    }
}

static string colour(int fg, int bg)
{
    return "\033[38;5;" + std::to_string(fg) + ";48;5;" + std::to_string(bg) + "m";
}

static void drawPicture()
{
    if (!picture)
        return;
    const auto& p = *picture;
    string out;
    for (int y = 0; y < p.rows; ++y) {
        out += "\033[" + std::to_string(picY + y) + ";" + std::to_string(picX) + "H";
        int lastFg = -1, lastBg = -1;
        for (int x = 0; x < p.cols; ++x) {
            const int fg = p.fg[y * p.cols + x], bg = p.bg[y * p.cols + x];
            if (fg != lastFg || bg != lastBg) {
                out += colour(fg, bg);
                lastFg = fg; lastBg = bg;
            }
            out += p.chars[y][x];
        }
        out += "\033[0m";
    }
    cout << out;
    animStart = std::chrono::steady_clock::now();
}

// Next frame of the fire and the smoke, if it is time for one. The cursor and
// the text colour are saved around it, so a title being typed goes on as before.
static void animatePicture()
{
    if (!picture || picture->animFrames == 0)
        return;
    const auto& p = *picture;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - animStart).count();
    const int frame = static_cast<int>(ms * IntroArt::ANIM_FPS / 1000 % p.animFrames);
    if (frame == animFrame)
        return;
    animFrame = frame;

    string out = "\0337";
    int lastFg = -1, lastBg = -1, lastIdx = -2;
    for (int k = 0; k < p.animCellCount; ++k) {
        const int idx = p.animCells[k];
        if (covered[idx])
            continue;
        if (idx != lastIdx + 1 || idx % p.cols == 0) {
            out += "\033[" + std::to_string(picY + idx / p.cols) + ";" + std::to_string(picX + idx % p.cols) + "H";
        }
        const int fg = p.animFg[frame * p.animCellCount + k], bg = p.animBg[frame * p.animCellCount + k];
        if (fg != lastFg || bg != lastBg) {
            out += colour(fg, bg);
            lastFg = fg; lastBg = bg;
        }
        out += p.animChars[frame][k];
        lastIdx = idx;
    }
    out += "\033[0m\0338";
    cout << out;
    cout.flush();
}
// What the picture shows at screen position (y, x), 1-based: used to repaint
// a cell after something moved over it.
static string pictureCell(int y, int x)
{
    if (!picture || y < picY || x < picX || y >= picY + picture->rows || x >= picX + picture->cols)
        return " ";
    const auto& p = *picture;
    const int py = y - picY, px = x - picX;
    return colour(p.fg[py * p.cols + px], p.bg[py * p.cols + px]) + p.chars[py][px] + "\033[0m";
}

static void typewrite(const string& text, uint32_t delayMs)
{
    for (size_t i = 0; i < text.length(); ++i) {
        cout << text[i];
        cout.flush();
        wait(delayMs);
    }
}

void Intro::show()
{
    skipped = false;
    InputNS::Input::raw();
    const string anykey = "Paina nappia jatkaaksesi";

    printer->cursorHome();

    // center position for title
    constexpr int CENTER_X = 30;
    constexpr int TITLE_Y = 4;

    // clear screen, then the game ad as the background
    cout << "\033[2J\033[H";
    placePicture();
    drawPicture();

    // typewrite the title letter by letter
    char posBuf[64];
    snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", TITLE_Y, CENTER_X);
    cout << posBuf;

    // "GRAND TYR MOSCOW: " appears letter by letter in bold white
    cover(TITLE_Y, CENTER_X, 18);
    cover(TITLE_Y, CENTER_X + 18, 10); // "Russki Mir" comes right after
    cout << "\033[1;37m";
    typewrite("GRAND TYR MOSCOW: ", 100);
    cout << "\033[0m";

    // subtitle sits right after the 18-column "GRAND TYR MOSCOW: "
    const int SUB_X = CENTER_X + 18;
    const int CYR_Y = TITLE_Y + 1; // the Cyrillic forms on the very next line
    auto at = [&](int y, int x) {
        snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", y, x);
        cout << posBuf;
    };

    wait(500);
    cout << "\033[1;37mRusski Mir\033[0m";
    cout.flush();
    wait(1200);

    cover(TITLE_Y + 2, CENTER_X, 6);
    at(TITLE_Y + 2, CENTER_X);
    cout << "\033[33m*R\xC3\x96KH*\033[0m";
    cout.flush();
    common::sound("cough");
    wait(600);

    // vomit hits the subtitle
    at(TITLE_Y, SUB_X);
    cout << "\033[32;1mRusski Mir\033[0m";
    cout.flush();
    wait(400);

    cover(TITLE_Y + 4, CENTER_X - 2, 10);
    at(TITLE_Y + 4, CENTER_X - 2);
    cout << "\033[33;1m*BLAAARGH*\033[0m";
    cout.flush();
    common::sound("vomit");
    wait(600);

    // drip trails run down from the letters
    const char* trail[] = { ":", ".", "|", ":", ".", ";", "'" };
    for (int row = 1; row < CYR_Y - TITLE_Y; ++row) {
        for (int i = 0; i < 11; ++i) {
            if ((i * 7 + row * 3) % 5 == 0) continue;
            cover(TITLE_Y + row, SUB_X + i, 1);
            at(TITLE_Y + row, SUB_X + i);
            cout << "\033[32;2m" << trail[(i + row) % 7] << "\033[0m";
        }
        cout.flush();
        wait(120);
    }

    cover(TITLE_Y + 3, SUB_X + 13, 11);
    at(TITLE_Y + 3, SUB_X + 13);
    cout << "\033[33;1m*\xC3\x96\xC3\x96\xC3\x96RRRKKK*\033[0m";
    cout.flush();
    common::sound("vomit2");
    wait(500);

    // the drips pool into "Русский Мир", one letter at a time
    const char* cyrillic[] = {
        "\xD0\xA0", "\xD1\x83", "\xD1\x81", "\xD1\x81", "\xD0\xBA", "\xD0\xB8",
        "\xD0\xB9", " ", "\xD0\x9C", "\xD0\xB8", "\xD1\x80",
    };
    for (int i = 0; i < 11; ++i) {
        for (int y = TITLE_Y + 1; y < CYR_Y; ++y) {
            at(y, SUB_X + i);
            cout << "\033[32;1mo\033[0m";
            cout.flush();
            wait(14);
            at(y, SUB_X + i);
            cout << pictureCell(y, SUB_X + i);
        }
        cover(CYR_Y, SUB_X + i, 1);
        at(CYR_Y, SUB_X + i);
        cout << "\033[1;31m" << cyrillic[i] << "\033[0m";
        cout.flush();
        common::sound("drip", 0.5f);
        wait(50);
    }
    wait(300);

    cover(CYR_Y + 1, SUB_X - 2, 15);
    at(CYR_Y + 1, SUB_X - 2);
    cout << "\033[32;2m~.~~~.~..~~.~~.\033[0m";
    cout.flush();
    wait(500);

    // balalaika sounds
    cover(TITLE_Y + 13, CENTER_X - 5, 51);
    snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", TITLE_Y + 13, CENTER_X - 5);
    cout << posBuf;
    cout << "\033[2;37m\xE2\x99\xAA \xD0\x91\xD0\xB0\xD0\xBB\xD0\xB0\xD0\xBB\xD0\xB0\xD0\xB9\xD0\xBA\xD0\xB0 \xE2\x99\xAA"
            "  tiling-tiling-tiling...  "
            "\xE2\x99\xAA \xD0\xBA\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xBD\xD0\xBA\xD0\xB0 \xE2\x99\xAA\033[0m";
    cout.flush();
    common::sound("balalaika");
    wait(1500);

    cover(TITLE_Y + 24, CENTER_X, static_cast<int>(anykey.size()));
    snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", TITLE_Y + 24, CENTER_X);
    cout << posBuf;
    cout << anykey;

    cout.flush();

    if (!skipped) {
        while (InputNS::Input::getRawCharNonBlocking() == 0) {
            animatePicture();
            common::sleep(20);
        }
    }
    InputNS::Input::noraw();

    // the picture is larger than the game screen: leave nothing of it behind
    cout << "\033[0m\033[2J\033[H";
    printer->clear();
}

}