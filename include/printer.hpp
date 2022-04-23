#ifndef PRINTER_HPP
#define PRINTER_HPP

#include "level.hpp"
#include <string>

namespace PlayerNS {
    class Player;
}

namespace VisionNS {
    class Mask;
}

namespace PathNS {
    class PathMask;
}

class Printer {
public:
    static void raw();
    static void noraw();

    void setMessage(const std::string& message);
    void showMessage(std::string message, Level& level, bool waitKey = true);
    void removeMessage();

    void print(Level& level);

    std::string msg = "";
    bool msgShown = false;

    PlayerNS::Player* player = nullptr;
    VisionNS::Mask* mask = nullptr;
    PathNS::PathMask* pathmask = nullptr;
};

#endif