#ifndef PRINTER_HPP
#define PRINTER_HPP

#include "level.hpp"
#include <string>

namespace PlayerNS {
    class Player;
}

class Printer {
public:
    static void raw();
    static void noraw();

    void showMessage(std::string message, Level& level, bool waitKey = true);
    void removeMessage();

    void print(Level& l);

    std::string msg = "";
    bool msgShown = false;

    PlayerNS::Player* player;
};

#endif