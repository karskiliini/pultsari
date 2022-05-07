#include "input.hpp"
#include "iostream"

#if defined(_WIN32)
#include <conio.h>
#include <unistd.h>
#endif

namespace InputNS {

char readChar() {
#if defined(_WIN32)
    return getch();
#else
    return getchar();
#endif
}

void Input::raw()
{
#if defined(_WIN32)
#elif defined(__linux__)
    // Set terminal to raw mode
    system("stty raw -echo");
#endif // Windows/Linux
}

void Input::noraw()
{
#if defined(_WIN32)
#elif defined(__linux__)
    // Set terminal to cooked mode
    system("stty -raw echo");
#endif // Windows/Linux
}

void Input::waitKey()
{
    Input::raw();
    readChar();
    Input::noraw();
}

InputType Input::getInput()
{
    Input::raw();

    InputType sel = inputpending;
    while(sel == inputpending)
    {
        auto c = readChar();
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
    }

    Input::noraw();
    return sel;
}

uint32_t InputNS::Input::getThrow()
{
     Input::raw();

    uint32_t sel = 0;
    while(sel == 0)
    {
        auto c = readChar();
        switch(c)
        {
            case '1': sel = 1; break;
            case '2': sel = 2; break;
            case '3': sel = 3; break;
            case '4': sel = 4; break;
            case '5': sel = 5; break;
            case '6': sel = 6; break;
            case '7': sel = 7; break;
            case '8': sel = 8; break;
            case '9': sel = 9; break;
            case 'A':
            case 'a': sel = 10; break;
            case 'B':
            case 'b': sel = 11; break;
            default:
                break;
        }
    }

    Input::noraw();
    return sel;
}

InputAlkoType Input::getInputAlko()
{
    Input::raw();

    InputAlkoType sel = alkopending;
    while(sel == alkopending)
    {
        auto c = readChar();
        switch(c)
        {
            case '1': sel = InputAlkoType::ostakalja; break;
            case '2': sel = InputAlkoType::ostalonkka; break;
            default: sel = InputAlkoType::poistualko; break;
        }
    }

    Input::noraw();
    return sel;
}

bool Input::getInputPutka()
{
    Input::raw();

    bool sel = false;
    bool done = false;
    while(!done)
    {
        auto c = readChar();
        switch(c)
        {
            case 'E':
            case 'e': sel = false; done = true; break;
            case 'K':
            case 'k': sel = true; done = true; break;
            default:  break;
        }
    }

    Input::noraw();
    return sel;
}

uint32_t Input::getInputDivari()
{
    Input::raw();

    uint32_t sel = 0;
    while (sel == 0)
    {
        auto c = readChar();
        switch(c)
        {
            case '1': sel = 1; break;
            case '2': sel = 2; break;
            case '3': sel = 3; break;
            case '4': sel = 4; break;
            default: sel = 0; break;
        }
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
        auto c = readChar();
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

FoodType Input::getFood()
{
    Input::raw();

    FoodType sel = foodpending;
    while(sel == foodpending)
    {
        auto c = readChar();
        switch(c)
        {
            case '1': sel = FoodType::foodlenkki; break;
            case '2': sel = FoodType::foodkala; break;
            case '3': sel = FoodType::foodomppo; break;
            case '4': sel = FoodType::foodbansku; break;
            default: sel = FoodType::nofood; break;
        }
    }

    InputNS::Input::noraw();
    return sel;
}

char Input::inputYka()
{
    Input::raw();

    char sel = '1';
    while(sel == '1')
    {
        auto c = readChar();
        switch(c)
        {
            case 'k':
            case 'K': sel = 'k'; break;
            case 'e':
            case 'E': sel = 'e'; break;
            default: sel = '1'; break;
        }
    }

    InputNS::Input::noraw();
    return sel;
}

}