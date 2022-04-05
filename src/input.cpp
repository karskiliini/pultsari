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

InputType Input::getInput()
{

    InputNS::Input::raw();

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
            default:
                break;
        }
    }

    InputNS::Input::noraw();

    return sel;
}

DrinkType Input::getDrink()
{
    InputNS::Input::raw();

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
    }

    InputNS::Input::noraw();
    return sel;
}

}