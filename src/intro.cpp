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

string name1 = "(C) By Jari  Maaranen";
string name2 = "       Tero  Maaranen";
string name3 = "       Kimmo Korhonen";

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
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y, CENTER_X);
    cout << posBuf;

    // "PПLTSARI 2: " appears letter by letter in bold white
    cout << "\033[1;37m";
    typewrite("P" "\xD0\x9F" "LTSA" "\xD0\xAF" "I 2: ", 120);
    cout << "\033[0m";

    // pause, then "Russki Mir" appears in white
    common::sleep(500);
    cout << "\033[1;37mRusski Mir\033[0m";
    cout.flush();
    common::sleep(1200);

    // *RÖKH* - first cough
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2, CENTER_X + 5);
    cout << posBuf;
    cout << "\033[33m*RÖKH*\033[0m";
    cout.flush();
    common::sleep(600);

    // vomit hits the subtitle - "Russki Mir" turns green
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y, CENTER_X + 13);
    cout << posBuf;
    cout << "\033[32;1mRusski Mir\033[0m";
    cout.flush();
    common::sleep(400);

    // *BLAAARGH* - bigger vomit
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2, CENTER_X + 2);
    cout << posBuf;
    cout << "\033[33;1m*BLAAARGH*\033[0m";
    cout.flush();
    common::sleep(700);

    // drip trails fall from the Latin letters
    const char* trail[] = { ":", ".", "|", ":", ".", ";", "'" };
    for (int row = 0; row < 5; ++row) {
        for (int i = 0; i < 10; ++i) {
            int dx = CENTER_X + 13 + i;
            sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2 + row, dx);
            cout << posBuf;
            cout << "\033[32;2m" << trail[(i + row) % 7] << "\033[0m";
        }
        cout.flush();
        common::sleep(120);
    }
    common::sleep(300);

    // *ÖÖÖRRRKKK* - more vomit
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 8, CENTER_X + 16);
    cout << posBuf;
    cout << "\033[33;1m*\xC3\x96\xC3\x96\xC3\x96RRRKKK*\033[0m";
    cout.flush();
    common::sleep(600);

    // drips collect and form "Русский Мир" in Cyrillic, letter by letter
    // Р у с с к и й   М и р
    const char* cyrillic[] = {
        "\xD0\xA0", "\xD1\x83", "\xD1\x81", "\xD1\x81",
        "\xD0\xBA", "\xD0\xB8", "\xD0\xB9", " ",
        "\xD0\x9C", "\xD0\xB8", "\xD1\x80",
    };
    int cyX = CENTER_X + 12;
    int cyY = TITLE_Y + 9;
    for (int i = 0; i < 11; ++i) {
        if (cyrillic[i][0] == ' ') continue;
        // drip falls to position
        for (int dy = TITLE_Y + 2; dy <= cyY; ++dy) {
            sprintf(posBuf, "\033[%d;%dH", dy, cyX + i * 2);
            cout << posBuf;
            cout << "\033[32;2m.\033[0m";
            cout.flush();
            common::sleep(12);
            if (dy < cyY) {
                sprintf(posBuf, "\033[%d;%dH", dy, cyX + i * 2);
                cout << posBuf;
                cout << " ";
            }
        }
        // place Cyrillic letter
        sprintf(posBuf, "\033[%d;%dH", cyY, cyX + i * 2);
        cout << posBuf;
        cout << "\033[31;1m" << cyrillic[i] << "\033[0m";
        cout.flush();
        common::sleep(60);
    }
    common::sleep(400);

    // puddle under "Русский Мир"
    sprintf(posBuf, "\033[%d;%dH", cyY + 1, cyX - 2);
    cout << posBuf;
    cout << "\033[32;2m~.~~~.~..~~.~~~.~..~~.~~~.~\033[0m";
    cout.flush();
    common::sleep(300);
    sprintf(posBuf, "\033[%d;%dH", cyY + 2, cyX - 1);
    cout << posBuf;
    cout << "\033[32;2m  ~..~.~~..~.~~.~..~~.~  \033[0m";
    cout.flush();
    common::sleep(500);

    // balalaika sounds
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 13, CENTER_X - 5);
    cout << posBuf;
    cout << "\033[2;37m\xE2\x99\xAA \xD0\x91\xD0\xB0\xD0\xBB\xD0\xB0\xD0\xBB\xD0\xB0\xD0\xB9\xD0\xBA\xD0\xB0 \xE2\x99\xAA"
            "  tiling-tiling-tiling...  "
            "\xE2\x99\xAA \xD0\xBA\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xBD\xD0\xBA\xD0\xB0 \xE2\x99\xAA\033[0m";
    cout.flush();
    common::sleep(1500);

    // credits
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 16, 5);
    cout << posBuf;
    cout << name1;
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 17, 5);
    cout << posBuf;
    cout << name2;
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 18, 5);
    cout << posBuf;
    cout << name3;

    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 20, 5);
    cout << posBuf;
    cout << "Moskova 2025";

    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 24, CENTER_X);
    cout << posBuf;
    cout << anykey;

    cout.flush();

    InputNS::Input::waitKey();

    printer->clear();
}

}