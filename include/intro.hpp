#ifndef INTRO_HPP
#define INTRO_HPP

class Printer;

namespace IntroNS {

class Intro {
public:
    Intro(Printer* p) { printer = p; }
    ~Intro();

    void show();
    Printer* printer = nullptr;
};

}

#endif