#ifndef INPUT_HPP
#define INPUT_HPP

#include <cstdint>

namespace InputNS {

enum InputType {
    inputpending,
    right,
    down,
    left,
    up,
    throwing,
    drinking,
    eating,
    quit
};

enum DrinkType {
    drinkpending,
    nodrink,
    kalja,
    lonkka
};

enum FoodType {
    foodpending,
    nofood,
    foodlenkki,
    foodomppo,
    foodbansku,
    foodkala
};

enum InputAlkoType {
    alkopending,
    ostakalja,
    ostalonkka,
    poistualko
};

class Input {
public:
    Input() = default;
    ~Input() = default;

    static void raw();
    static void noraw();

    static void waitKey();
    static InputType getInput();
    static DrinkType getDrink();
    static FoodType getFood();
    static uint32_t getThrow();
    static InputAlkoType getInputAlko();
    static bool getInputPutka();
    static uint32_t getInputDivari();
    static char inputYka();
};

}

#endif