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

    // pause, then first letter hints
    common::sleep(500);
    cout << "\033[1;37mS D O\033[0m";
    cout.flush();
    common::sleep(800);

    // *RÖKH* - first cough
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2, CENTER_X + 5);
    cout << posBuf;
    cout << "\033[33m*RÖKH*\033[0m";
    cout.flush();
    common::sleep(600);

    // "Special" drips down in green vomit text
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y, CENTER_X + 17);
    cout << posBuf;
    cout << "          ";
    cout.flush();

    const char* special = "Special";
    for (int i = 0; special[i]; ++i) {
        sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2 + i, CENTER_X + 13);
        cout << posBuf;
        cout << "\033[32;1m" << special[i] << "\033[0m";
        cout.flush();
        common::sleep(80);
    }
    common::sleep(400);

    // *BLAAARGH* - bigger vomit
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 10, CENTER_X + 2);
    cout << posBuf;
    cout << "\033[33;1m*BLAAARGH*\033[0m";
    cout.flush();
    common::sleep(700);

    // "Drunken" drips down
    const char* drunken = "Drunken";
    for (int i = 0; drunken[i]; ++i) {
        sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2 + i, CENTER_X + 15);
        cout << posBuf;
        cout << "\033[32;1m" << drunken[i] << "\033[0m";
        cout.flush();
        common::sleep(80);
    }
    common::sleep(400);

    // *ÖÖÖRRRKKK* - final vomit
    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 10, CENTER_X + 16);
    cout << posBuf;
    cout << "\033[33;1m*\xC3\x96\xC3\x96\xC3\x96RRRKKK*\033[0m";
    cout.flush();
    common::sleep(700);

    // "Operation" drips down
    const char* operation = "Operation";
    for (int i = 0; operation[i]; ++i) {
        sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 2 + i, CENTER_X + 17);
        cout << posBuf;
        cout << "\033[32;1m" << operation[i] << "\033[0m";
        cout.flush();
        common::sleep(80);
    }

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
    cout << "Moskovan versio 2025 - Tavoite: Putinin viinakellariin!";

    sprintf(posBuf, "\033[%d;%dH", TITLE_Y + 24, CENTER_X);
    cout << posBuf;
    cout << anykey;

    cout.flush();

    InputNS::Input::waitKey();

    printer->clear();
}

}