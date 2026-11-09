
#include "building.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "input.hpp"
#include "common.hpp"
#include "level.hpp"
#include <string>
#include <iostream>
#include <exception>
#include <vector>

using std::string;
using std::cout;
using std::endl;
using InputNS::Input;
using common::random;

Building::Building(BuildingType buildingType) : type(buildingType)
{ }

void Building::npcAct()
{ }

std::string Building::typeToChar(const Coord& c) const
{
    uint32_t x = c.x;
    uint32_t y = c.y;

    if (isBreach(c)) return "\xE2\x96\x9E"; // smashed wall, glass and rubble
    if (hasDoor && hitDoor(c)) return "░";
    if (hitWindow(c)) return "▒";
    if (hitWall(c)) return "█";

    if (y == (wallTop + wallBot) / 2)
    {
        // split into UTF-8 characters so Cyrillic names render whole
        const auto name = getName();
        std::vector<string> chars;
        for (size_t i = 0; i < name.size();) {
            size_t n = 1;
            const unsigned char b = name[i];
            if (b >= 0xF0) n = 4; else if (b >= 0xE0) n = 3; else if (b >= 0xC0) n = 2;
            chars.push_back(name.substr(i, n));
            i += n;
        }
        const uint32_t len = chars.size();
        const uint32_t mid = ((wallRight + wallLeft) / 2);
        const uint32_t left = mid - len/2;

        if ((x >= left) && (x < (left + len))) {
            return chars[x - left];
        }
    }
    return " ";
}

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

bool Building::isBreach(const Coord& c) const
{
    for (const auto& b : breaches) if (b == c) return true;
    return false;
}

bool Building::hitWall(const Coord& c) const {
    if (isBreach(c)) return false;
    if (hitTopWall(c)   ||
        hitRightWall(c) ||
        hitBotWall(c)   ||
        hitLeftWall(c))
    {
        return true;
    }
    return false;
}

bool Building::hitWindow(const Coord& c) const {
    for (const auto& w : windows) {
        if (w == c) return true;
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

void Building::setOffset(int32_t dx, int32_t dy)
{
    wallLeft = (uint32_t)((int32_t)wallLeft + dx);
    wallRight = (uint32_t)((int32_t)wallRight + dx);
    wallTop = (uint32_t)((int32_t)wallTop + dy);
    wallBot = (uint32_t)((int32_t)wallBot + dy);
    for (auto& w : windows) {
        w.x = (uint32_t)((int32_t)w.x + dx);
        w.y = (uint32_t)((int32_t)w.y + dy);
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


// a customer standing next to the player in the queue loses patience
static void queueRage(Building* b, PlayerNS::Player* player, std::string& msg, int chance)
{
    Person* angry = nullptr;
    for (auto p : b->level->persons) {
        if (p == player || p->stats.health == 0 || !b->hitBuilding(p->coord)) continue;
        if (p->coord.distance(player->coord) <= 2) { angry = p; break; }
    }
    if (!angry || !common::random(chance)) return;
    static const char* rage[] = {
        "KIIREHDI! Vodkani odottaa!",
        "Liiku jo, svinya!",
        "Min\xC3\xA4 olin ensin!",
        "Jono, ty\xC3\xB6l\xC3\xA4inen! JONO!",
        "Sinun takiasi t\xC3\xA4\xC3\xA4ll\xC3\xA4 haisee!",
        "Mit\xC3\xA4 tuijotat? Haluatko turpaan?",
    };
    angry->say(rage[std::rand() % 6]);
    msg += "\n";
    msg += b->level->nameOf(angry) + (common::random(50) ? " ly\xC3\xB6 sinua takaraivoon!" : " t\xC3\xB6n\xC3\xA4isee sinua!");
    player->damage(std::rand() % 2 + 1);
}

// ALKO
Alko::Alko() : Building(BuildingType::EAlko) {
    walkable = true;
    wallRight = 20;
    wallTop = 0;
    wallLeft = 0;
    wallBot = 4;

    door = DirectionNS::down;
    windows.push_back({wallRight, 2});
    windows.push_back({wallLeft, 2});
}

std::string Alko::getName() const
{
    return "VINO-VODKA";
}

bool Alko::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    // anyone gets in; the counter is where you get turned away
    msg = "Ovikello kilahtaa. Hyllyt notkuvat vodkaa, jonossa seisoo puoli kortteli\xC3\xA4.";
    return true;
}

static bool alkoRefuses(PlayerNS::Player* player, std::string& msg)
{
    if (player->stats.promilles > 14) {
        msg = "Myyj\xC3\xA4: '\xD0\x9F\xD0\xBE\xD1\x88\xD1\x91\xD0\xBB \xD0\xB2\xD0\xBE\xD0\xBD!' Olet liian k\xC3\xA4nniss\xC3\xA4 edes Moskovan mittapuulla.";
        return true;
    }
    if (player->stats.money < 10) {
        msg = "Myyj\xC3\xA4: 'Ei ruplia, ei vodkaa. Katsele vaan, katsominen on ilmaista.'";
        return true;
    }
    return false;
}

bool Alko::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (alkoRefuses(player, msg)) return true;
    msg = player->stats.promilles < 5 ? "Myyj\xC3\xA4: 'Selv\xC3\xA4 mies? T\xC3\xA4\xC3\xA4ll\xC3\xA4? Se korjataan. Baltika vai Jaguar (1/2)?'"
                                      : "Myyj\xC3\xA4: 'Mit\xC3\xA4 otetaan? Baltika vai Jaguar (1/2)?'";
    return true;
}

void Alko::interact(PlayerNS::Player* player, std::string& msg)
{
    std::string refusal;
    if (alkoRefuses(player, refusal)) { msg.clear(); return; }
    auto purchase = Input::getInputAlko();
    switch(purchase)
    {
        case InputNS::ostakalja:
            msg = "Myyjä: 'Спасибо. Seuraava!'";
            ++player->inventory.kalja;
            player->updateMoney(-10);
            break;
        case InputNS::ostalonkka:
            if (player->stats.money < 15) { msg = "Myyj\xC3\xA4: 'Jaguar maksaa viisitoista. Ota Baltika.'"; break; }
            ++player->inventory.lonkka;
            player->updateMoney(-15);
            msg = "Myyjä: 'Jaguar on kielletty. Se maksaa siksi vähän enemmän.'";
            break;
        default:
        case InputNS::poistualko:
            msg = "Myyjä: 'Hyvää päivänjatkoa. Tai mitä niitä nyt on.'";
            break;
    }
    queueRage(this, player, msg, 35);
}

std::string Alko::getWalkMsg() const { return "Isket pääsi viinakaupan seinään. Jonottajat nauravat."; }

// ISKU
Isku::Isku() : Building(BuildingType::EIsku) {
    wallRight = 20;
    wallTop = 23;
    wallLeft = 0;
    wallBot = 27;

    door = DirectionNS::up;
    windows.push_back({wallRight, 25});
    windows.push_back({wallLeft, 25});
}

std::string Isku::getName() const
{
    return "IKEA";
}

bool Isku::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "IKEA on ollut kiinni vuodesta 2022. Ovessa lukee: 'Мы вернёмся.'";
    return true;
}

bool Isku::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    return false;
}
void Isku::interact(PlayerNS::Player* player, std::string& msg) { }
std::string Isku::getWalkMsg() const { return "Ikkunoissa on yhä alennuslappuja vuodelta 2022."; }

// DIVARI
Divari::Divari() : Building(BuildingType::EDivari) {
    walkable = true;
    wallRight = 20;
    wallTop = 23;
    wallLeft = 0;
    wallBot = 27;

    door = DirectionNS::up;
    windows.push_back({wallRight, 25});
    windows.push_back({wallLeft, 25});
}

std::string Divari::getName() const
{
    return "KOMISSIONKA";
}

bool Divari::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (player->stats.promilles > 15) {
        msg = "Panttilainaaja: 'Olet sikakännissä. Ulos täältä!'";
        return false;
    }
    else if ((player->inventory.bootsit == 0) &&
             (player->inventory.pamput == 0) &&
             (player->inventory.ketjut == 0) &&
             (player->inventory.veitset == 0)) {
           msg = "Panttilainaaja: 'Sinulla ei ole mitään, mitä ostaisin. Пока.'";
           return false;
    }
    return true;
}

bool Divari::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Mitä myyt? Verkkarit=1  Pamppu=2  Ketjut=3  Veitsi=4";
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
                    msg = "Sinulla ei ole verkkareita. Housutkin ovat lainassa.";
                } else {
                    --player->inventory.bootsit;
                    player->updateMoney(100);
                    msg = "Adidas-verkkarit menivät satasella. Kolme raitaa, kolme kertaa hinta.";
                }
                break;
            case 2:
                if (player->inventory.pamput < 1) {
                    msg = "Joo, montahan noita pamppuja sinulla onkin.";
                } else {
                    --player->inventory.pamput;
                    uint32_t price = std::rand() % 40 + 1;
                    player->updateMoney(price);
                    msg = "Pamppu meni " + std::to_string(price) + " ruplalla. Ei kysytty, kenen se oli.";
                }
                break;
            case 3:
                if (player->inventory.ketjut < 1) {
                    msg = "Sinulla ei ole yhtikäs yhtään ketjun pätkääkään.";
                } else {
                    --player->inventory.ketjut;
                    player->updateMoney(30);
                    msg = "Myit ketjut kolmellakympillä.";
                }
                break;
            case 4:
                if (player->inventory.veitset < 1) {
                    msg = "Et omista veitsiä.";
                } else {
                    --player->inventory.veitset;
                    uint32_t price = std::rand() % 50 + 1;
                    player->updateMoney(price);
                    msg = "Veitsestä sai " + std::to_string(price) + " ruplaa.";
                }
                break;
            default:
                retry = true;
                break;
        }
    }
}

std::string Divari::getWalkMsg() const { return "Panttilainaamon seinä ei anna periksi, vaikka kuinka yrität kävellä sitä päin."; }


// KRauta
KRauta::KRauta() : Building(BuildingType::EKRauta) {
    wallRight = 20;
    wallTop = 0;
    wallLeft = 0;
    wallBot = 4;

    door = DirectionNS::down;
    windows.push_back({wallRight, 2});
    windows.push_back({wallLeft, 2});
}

std::string KRauta::getName() const
{
    return "STROIMARKET";
}

bool KRauta::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Stroimarket on suljettu: 'Remontissa sanktioiden takia.' Varaosia odotetaan Kiinasta.";
    return false;
}

bool KRauta::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    return false;
}

void KRauta::interact(PlayerNS::Player* player, std::string& msg) { }
std::string KRauta::getWalkMsg() const { return "Rautakaupan seinä on vahvempaa tekoa kuin pääsi. Et päässyt sisään."; }


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
    return "SIZO";
}

bool Vankila::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Jos haluat tutkintavankilaan, mene politsijan ovesta.";
    return false;
}

bool Vankila::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    return false;
}

void Vankila::interact(PlayerNS::Player* player, std::string& msg)
{
}

std::string Vankila::getWalkMsg() const { return "Isket pääsi SIZOn seinään. Sisältä kuuluu koputus takaisin."; }


// POLIISIASEMA
PoliisiAsema::PoliisiAsema() : Building(BuildingType::EPoliisiAsema) {
    wallRight = 46;
    wallTop = 23;
    wallLeft = 34;
    wallBot = 27;

    door = DirectionNS::up;
    windows.push_back({wallLeft, 25});
    windows.push_back({wallRight, 25});
}

std::string PoliisiAsema::getName() const
{
    return "POLITSIJA";
}

bool PoliisiAsema::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Päivystäjä: 'Juoppoja otetaan aina. Tuletko (K/E)?'";
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
        msg = "Päivystäjä: 'No häivy sitten! Пошёл!'";
    }
}

std::string PoliisiAsema::getWalkMsg() const { return "Isket pääsi politsija-aseman seinään. Joku kirjaa sen ylös."; }


// ASEMA
Asema::Asema() : Building(BuildingType::EAsema) {
    wallRight = 76;
    wallTop = 25;
    wallLeft = 68;
    wallBot = 27;

    door = DirectionNS::left;
    windows.push_back({72, wallTop});
}

std::string Asema::getName() const
{
    return "VOKZAL";
}


void Asema::npcAct()
{
    if (!emitted) {
        ++turn;
        if (turn >= 30) {
            level->createPerson(getSpawn(), skinhead);
            emitted = true;
        }
    }
}

bool Asema::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Vokzalilla saa turpaansa. Ja junalipun rintamalle.";
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

std::string Asema::getWalkMsg() const { return "Isket pääsi rautatieaseman seinään. Mielenkiintoista."; }


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

std::string Seina::getWalkMsg() const { return "Betonimuuri. Joku on maalannut siihen Z-kirjaimen."; }


// KREMLIN
Kremlin::Kremlin() : Building(BuildingType::EKremlin) {
    lit = true; // crystal chandeliers in every hall
    walkable = true;
    wallRight = 45;
    wallTop = 0;
    wallLeft = 0;
    wallBot = 20;

    door = DirectionNS::down;
    windows.push_back({wallLeft, 5});
    windows.push_back({wallRight, 5});
    windows.push_back({wallLeft, 10});
    windows.push_back({wallRight, 10});
    windows.push_back({wallLeft, 15});
    windows.push_back({wallRight, 15});
    windows.push_back({10, wallTop});
    windows.push_back({20, wallTop});
    windows.push_back({30, wallTop});
    windows.push_back({40, wallTop});
}

std::string Kremlin::getName() const
{
    return "KREML";
}

bool Kremlin::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (!player->hasKremlinPass) {
        msg = "Vartija: STOI! Propusk! Tarvitset kulkuluvan!";
        return false;
    }
    msg = "Vartija nyökkää ja päästää sinut sisään Kremliin.";
    return true;
}

bool Kremlin::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (level && level->putinDead) {
        msg = "Kremlin on kaaoksessa. Henkivartijat juoksevat ympäriinsä.";
    } else {
        msg = "Valtavat salit kaikuvat askeleistasi. Kreml on sisältä koomisen suuri.";
    }
    return true;
}

void Kremlin::interact(PlayerNS::Player* player, std::string& msg)
{
    if (level && level->putinDead) {
        msg = "Kaaos! Pääse ulos Kremlistä!";
    } else {
        msg = "Kuljet loputtomien salien läpi. Katto häviää korkeuksiin.";
    }
}

std::string Kremlin::getWalkMsg() const { return "Kremlin muurit ovat läpäisemättömät."; }

// HYPERMARKET

Hypermarket::Hypermarket() : Building(BuildingType::EHypermarket) {
    walkable = true;
    wallRight = 25;
    wallTop = 0;
    wallLeft = 0;
    wallBot = 8;

    door = DirectionNS::down;
    windows.push_back({5, wallBot});
    windows.push_back({10, wallBot});
    windows.push_back({15, wallBot});
    windows.push_back({20, wallBot});
    windows.push_back({wallLeft, 4});
    windows.push_back({wallRight, 4});
}

std::string Hypermarket::getName() const
{
    return onFire ? "ГОРЯЩИЙ ГИПЕРМАРКЕТ" : "ГИПЕРМАРКЕТ";
}

std::string Hypermarket::typeToChar(const Coord& c) const
{
    for (const auto& f : fireSpots) {
        if (f == c) return "~";
    }
    for (const auto& s : smokeSpots) {
        if (s == c) return ";";
    }
    return Building::typeToChar(c);
}

// door guards still at their post (within a few steps of the door)
static int guardsAtDoor(const Building* b)
{
    if (!b->level) return 0;
    const Coord door = b->getDoor();
    int n = 0;
    for (auto p : b->level->persons) {
        const Vartija* v = dynamic_cast<const Vartija*>(p);
        if (v && !v->hasRadio && v->stats.health > 0 && v->coord.distance(door) <= 4) ++n;
    }
    return n;
}

bool Hypermarket::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (onFire) {
        msg = "Sisälle palavaan hypermarkettiin! Vaarallista!";
        return true;
    }
    if (guardsAtDoor(this) > 0) {
        msg = random(50) ? "Vartijat estävät tiesi. Houkuttele ne ensin muualle."
                         : "Vartija: 'Pummeille ei ole asiaa. Häivy.'";
        return false;
    }
    msg = "Ovella ei ole ketään! Livahdat sisään kassojen ohi.";
    return true;
}

bool Hypermarket::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = onFire ? "Savua ja liekkejä. Hyllyt romahtelevat."
                 : "Loisteputket surisevat. Hyllyt ovat täynnä. Kukaan ei katso.";
    return true;
}

void Hypermarket::interact(PlayerNS::Player* player, std::string& msg)
{
    if (onFire) {
        // inside the burning building - chance to find loot
        int roll = std::rand() % 100;
        if (roll < 20) {
            msg = "Löydät pivoa savuiselta hyllyltä!";
            player->inventory.kalja += 2;
        } else if (roll < 35) {
            msg = "Löydät doktorskaja-makkaraa!";
            player->inventory.lenkki += 1;
        } else if (roll < 50) {
            msg = "Löydät Jaguar-tölkkejä!";
            player->inventory.lonkka += 2;
        } else if (roll < 60) {
            msg = "Löydät vodkaa romahtaneelta hyllyltä!";
            player->inventory.vodka += 1;
        } else if (roll < 75) {
            msg = "Hylly kaatuu päällesi!";
            player->damage(3);
        } else {
            msg = "Savu peittää näkyvyyden. Et löydä mitään.";
        }
        return;
    }
    // grand tyr: grab what you can and run
    auto& inv = player->inventory;
    msg = "Kahmit hyllyistä:";
    const int grabs = 1 + std::rand() % 3;
    for (int i = 0; i < grabs; ++i) {
        switch (std::rand() % 6) {
        case 0: ++inv.vodka; msg += " vodkaa,"; break;
        case 1: inv.kalja += 2; msg += " kaksi Baltikaa,"; break;
        case 2: ++inv.lenkki; msg += " makkaraa,"; break;
        case 3: ++inv.kalat; msg += " voblaa,"; break;
        case 4: ++inv.lonkka; msg += " Jaguaria,"; break;
        default: ++inv.bansku; msg += " banaanin,"; break;
        }
    }
    msg.back() = '!';
    // the theft alarm beeps: guards who hear it come running
    msg += " Hälytin piippaa: VARAS!";
    if (level) {
        level->audio.addSound(getDoor(), SoundType::Siren, "Hypermarketin hälytin piippaa!", "Hälytin", "...piippausta...", 30);
        for (auto p : level->persons) {
            Vartija* v = dynamic_cast<Vartija*>(p);
            if (v && !v->hasRadio && v->stats.health > 0 && level->canHear(v->coord, getDoor(), 30, true)) v->provoke(player);
        }
    }
}

std::string Hypermarket::getWalkMsg() const { return "Hypermarketin seinät."; }

void Hypermarket::npcAct()
{
    if (!onFire) return;

    ++fireTurn;

    // fire spreads inside the building
    if (fireTurn % 3 == 0 && fireSpots.size() < 30) {
        if (!fireSpots.empty()) {
            auto& src = fireSpots[std::rand() % fireSpots.size()];
            Coord newFire = src;
            switch(std::rand() % 4) {
                case 0: if (newFire.y > wallTop + 1) --newFire.y; break;
                case 1: if (newFire.y < wallBot - 1) ++newFire.y; break;
                case 2: if (newFire.x > wallLeft + 1) --newFire.x; break;
                case 3: if (newFire.x < wallRight - 1) ++newFire.x; break;
            }
            fireSpots.push_back(newFire);
        }
    }

    // smoke drifts out from fire
    if (fireTurn % 5 == 0 && smokeSpots.size() < 20) {
        if (!fireSpots.empty()) {
            auto& src = fireSpots[std::rand() % fireSpots.size()];
            Coord smoke = src;
            // smoke drifts up and out
            for (int i = 0; i < 2 + std::rand()%3; ++i) {
                switch(std::rand() % 4) {
                    case 0: if (smoke.y > 0) --smoke.y; break;
                    case 1: ++smoke.y; break;
                    case 2: if (smoke.x > 0) --smoke.x; break;
                    case 3: ++smoke.x; break;
                }
            }
            smokeSpots.push_back(smoke);
        }
    }

    // old smoke dissipates
    if (smokeSpots.size() > 5 && common::random(20)) {
        smokeSpots.erase(smokeSpots.begin());
    }

    // damage player if standing in fire
    if (level) {
        auto p = level->getPlayer();
        if (p) {
            for (const auto& f : fireSpots) {
                if (f == p->coord) {
                    p->damage(2);
                    p->printer->showMessage("Tuli polttaa sinua!", level, false);
                    break;
                }
            }
            for (const auto& s : smokeSpots) {
                if (s == p->coord) {
                    p->damage(1);
                    p->printer->showMessage("Savu tukehduttaa sinua!", level, false);
                    break;
                }
            }
        }
    }
}

void Hypermarket::startFire()
{
    onFire = true;
    fireTurn = 0;
    // initial fire spots near center of building
    uint32_t cx = (wallLeft + wallRight) / 2;
    uint32_t cy = (wallTop + wallBot) / 2;
    fireSpots.push_back({cx, cy});
    fireSpots.push_back({cx + 1, cy});
    fireSpots.push_back({cx, cy + 1});
}

// RUSSIAN SHOPS: open, but nobody is happy about it

Kauppa::Kauppa(const std::string& name, Kind kind)
    : Building(BuildingType::EShop), name(name), kind(kind)
{
    walkable = true;
    wallLeft = 0;
    wallRight = 14;
    wallTop = 0;
    wallBot = 4;
    door = DirectionNS::down;
    windows.push_back({3, wallBot});
    windows.push_back({11, wallBot});
}

bool Kauppa::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    static const char* lines[] = {
        "Ovikello kilahtaa. Myyj\xC3\xA4 katsoo sinua kuin rottaa.",
        "Myyj\xC3\xA4 ei nosta katsettaan puhelimesta. 'Mit\xC3\xA4?'",
        "Myyj\xC3\xA4 huokaa syv\xC3\xA4\xC3\xA4n. 'Taas sin\xC3\xA4.'",
    };
    msg = lines[std::rand() % 3];
    return true;
}

bool Kauppa::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (kind == Kind::Produkty)
        msg = "MYYJ\xC3\x84: 1) Makkara 6\xE2\x82\xBD  2) Vobla 4\xE2\x82\xBD  3) Antonovka 2\xE2\x82\xBD  4) Baltika 10\xE2\x82\xBD  muu) ulos";
    else
        msg = "MYYJ\xC3\x84: 1) Baltika 10\xE2\x82\xBD  2) Banaani 5\xE2\x82\xBD  3) Jaguar 15\xE2\x82\xBD  4) Vodka 25\xE2\x82\xBD  muu) ulos";
    return true;
}

void Kauppa::interact(PlayerNS::Player* player, std::string& msg)
{
    const char c = InputNS::Input::getMenuKey();
    static const int foodPrice[] = { 6, 4, 2, 10 };
    static const int kioskPrice[] = { 10, 5, 15, 25 };
    if (c < '1' || c > '4') {
        static const char* bye[] = {
            "Myyj\xC3\xA4: 'Ei ostoksia? Ulos sitten. T\xC3\xA4m\xC3\xA4 ei ole museo.'",
            "Myyj\xC3\xA4: 'Ja \xC3\xA4l\xC3\xA4 tule takaisin.'",
            "Myyj\xC3\xA4 sylk\xC3\xA4isee lattialle per\xC3\xA4\xC3\xA4si.",
        };
        msg = bye[std::rand() % 3];
        queueRage(this, player, msg, 25);
        return;
    }
    const int k = c - '1';
    const int price = kind == Kind::Produkty ? foodPrice[k] : kioskPrice[k];
    if ((int)player->stats.money < price) {
        static const char* broke[] = {
            "Myyj\xC3\xA4: 'Ei rahaa? Ulos, bom\xC5\xBE""!'",
            "Myyj\xC3\xA4: 'Velaksi? Velaksi saat turpaan.'",
            "Myyj\xC3\xA4: 'Ruplat ensin. Sitten puhutaan.'",
        };
        msg = broke[std::rand() % 3];
        queueRage(this, player, msg, 40);
        return;
    }
    player->updateMoney(-price);
    auto& inv = player->inventory;
    if (kind == Kind::Produkty) {
        switch (k) {
        case 0: ++inv.lenkki; msg = "Makkarassa on 30 % lihaa. Loput on toivoa."; break;
        case 1: ++inv.kalat;  msg = "Vobla on kuivempi kuin myyj\xC3\xA4n huumorintaju."; break;
        case 2: ++inv.omppo;  msg = "Antonovka. Kotimainen. Ainoa asia, jota ei ole pakotteissa."; break;
        default: ++inv.kalja; msg = "Myyj\xC3\xA4 l\xC3\xA4im\xC3\xA4ytt\xC3\xA4\xC3\xA4 Baltikan tiskille. 'Seuraava!'"; break;
        }
    } else {
        switch (k) {
        case 0: ++inv.kalja;  msg = "Myyj\xC3\xA4: 'Baltika. L\xC3\xA4mmin. Ota tai j\xC3\xA4t\xC3\xA4.'"; break;
        case 1: ++inv.bansku; msg = "Banaani on Ecuadorista. Se on n\xC3\xA4hnyt enemm\xC3\xA4n maailmaa kuin sin\xC3\xA4."; break;
        case 2: ++inv.lonkka; msg = "Myyj\xC3\xA4: 'Jaguar on kielletty. Siksi se maksaa enemm\xC3\xA4n.'"; break;
        default: ++inv.vodka; msg = "Vodka tiskin alta. Etiketti on liimattu vinoon."; break;
        }
    }
    queueRage(this, player, msg, 35);
}

std::string Kauppa::getWalkMsg() const
{
    return name + ": ikkunassa lukee 'EI VAIHTORAHAA. EI VELKAA. EI PALAUTUKSIA.'";
}

// CLOSED WESTERN SHOPS

SuljettuLiike::SuljettuLiike(const std::string& name, const std::string& sign)
    : Building(BuildingType::EClosedShop), name(name), sign(sign)
{
    wallLeft = 0;
    wallRight = 14;
    wallTop = 0;
    wallBot = 4;
    // the front door is chained shut; business goes on through the back door
    walkable = true;
    door = DirectionNS::down;
    windows.push_back({3, wallTop});
    windows.push_back({11, wallTop});
}

bool SuljettuLiike::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Takaovi raottuu. Kalju p\xC3\xA4\xC3\xA4 kurkistaa: 'Kuka l\xC3\xA4hetti? ...No tule, bystro.'";
    return true;
}

bool SuljettuLiike::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "TAKAHUONE: 1) Vodka 20\xE2\x82\xBD  2) Samogon 8\xE2\x82\xBD  3) Pussi 'suolaa' 15\xE2\x82\xBD  muu) poistu";
    return true;
}

void SuljettuLiike::interact(PlayerNS::Player* player, std::string& msg)
{
    const char c = InputNS::Input::getMenuKey();
    const int price = c == '1' ? 20 : c == '2' ? 8 : c == '3' ? 15 : 0;
    if (price == 0) {
        msg = "Myyj\xC3\xA4: 'Et n\xC3\xA4hnyt mit\xC3\xA4\xC3\xA4n. T\xC3\xA4\xC3\xA4ll\xC3\xA4 on ollut kiinni vuodesta 2022.'";
        return;
    }
    if ((int)player->stats.money < price) {
        msg = "Myyj\xC3\xA4: 'Ei rahaa, ei kamaa. T\xC3\xA4m\xC3\xA4 ei ole sosiaalitoimisto.'";
        return;
    }
    player->updateMoney(-price);
    switch (c) {
    case '1':
        ++player->inventory.vodka;
        msg = "Pullossa lukee 'Absolut'. Kirjaimet ovat v\xC3\xA4\xC3\xA4rin p\xC3\xA4in. Sis\xC3\xA4lt\xC3\xB6 on aitoa.";
        break;
    case '2':
        if (common::random(25)) {
            player->updatepromilles(15);
            player->damage(2);
            msg = "Samogon polttaa kurkkua kuin jet-polttoaine. N\xC3\xA4k\xC3\xB6kentt\xC3\xA4 kapenee hetkeksi.";
        } else {
            ++player->inventory.vodka;
            msg = "Samogon kolmen litran kurkkupurkissa. Mummon resepti, sanoo myyj\xC3\xA4.";
        }
        break;
    default:
        if (common::random(40)) {
            player->updatepromilles(8);
            player->damage(1);
            msg = "Pussissa oli jotain muuta kuin suolaa. Sein\xC3\xA4t hengitt\xC3\xA4v\xC3\xA4t.";
        } else {
            player->powerup(3);
            msg = "Syd\xC3\xA4n hakkaa kuin konekivri. Olet yht\xC3\xA4kki\xC3\xA4 todella tehokas.";
        }
        break;
    }
}

std::string SuljettuLiike::getWalkMsg() const
{
    if (smashed) return name + ": ikkuna on rikki. H\xC3\xA4lytin piippaa yh\xC3\xA4.";
    return "Etuovi on lukossa. Lapussa: 'Suljettu toistaiseksi. 03/2022.' " + sign +
           " Takapihalta kuuluu musiikkia.";
}

void SuljettuLiike::smashWindow(const Coord& c, Person* source)
{
    breaches.push_back(c);
    smashed = true;
    burglar = source;
    alarmTicks = 40;
    level->audio.addSound(c, SoundType::Glass, "KILIN\xC3\x84! " + name + ":n ikkuna hajoaa!",
                          "Lasin s\xC3\xA4rkymist\xC3\xA4", "...lasin helin\xC3\xA4\xC3\xA4...", 30);
    level->audio.addSound(c, SoundType::Siren, name + ":n h\xC3\xA4lytin ulvoo!",
                          "Murtoh\xC3\xA4lytin", "...h\xC3\xA4lytin ulvoo jossain...", 40);
    if (source) level->alertCops(source, 40);

    // usually there is nothing inside but dust
    if (!common::random(30)) return;
    static const uint32_t loot[] = { 1, 2, 13, 3, 5, 14 };
    const uint32_t idx = loot[std::rand() % 6];
    for (int tries = 0; tries < 20; ++tries) {
        const Coord at { wallLeft + 1 + std::rand() % (wallRight - wallLeft - 1),
                         wallTop + 1 + std::rand() % (wallBot - wallTop - 1) };
        if (level->getItem(at) || level->hitPerson(at)) continue;
        if (Item* it = Item::createItem(idx, at)) level->addItem(it);
        break;
    }
}

void SuljettuLiike::npcAct()
{
    if (alarmTicks == 0) return;
    --alarmTicks;
    const Coord front { (wallLeft + wallRight) / 2, wallTop };
    if (alarmTicks % 6 == 0)
        level->audio.addSound(front, SoundType::Siren, name + ":n h\xC3\xA4lytin ulvoo!",
                              "Murtoh\xC3\xA4lytin", "...h\xC3\xA4lytin ulvoo jossain...", 40);

    // the patrol turns up a while after the alarm, if the burglar is still around
    if (alarmTicks == 22 && level->isAlive(burglar) && burglar->coord.distance(front) < 25) {
        for (int k = 0; k < 2; ++k) {
            for (int tries = 0; tries < 30; ++tries) {
                const int dx = (std::rand() % 2 ? 1 : -1) * (14 + std::rand() % 6);
                const int dy = (int)(std::rand() % 9) - 4;
                const int x = (int)burglar->coord.x + dx, y = (int)burglar->coord.y + dy;
                if (x < 1 || y < 1 || x >= (int)common::SIZEX - 1 || y >= (int)common::SIZEY - 1) continue;
                const Coord at { (uint32_t)x, (uint32_t)y };
                if (level->hit(at)) continue;
                Person* cop = level->createPerson(at, poliisi);
                if (!cop) break;
                cop->chaseTarget = burglar;
                cop->alerted = true;
                cop->lastKnownPos = burglar->coord;
                cop->alertCooldown = 40;
                cop->say(k ? "Murtoh\xC3\xA4lytys! Seis!" : "Tuolla se on!");
                break;
            }
        }
    }
}

// OIL REFINERY

Jalostamo::Jalostamo() : Building(BuildingType::ERefinery)
{
    wallLeft = 0;
    wallRight = 40;
    wallTop = 0;
    wallBot = 12;
    door = DirectionNS::up;
    windows.push_back({10, wallTop});
    windows.push_back({30, wallTop});
}

bool Jalostamo::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = burning ? "Portti on auki, mutta sis\xC3\xA4ll\xC3\xA4 kaikki palaa. Et mene."
                  : "Portilla vartija: 'Strateginen kohde! Kuvaaminen kielletty! Drooneja ei ole!'";
    return false;
}

std::string Jalostamo::getWalkMsg() const
{
    return burning ? "Sein\xC3\xA4 on tulikuuma. \xC3\x96ljy palaa sis\xC3\xA4ll\xC3\xA4."
                   : "S\xC3\xA4ili\xC3\xB6iss\xC3\xA4 lukee: \xD0\x9D\xD0\xB5 \xD0\xBA\xD1\x83\xD1\x80\xD0\xB8\xD1\x82\xD1\x8C!";
}

// BANK

Pankki::Pankki() : Building(BuildingType::EBank)
{
    walkable = true;
    wallLeft = 0;
    wallRight = 18;
    wallTop = 0;
    wallBot = 7;
    door = DirectionNS::down;
    windows.push_back({4, wallBot});
    windows.push_back({14, wallBot});
    windows.push_back({wallLeft, 3});
    windows.push_back({wallRight, 3});
}

bool Pankki::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Sberbankin ovi aukeaa. Vartija nukkuu tuolillaan.";
    return true;
}

bool Pankki::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Jonossa seisoo el\xC3\xA4kel\xC3\xA4isi\xC3\xA4. Kassalla lukee: 'Dollareita ei ole.'";
    return true;
}

std::string Pankki::getWalkMsg() const
{
    return "Sberbankin sein\xC3\xA4. Ikkunassa mainos: '12 % korko, takuulla!'";
}
