
#include "building.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "input.hpp"
#include "common.hpp"
#include "level.hpp"
#include <string>
#include <iostream>
#include <exception>

using std::string;
using std::cout;
using std::endl;
using InputNS::Input;
using common::random;

Building::Building(BuildingType buildingType) : type(buildingType)
{
}

void Building::npcAct()
{
}

std::string Building::typeToChar(const Coord& c) const { return printChar(c); };
std::string Building::getName() const { return ""; }

bool Building::getEnterMsg(PlayerNS::Player* player, std::string& msg) const { return false; }
bool Building::getInteractMsg(PlayerNS::Player* player, std::string& msg) const { return false; }
void Building::interact(PlayerNS::Player* player, std::string& msg) { }

std::string Building::getWalkMsg() const { return ""; }

bool Building::hitBuilding(const Coord& c) const
{
    return ((c.x >= wallLeft) && (c.x <= wallRight) && (c.y >= wallTop) && (c.y <= wallBot));
}

bool Building::hitTopWall(const Coord& c) const {
    return ((c.x >= wallLeft) && (c.x <= wallRight) && (c.y == wallTop));
}

bool Building::hitBotWall(const Coord& c) const {
    return ((c.x >= wallLeft) && (c.x <= wallRight) && (c.y == wallBot));
}

bool Building::hitLeftWall(const Coord& c) const {
    return ((c.x == wallLeft) && (c.y >= wallTop) && (c.y <= wallBot));
}

bool Building::hitRightWall(const Coord& c) const {
    return ((c.x == wallRight) && (c.y >= wallTop) && (c.y <= wallBot));
}

bool Building::hitWall(const Coord& c) const {
    if (hitTopWall(c)   ||
        hitRightWall(c) ||
        hitBotWall(c)   ||
        hitLeftWall(c))
    {
        return true;
    }
    return false;
}

bool Building::hitDoor(const Coord& c) const {
    if (!hasDoor) return false;

    uint32_t x = c.x;
    uint32_t y = c.y;

    if (hitWall(c))
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

string Building::printChar(const Coord& c) const
{
    uint32_t x = c.x;
    uint32_t y = c.y;

    if (hasDoor && hitDoor(c)) return "░";
    if (hitWall(c)) return "█";

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

Coord Building::getDoor() const
{
    switch(door) {
        case DirectionNS::Direction::up:
        {
            Coord c { ((wallRight + wallLeft) / 2), wallTop };
                return Coord { ((wallRight + wallLeft) / 2), wallTop };
        }
        case DirectionNS::Direction::down:
                return Coord { ((wallRight + wallLeft) / 2), wallBot };
        case DirectionNS::Direction::right:
                return Coord { wallRight, (wallTop + wallBot) / 2 };
        case DirectionNS::Direction::left:
                return Coord { wallLeft, (wallTop + wallBot) / 2 };
        default:
            throw std::out_of_range("no door!");
    }
}

Coord Building::getSpawn() const
{
    Coord c = getDoor();

    switch(door) {
        case DirectionNS::Direction::up:
                c -= { 0, 1 };
                break;
        case DirectionNS::Direction::down:
                c += { 0, 1 };
                break;
        case DirectionNS::Direction::right:
                c += { 1, 0 };
                break;
        case DirectionNS::Direction::left:
                c -= { 1, 0 };
                break;
        default:
            throw std::out_of_range("no door!");
    }

    return c;
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

bool Divari::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (player->promilles > 15) {
        msg = "Tehän olette sikakännissä. Ulos täältä !";
        return false;
    }
    else if ((player->inventory.bootsit == 0) &&
             (player->inventory.pamput == 0) &&
             (player->inventory.ketjut == 0) &&
             (player->inventory.veitset == 0)) {
           msg = "Emme osta teilta mitaan. Out.";
           return false;
    }
    return true;
}

bool Divari::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Mitas myisit ? Bootsit=1  Pamppu=2  Ketjut=3  Veitsi=4";
    return true;
}

void Divari::interact(PlayerNS::Player* player, std::string& msg) {
    auto item = InputNS::Input::getInputDivari();

    bool retry = true;
    while (retry)
    {
        retry = false;

        switch(item) {
            case 1:
                if (player->inventory.bootsit < 1) {
                    msg = "Eihän teillä ole kenkia edes jaloissanne.";
                } else {
                    --player->inventory.bootsit;
                    player->money += 100;
                    msg = "Bootsit meni satasella.";
                }
                break;
            case 2:
                if (player->inventory.pamput < 1) {
                    msg = "Joo, montahan noita pamppuja sinulla onkin.";
                } else {
                    --player->inventory.pamput;
                    uint32_t price = rand() % 40 + 1;
                    player->money += price;
                    msg = "Pamppu hurahti " + std::to_string(price) + " markalla.";
                }
                break;
            case 3:
                if (player->inventory.ketjut < 1) {
                    msg = "Mutta, kun sinulla ei ole yhtikäs yhtään ketjun patkääkään.";
                } else {
                    --player->inventory.ketjut;
                    player->money += 30;
                    msg = "Ketjut onnistuit myymaan kolmella kympilla.";
                }
                break;
            case 4:
                if (player->inventory.veitset < 1) {
                    msg = "Et omista veitsiä.";
                } else {
                    --player->inventory.veitset;
                    uint32_t price = rand() % 50 + 1;
                    player->money += price;
                    msg = "Voitit veitellas " + std::to_string(price) + " markkaa.";
                }
                break;
            default:
                retry = true;
                break;
        }
    }
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

void Vankila::npcAct()
{
    if (!emitted)
    {
        ++turn;
        if (turn > 50) {
            if (!level->PersonExists(vanki)) {
                if (random(50)) {
                    level->createPerson(getSpawn(), vanki);
                    emitted = true;
                }
            }
        }
    }
}

std::string Vankila::getName() const
{
    return "PUTKA";
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

bool PoliisiAsema::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Putkaan otetaan kylla juoppoja, tuletko (K/E) ?";
    return true;
}

bool PoliisiAsema::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "";
    return true;
}

void PoliisiAsema::interact(PlayerNS::Player* player, std::string& msg)
{
    auto enter = Input::getInputPutka();
    if (enter) {
        player->inJail = true;
        msg = "";
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


void Asema::npcAct()
{
    ++turn;
    if (turn >= 30) {
        level->createPerson(getSpawn(), skinhead);
        turn = 0;
    }
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

std::string Seina::getWalkMsg() const { return "Isket paasi aseman seinaan. Mielenkiintoista."; }
