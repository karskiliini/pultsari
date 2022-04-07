#include "intro.hpp"
#include "input.hpp"
#include <string>
#include <iostream>

namespace IntroNS {

using std::string;
using std::cout;
using std::endl;
using std::cin;

string names = "(C) By Mika  Maaranen\n\
       Jari  Maaranen\n\
       Tero  Maaranen\n\
       Kimmo Korhonen\n";

void Intro::show()
{
    for (uint32_t i = 0; i < 20; ++i) {
        cout << endl;
    }

    cout << names;

    for (uint32_t i = 0; i < 24; ++i) {
        cout << endl;
    }

    InputNS::Input::waitKey();
}

}