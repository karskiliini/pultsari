#include "intro.hpp"
#include "printer.hpp"
#include "input.hpp"
#include "common.hpp"
#include <string>
#include <iostream>

namespace IntroNS {

using std::string;
using std::cout;
using std::endl;


Intro::~Intro()
{
    printer->endIntro();
}

static void typewrite(const string& text, uint32_t delayMs)
{
    for (size_t i = 0; i < text.length(); ++i) {
        cout << text[i];
        cout.flush();
        common::sleep(delayMs);
    }
}

void Intro::show()
{
    const string anykey = "Paina nappia jatkaaksesi";

    printer->cursorHome();

    // center position for title
    constexpr int CENTER_X = 30;
    constexpr int TITLE_Y = 4;

    // clear screen
    cout << "\033[2J\033[H";

    // typewrite the title letter by letter
    char posBuf[64];
    snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", TITLE_Y, CENTER_X);
    cout << posBuf;

    // "PПLTSARI 2: " appears letter by letter in bold white
    cout << "\033[1;37m";
    typewrite("P" "\xD0\x9F" "LTSA" "\xD0\xAF" "I 2: ", 120);
    cout << "\033[0m";

    // subtitle sits right after the 12-column "PПLTSAЯI 2: "
    const int SUB_X = CENTER_X + 12;
    const int CYR_Y = TITLE_Y + 7;
    auto at = [&](int y, int x) {
        snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", y, x);
        cout << posBuf;
    };

    common::sleep(500);
    cout << "\033[1;37mRusski Mir\033[0m";
    cout.flush();
    common::sleep(1200);

    at(TITLE_Y + 2, CENTER_X);
    cout << "\033[33m*R\xC3\x96KH*\033[0m";
    cout.flush();
    common::sound("cough");
    common::sleep(600);

    // vomit hits the subtitle
    at(TITLE_Y, SUB_X);
    cout << "\033[32;1mRusski Mir\033[0m";
    cout.flush();
    common::sleep(400);

    at(TITLE_Y + 4, CENTER_X - 2);
    cout << "\033[33;1m*BLAAARGH*\033[0m";
    cout.flush();
    common::sound("vomit");
    common::sleep(600);

    // drip trails run down from the letters
    const char* trail[] = { ":", ".", "|", ":", ".", ";", "'" };
    for (int row = 1; row < CYR_Y - TITLE_Y; ++row) {
        for (int i = 0; i < 11; ++i) {
            if ((i * 7 + row * 3) % 5 == 0) continue;
            at(TITLE_Y + row, SUB_X + i);
            cout << "\033[32;2m" << trail[(i + row) % 7] << "\033[0m";
        }
        cout.flush();
        common::sleep(120);
    }

    at(TITLE_Y + 3, SUB_X + 13);
    cout << "\033[33;1m*\xC3\x96\xC3\x96\xC3\x96RRRKKK*\033[0m";
    cout.flush();
    common::sound("vomit2");
    common::sleep(500);

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
            common::sleep(14);
            at(y, SUB_X + i);
            cout << " ";
        }
        at(CYR_Y, SUB_X + i);
        cout << "\033[1;31m" << cyrillic[i] << "\033[0m";
        cout.flush();
        common::sound("drip", 0.5f);
        common::sleep(50);
    }
    common::sleep(300);

    at(CYR_Y + 1, SUB_X - 2);
    cout << "\033[32;2m~.~~~.~..~~.~~.\033[0m";
    cout.flush();
    common::sleep(500);

    // balalaika sounds
    snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", TITLE_Y + 13, CENTER_X - 5);
    cout << posBuf;
    cout << "\033[2;37m\xE2\x99\xAA \xD0\x91\xD0\xB0\xD0\xBB\xD0\xB0\xD0\xBB\xD0\xB0\xD0\xB9\xD0\xBA\xD0\xB0 \xE2\x99\xAA"
            "  tiling-tiling-tiling...  "
            "\xE2\x99\xAA \xD0\xBA\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xBD\xD0\xBA\xD0\xB0 \xE2\x99\xAA\033[0m";
    cout.flush();
    common::sound("balalaika");
    common::sleep(1500);

    snprintf(posBuf, sizeof(posBuf), "\033[%d;%dH", TITLE_Y + 24, CENTER_X);
    cout << posBuf;
    cout << anykey;

    cout.flush();

    InputNS::Input::waitKey();

    printer->clear();
}

}