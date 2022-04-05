#include "intro.hpp"
#include <string>
#include <iostream>

namespace IntroNS {

using std::string;
using std::cout;
using std::cin;

string name = "(C) By Mika  Maaranen\n\
       Jari  Maaranen\n\
       Tero  Maaranen\n\
       Kimmo Korhonen\n";

void Intro::show()
{
    cout << name;
    auto c = getchar();

    // suppress compiler warning
    c = c;
}

}