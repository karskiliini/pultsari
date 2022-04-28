#ifndef INTRO_HPP
#define INTRO_HPP

class Printer;

namespace IntroNS {

class Intro {
public:
    Intro(Printer* p) { printer = p; }
    ~Intro() = default;

    void show();
    Printer* printer = nullptr;
};

}

#endif