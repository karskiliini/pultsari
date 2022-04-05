#ifndef INPUT_HPP
#define INPUT_HPP

namespace InputNS {

enum InputType {
    inputpending,
    right,
    down,
    left,
    up,
    throwing,
    drinking,
    quit
};

enum DrinkType {
    drinkpending,
    nodrink,
    kalja,
    lonkka
};

class Input {
public:
    Input() = default;
    ~Input() = default;

    static void raw();
    static void noraw();

    static InputType getInput();
    static DrinkType getDrink();
};

}

#endif