#include "intro.hpp"
#include "input.hpp"
#include "common.hpp"
#include <string>
#include <iostream>

namespace IntroNS {

using std::string;
using std::cout;
using std::endl;
using std::cin;

string name1 = "(C) By Mika  Maaranen";
string name2 = "       Jari  Maaranen";
string name3 = "       Tero  Maaranen";
string name4 = "       Kimmo Korhonen";

void emptyTitleLine(uint32_t textLen = 0)
{
    for (uint32_t i = 0; i < common::SIZEX / 2 - textLen; ++i) {
        cout << " ";
    }
}

void emptyLine(uint32_t textLen = 0)
{
    for (uint32_t i = 0; i < common::SIZEX - textLen; ++i) {
        cout << " ";
    }
    cout << endl;
}

void Intro::show()
{
    const string pulsu = "PULTSARI";
    const string anykey = "Press any key to continue";

    common::cursorHome();

    emptyTitleLine(pulsu.length() / 2);
    cout << pulsu;
    emptyTitleLine(pulsu.length() / 2);
    cout << endl;

    for (uint32_t i = 0; i < 7 ; ++i) {
        emptyLine();
    }

    cout << name1;
    emptyLine(name1.length());
    cout << name2;
    emptyLine(name2.length());
    cout << name3;
    emptyLine(name3.length());
    cout << name4;
    emptyLine(name4.length());

    emptyLine();
    cout << anykey;
    emptyLine(anykey.length());

    for (uint32_t i = 0; i < 6; ++i) {
        emptyLine();
    }

    cout << "28.12.1991 - 14.1.1992 (Välillä paussi)" << endl;
    emptyLine();

    InputNS::Input::waitKey();
}

}