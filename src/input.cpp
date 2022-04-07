#include "input.hpp"
#include "iostream"

namespace InputNS {

void Input::raw()
{
    // Set terminal to raw mode
    system("stty raw");
}

void Input::noraw()
{
    // Set terminal to cooked mode
    system("stty -raw");
}

void Input::waitKey()
{
    Input::raw();

    getchar();
    std::cout << "\b \b";

    Input::noraw();
}

InputType Input::getInput()
{
    Input::raw();

    InputType sel = inputpending;
    while(sel == inputpending)
    {
        auto c = getchar();
        switch(c)
        {
            case 'l': sel = right; break;
            case 'j': sel = left; break;
            case 'k': sel = down; break;
            case 'i': sel = up; break;
            case 'z': sel = InputType::quit; break;
            case 't': sel = throwing; break;
            case 'q': sel = drinking; break;
            case 'e': sel = eating; break;
            default:
                break;
        }
        std::cout << "\b \b";
    }

    Input::noraw();

    std::cout << std::endl;
    return sel;
}

InputAlkoType Input::getInputAlko()
{
    Input::raw();

    InputAlkoType sel = alkopending;
    while(sel == alkopending)
    {
        auto c = getchar();
        switch(c)
        {
            case '1': sel = InputAlkoType::ostakalja; break;
            case '2': sel = InputAlkoType::ostalonkka; break;
            default: sel = InputAlkoType::poistualko; break;
        }
        std::cout << "\b \b";
    }

    Input::noraw();
    return sel;
}

DrinkType Input::getDrink()
{
    Input::raw();

    DrinkType sel = drinkpending;
    while(sel == drinkpending)
    {
        auto c = getchar();
        switch(c)
        {
            case '1': sel = DrinkType::kalja; break;
            case '2': sel = DrinkType::lonkka; break;
            default: sel = DrinkType::nodrink; break;
        }
        std::cout << "\b \b";
    }

    InputNS::Input::noraw();
    return sel;
}

FoodType Input::getFood()
{
    Input::raw();

    FoodType sel = foodpending;
    while(sel == foodpending)
    {
        auto c = getchar();
        switch(c)
        {
            case '1': sel = FoodType::foodlenkki; break;
            case '2': sel = FoodType::foodkala; break;
            case '3': sel = FoodType::foodomppo; break;
            case '4': sel = FoodType::foodbansku; break;
            default: sel = FoodType::nofood; break;
        }
        std::cout << "\b \b";
    }

    InputNS::Input::noraw();
    return sel;
}

}