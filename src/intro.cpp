#include "intro.hpp"
#include "printer.hpp"
#include "input.hpp"
#include "common.hpp"
#include <string>
#include <iostream>

namespace IntroNS {

using std::string;

string name1 = "(C) By Mika  Maaranen";
string name2 = "       Jari  Maaranen";
string name3 = "       Tero  Maaranen";
string name4 = "       Kimmo Korhonen";

void Intro::show()
{
    const string pulsu = "PULTSARI";
    const string anykey = "Press any key to continue";

    printer->cursorHome();

    printer->emptyTitleLine(pulsu.length() / 2);
    printer->printChar(pulsu);
    printer->emptyTitleLine(pulsu.length() / 2);
    printer->printChar("\n");

    for (uint32_t i = 0; i < 7 ; ++i) {
        printer->emptyLine();
    }

    printer->printChar(name1);
    printer->emptyLine(name1.length());
    printer->printChar(name2);
    printer->emptyLine(name2.length());
    printer->printChar(name3);
    printer->emptyLine(name3.length());
    printer->printChar(name4);
    printer->emptyLine(name4.length());

    printer->emptyLine();
    printer->printChar(anykey);
    printer->emptyLine(anykey.length());

    for (uint32_t i = 0; i < 6; ++i) {
        printer->emptyLine();
    }

    printer->printChar("28.12.1991 - 14.1.1992 (Välillä paussi)");
    printer->emptyLine();

    InputNS::Input::waitKey();
}

}