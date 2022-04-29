#include "printerterm.hpp"
#include "printerncurses.hpp"
#include "pultsari.hpp"

int main(int argc, char *argv[])
{

#if 0
    PrinterNcurses printer;
#else
    PrinterTerm printer;
#endif
    return init(argc, argv, &printer);
}
