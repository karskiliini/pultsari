
#include "building.hpp"
#include "player.hpp"
#include "input.hpp"
#include <string>
#include <iostream>

using std::string;
using InputNS::Input;

Building::Building(BuildingType buildingType) : type(buildingType)
{
}

char Building::typeToChar(uint32_t x, uint32_t y) const { return ' '; };
std::string Building::getName() const { return ""; }

bool Building::getEnterMsg(PlayerNS::Player* player, std::string& msg) const { return false; }
bool Building::getInteractMsg(PlayerNS::Player* player, std::string& msg) const { return false; }
void Building::interact(PlayerNS::Player* player, std::string& msg) { msg = "error"; }

std::string Building::getWalkMsg() const { return ""; }

bool Building::hitTopWall(uint32_t x, uint32_t y) const {
    return ((x >= wallLeft) && (x <= wallRight) && (y == wallTop));
}

bool Building::hitBotWall(uint32_t x, uint32_t y) const {
    return ((x >= wallLeft) && (x <= wallRight) && (y == wallBot));
}

bool Building::hitLeftWall(uint32_t x, uint32_t y) const {
    return ((x == wallLeft) && (y >= wallTop) && (y <= wallBot));
}

bool Building::hitRightWall(uint32_t x, uint32_t y) const {
    return ((x == wallRight) && (y >= wallTop) && (y <= wallBot));
}

bool Building::hitWall(uint32_t x, uint32_t y) const {
    if (hitTopWall(x, y) ||
        hitRightWall(x, y) ||
        hitBotWall(x, y) ||
        hitLeftWall(x, y))
    {
        return true;
    }
    return false;
}

bool Building::hitDoor(uint32_t x, uint32_t y) const {
    if (hitWall(x, y))
    {
        if ((x >= wallLeft) && (x <= wallRight) && ((y == wallTop) || (y == wallBot))) {
            if (door == DirectionNS::Direction::up)
            {
                // door top
                const uint32_t mid = ((wallRight + wallLeft) / 2);
                if ((x >= mid-1) && (x <= mid+1) && (y == wallTop)) return true;
            } else if (door == DirectionNS::Direction::down)
            {
                // door bot
                const uint32_t mid = ((wallRight + wallLeft) / 2);
                if ((x >= mid-1) && (x <= mid+1) && (y == wallBot)) return true;
            }
        }
        else if (((x == wallLeft) || (x == wallRight)) && (y >= wallTop) && (y <= wallBot)) {
            if ((door == DirectionNS::Direction::left) || (door == DirectionNS::Direction::right))
            {
                if (door == DirectionNS::Direction::left)
                {
                    // special case
                    const uint32_t mid = ((wallTop + wallBot) / 2);
                    if ((y >= mid-1) && (y <= mid+1) && (x == wallLeft)) return true;
                }
                else if (door == DirectionNS::Direction::right)
                {
                    // special case
                    const uint32_t mid = ((wallTop + wallBot) / 2);
                    if ((y >= mid-1) && (y <= mid+1) && (x == wallRight)) return true;
                }

            }
        }
    }
    return false;
}

char Building::printChar(uint32_t x, uint32_t y) const
{
    if (hitDoor(x, y)) return '#';
    if (hitWall(x, y)) return '*';

    if (y == (wallTop + wallBot) / 2)
    {
        const auto name = getName();
        const auto len = name.length();
        const uint32_t mid = ((wallRight + wallLeft) / 2);
        const uint32_t left = mid - len/2;

        if ((x >= left) && (x < (left + len))) {
            const auto index = x - left;
            return name[index];
        }
    }
    return ' ';
}

Alko::Alko() : Building(BuildingType::EAlko) {
    wallRight = 20;
    wallTop = 20;
    wallLeft = 0;
    wallBot = 27;

    door = DirectionNS::up;
}

std::string Alko::getName() const
{
    return "ALKO";
}

char Alko::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool Alko::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (player->promilles > 14) {
        msg = "Vartija heitti sinut pihalle. Olet juopunut.";
        return false;
    } else if ((player->promilles < 5) &&  (player->money >= 15)) {
        msg = "Tehan olette selvä. Meillä sen voi korjata.";
        return true;
    } else if (player->money < 15) {
        msg = "Putiikki on kiinni.";
        return false;
    }
    msg = "";
    return true;
}

bool Alko::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Ja mitas herra ostaa ,kalja vai lonkka (1/2)?";
    return true;
}

void Alko::interact(PlayerNS::Player* player, std::string& msg)
{
    auto purchase = Input::getInputAlko();
    switch(purchase)
    {
        case InputNS::ostakalja:
            msg = "Kiitos,ja hyvää päivän jatkoa. (kalja)";
            ++player->inventory.kalja;
            player->money -= 10;
            break;
        case InputNS::ostalonkka:
            ++player->inventory.lonkka;
            player->money -= 15;
            msg = "Kiitos,ja hyvää päivän jatkoa. (lonkka)";
            break;
        default:
        case InputNS::poistualko:
            msg = "Kiitos,ja hyvää päivän jatkoa. (ulos)";
            break;
    }
}

std::string Alko::getWalkMsg() const { return "Isket paasi alkon seinaan. Mielenkiintoista."; }

Divari::Divari() : Building(BuildingType::EDivari) {
    wallRight = 20;
    wallTop = 0;
    wallLeft = 0;
    wallBot = 8;

    door = DirectionNS::down;
}

std::string Divari::getName() const
{
    return "DIVARI";
}

char Divari::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool Divari::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "divari on suljettu";
    return false;
}

std::string Divari::getWalkMsg() const { return "Divarin seinä on vankkumaton,vaikka kuinka yrität kävellä sitä päin."; }