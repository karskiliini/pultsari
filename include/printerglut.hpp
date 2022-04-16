#ifndef PRINTERGLUT_HPP
#define PRINTERGLUT_HPP

#include "level.hpp"
#include <string>

namespace PlayerNS {
    class Player;
}

namespace VisionNS {
    class Mask;
}

class PrinterGlut {
public:
    static void raw();
    static void noraw();

    void setMessage(const std::string& message);
    void showMessage(std::string message, Level& level, bool waitKey = true);
    void removeMessage();

    void print(Level& level);

    std::string msg = "";
    bool msgShown = false;

    PlayerNS::Player* player;
    VisionNS::Mask* mask;
};

#endif