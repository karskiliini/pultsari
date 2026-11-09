#include "printerterm.hpp"
#include "printerncurses.hpp"
#include "printerweb.hpp"
#include "pultsari.hpp"
#include "common.hpp"
#include "version.hpp"
#include <cstdio>
#include <cstring>

int main(int argc, char *argv[])
{
    // before any printer touches the terminal
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::puts(gameVersion());
            return 0;
        }
    }

#ifdef USE_NCURSES
    PrinterNcurses printer;
    return init(argc, argv, &printer);
#else
    // the browser client gets JSON frames instead of a terminal
    if (common::webMode()) {
        PrinterWeb printer;
        return init(argc, argv, &printer);
    }
    PrinterTerm printer;
    return init(argc, argv, &printer);
#endif
}
