#ifndef PRINTER_HPP
#define PRINTER_HPP

#include "level.hpp"
#include <string>

class Printer {
public:
    static void raw();
    static void noraw();

    void showMessage(std::string message);
    void removeMessage();

    void print(Level l);

    std::string msg = "";
    bool msgShown = false;
};

#endif