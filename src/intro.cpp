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

string name1 = "(C) By Mika  Maaranen";
string name2 = "       Jari  Maaranen";
string name3 = "       Tero  Maaranen";
string name4 = "       Kimmo Korhonen";

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

    // pause, then subtitle appears
    common::sleep(500);
    // "Русский Мир" in bold red
    cout << "\033[1;31m\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80\033[0m";
    cout.flush();
    common::sleep(1200);

    // *RÖKH* - first cough
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2, CENTER_X + 5);
    cout << posBuf;
    cout << "\033[33m*RÖKH*\033[0m";
    cout.flush();
    common::sleep(600);

    // vomit hits the subtitle - overwrite "Русский Мир" in green
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y, CENTER_X + 13);
    cout << posBuf;
    cout << "\033[32;1m\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80\033[0m";
    cout.flush();
    common::sleep(400);

    // *BLAAARGH* - bigger vomit
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2, CENTER_X + 2);
    cout << posBuf;
    cout << "\033[33;1m*BLAAARGH*\033[0m";
    cout.flush();
    common::sleep(700);

    // drips fall from each letter, forming "Russki Mir" below
    // drip trails: dots and colons falling from the Cyrillic
    const char* trail[] = { ":", ".", "|", ":", ".", ";", "'" };
    for (int row = 0; row < 6; ++row) {
        for (int i = 0; i < 10; ++i) {
            int dx = CENTER_X + 13 + i;
            sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2 + row, dx);
            cout << posBuf;
            if (row < 5) {
                cout << "\033[32;2m" << trail[(i + row) % 7] << "\033[0m";
            }
        }
        cout.flush();
        common::sleep(120);
    }
    common::sleep(300);

    // *ÖÖÖRRRKKK* - more vomit
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 9, CENTER_X + 16);
    cout << posBuf;
    cout << "\033[33;1m*\xC3\x96\xC3\x96\xC3\x96RRRKKK*\033[0m";
    cout.flush();
    common::sleep(600);

    // the drips collect and form "Russki Mir" letter by letter
    const char* russki_mir = "R u s s k i   M i r";
    int rmX = CENTER_X + 9;
    int rmY = TITLE_Y + 8;
    for (int i = 0; russki_mir[i]; ++i) {
        if (russki_mir[i] == ' ') continue;
        // drip falls to position
        for (int dy = TITLE_Y + 2; dy <= rmY; ++dy) {
            sprintf(posBuf, "\033[%d;%dH", dy, rmX + i);
            cout << posBuf;
            cout << "\033[32;2m.\033[0m";
            cout.flush();
            common::sleep(15);
            // erase trail
            if (dy < rmY) {
                sprintf(posBuf, "\033[%d;%dH", dy, rmX + i);
                cout << posBuf;
                cout << " ";
            }
        }
        // place the letter
        sprintf(posBuf, "\033[%d;%dH", rmY, rmX + i);
        cout << posBuf;
        cout << "\033[32;1m" << russki_mir[i] << "\033[0m";
        cout.flush();
        common::sleep(50);
    }
    common::sleep(400);

    // puddle under "Russki Mir"
    sprintf(posBuf, "\033[%d;%dH", rmY + 1, rmX - 2);
    cout << posBuf;
    cout << "\033[32;2m~.~~~.~..~~.~~~.~..~~.~\033[0m";
    cout.flush();
    common::sleep(300);
    sprintf(posBuf, "\033[%d;%dH", rmY + 2, rmX - 1);
    cout << posBuf;
    cout << "\033[32;2m  ~..~.~~..~.~~.~..~  \033[0m";
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
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 19, 5);
    cout << posBuf;
    cout << name4;

    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 21, 5);
    cout << posBuf;
    cout << "28.12.1991 - 14.1.1992 (Valissakin paussi)";
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 22, 5);
    cout << posBuf;
    cout << "Moskovan versio 2025 - \xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9 \xD0\x9C\xD0\xB8\xD1\x80";

    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 24, CENTER_X);
    cout << posBuf;
    cout << anykey;

    cout.flush();

    InputNS::Input::waitKey();

    printer->clear();
}

}