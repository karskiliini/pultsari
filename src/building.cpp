
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

std::string Building::typeToChar(uint32_t x, uint32_t y) const { return " "; };
std::string Building::getName() const { return ""; }

bool Building::getEnterMsg(PlayerNS::Player* player, std::string& msg) const { return false; }
bool Building::getInteractMsg(PlayerNS::Player* player, std::string& msg) const { return false; }
void Building::interact(PlayerNS::Player* player, std::string& msg) { }

std::string Building::getWalkMsg() const { return ""; }

bool Building::hitBuilding(uint32_t x, uint32_t y) const
{
    return ((x >= wallLeft) && (x <= wallRight) && (y >= wallTop) && (y <= wallBot));
}

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
    if (!hasDoor) return false;

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

string Building::printChar(uint32_t x, uint32_t y) const
{
    if (hasDoor && hitDoor(x, y)) return "░";
    if (hitWall(x, y)) return "█";

    if (y == (wallTop + wallBot) / 2)
    {
        const auto name = getName();
        const auto len = name.length();
        const uint32_t mid = ((wallRight + wallLeft) / 2);
        const uint32_t left = mid - len/2;

        if ((x >= left) && (x < (left + len))) {
            const auto index = x - left;
            string ret = name.substr(index, 1);
            return ret;
        }
    }
    return " ";
}

// ALKO
Alko::Alko() : Building(BuildingType::EAlko) {
    wallRight = 20;
    wallTop = 0;
    wallLeft = 0;
    wallBot = 4;

    door = DirectionNS::down;
}

std::string Alko::getName() const
{
    return "ALKO";
}

string Alko::typeToChar(uint32_t x, uint32_t y) const
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
    msg = "Ja mitas herra ostaa, kalja vai lonkka (1/2)?";
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

// ISKU
Isku::Isku() : Building(BuildingType::EIsku) {
    wallRight = 20;
    wallTop = 23;
    wallLeft = 0;
    wallBot = 27;

    door = DirectionNS::up;
}

std::string Isku::getName() const
{
    return "ISKU";
}

string Isku::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool Isku::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Putiikki on kiinni.";
    return true;
}

bool Isku::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    return false;
}
void Isku::interact(PlayerNS::Player* player, std::string& msg) { }
std::string Isku::getWalkMsg() const { return "Iskun seina on iskunkestava,et paassyt sisaan."; }

// DIVARI
Divari::Divari() : Building(BuildingType::EDivari) {
    wallRight = 20;
    wallTop = 23;
    wallLeft = 0;
    wallBot = 27;

    door = DirectionNS::up;
}

std::string Divari::getName() const
{
    return "DIVARI";
}

string Divari::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool Divari::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "divari on suljettu";
    return false;
}

std::string Divari::getWalkMsg() const { return "Divarin seinä on vankkumaton,vaikka kuinka yrität kävellä sitä päin."; }


// KRauta
KRauta::KRauta() : Building(BuildingType::EKRauta) {
    wallRight = 20;
    wallTop = 0;
    wallLeft = 0;
    wallBot = 4;

    door = DirectionNS::down;
}

std::string KRauta::getName() const
{
    return "K-Rauta";
}

string KRauta::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool KRauta::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "K-RAUTAa ei ole ohjelmoitu loppuun.";
    return false;
}

bool KRauta::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Ja mitas herra ostaa ,kalja vai lonkka (1/2)?";
    return true;
}

void KRauta::interact(PlayerNS::Player* player, std::string& msg) { }
std::string KRauta::getWalkMsg() const { return "K-RAUDAN  seina on vahvempaa tekoa,kuin paasi.Et paassyt sisaan."; }


// VANKILA
Vankila::Vankila() : Building(BuildingType::EVankila) {
    wallRight = 58;
    wallTop = 23;
    wallLeft = 48;
    wallBot = 27;

    door = DirectionNS::up;
}

std::string Vankila::getName() const
{
    return "PUTKA";
}

string Vankila::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool Vankila::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Jos haluat putkaan,mene poliisiaseman ovesta.";
    return false;
}

bool Vankila::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    return false;
}

void Vankila::interact(PlayerNS::Player* player, std::string& msg)
{
}

std::string Vankila::getWalkMsg() const { return "Isket paasi Vankilan seinaan. Mielenkiintoista."; }


// POLIISIASEMA
PoliisiAsema::PoliisiAsema() : Building(BuildingType::EPoliisiAsema) {
    wallRight = 46;
    wallTop = 23;
    wallLeft = 34;
    wallBot = 27;

    door = DirectionNS::up;
}

std::string PoliisiAsema::getName() const
{
    return "POLIISI";
}

string PoliisiAsema::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool PoliisiAsema::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Putkaan otetaan kylla juoppoja, tuletko (K/E) ?";
    return true;
}

bool PoliisiAsema::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    return true;
}

void PoliisiAsema::interact(PlayerNS::Player* player, std::string& msg)
{
    auto enter = Input::getInputPutka();
    if (enter) {
        player->inJail = true;
        msg = "Putkassa istuskellessasi huomaat parin tunnin paasta miten          <LISää>";
    } else {
        msg = "No haivy sitten !!!";
    }
}

std::string PoliisiAsema::getWalkMsg() const { return "Isket paasi Vankilan seinaan. Mielenkiintoista."; }


// ASEMA
Asema::Asema() : Building(BuildingType::EAsema) {
    wallRight = 76;
    wallTop = 25;
    wallLeft = 68;
    wallBot = 27;

    door = DirectionNS::left;
}

std::string Asema::getName() const
{
    return "ASEMA";
}

string Asema::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

bool Asema::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Asemalla voi saada turpaansa!";
    return false;
}

bool Asema::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "";
    return false;
}

void Asema::interact(PlayerNS::Player* player, std::string& msg)
{
}

std::string Asema::getWalkMsg() const { return "Isket paasi aseman seinaan. Mielenkiintoista."; }


// ASEMAN SEINÄ
Seina::Seina() : Building(BuildingType::ESeina) {
    wallRight = 68;
    wallTop = 25;
    wallLeft = 62;
    wallBot = 25;

    door = DirectionNS::down;
    hasDoor = false;
}

std::string Seina::getName() const
{
    return "";
}

string Seina::typeToChar(uint32_t x, uint32_t y) const
{
    return printChar(x, y);
}

std::string Seina::getWalkMsg() const { return "Isket paasi aseman seinaan. Mielenkiintoista."; }
