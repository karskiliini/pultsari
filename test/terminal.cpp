#include "input.hpp"
#include <cassert>
#include <termios.h>
#include <unistd.h>
#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif
int main() {
    int master, slave;
    assert(openpty(&master, &slave, nullptr, nullptr, nullptr) == 0);
    const int savedInput = dup(STDIN_FILENO);
    assert(dup2(slave, STDIN_FILENO) >= 0);
    termios before{}, raw{}, after{};
    assert(tcgetattr(STDIN_FILENO, &before) == 0);
    InputNS::Input::raw();
    assert(tcgetattr(STDIN_FILENO, &raw) == 0);
    assert((raw.c_lflag & (ICANON | ECHO)) == 0);
    InputNS::Input::raw(); // Nested calls must not replace the original state.
    InputNS::Input::noraw();
    assert(tcgetattr(STDIN_FILENO, &after) == 0);
    // macOS sets the transient PENDIN flag when canonical mode is restored.
    const auto semanticFlags = ICANON | ECHO | ISIG | IEXTEN;
    assert((before.c_lflag & semanticFlags) == (after.c_lflag & semanticFlags));
    assert(before.c_iflag == after.c_iflag);
    assert(before.c_oflag == after.c_oflag && before.c_cflag == after.c_cflag);
    assert(dup2(savedInput, STDIN_FILENO) >= 0);
    close(savedInput); close(master); close(slave);
}
