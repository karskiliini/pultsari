#include "printerterm.hpp"
#include "printerncurses.hpp"
#include "printerweb.hpp"
#include "pultsari.hpp"
#include "common.hpp"

int main(int argc, char *argv[])
{
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
