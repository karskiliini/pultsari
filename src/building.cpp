
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


// ALKO
Alko::Alko() : Building(BuildingType::EAlko) {
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
    if (player->stats.promilles > 14) {
        msg = "Myyjä: 'Пошёл вон!' Olet liian kännissä edes Moskovan mittapuulla.";
        return false;
    } else if ((player->stats.promilles < 5) && (player->stats.money >= 15)) {
        msg = "Myyjä: 'Selvä mies? Täällä? Se korjataan.'";
        return true;
    } else if (player->stats.money < 15) {
        msg = "Myyjä: 'Ei ruplia, ei vodkaa.'";
        return false;
    }
    msg = "";
    return true;
}

bool Alko::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    msg = "Myyjä: 'Mitä otetaan? Baltika vai Jaguar (1/2)?'";
    return true;
}

void Alko::interact(PlayerNS::Player* player, std::string& msg)
{
    auto purchase = Input::getInputAlko();
    switch(purchase)
    {
        case InputNS::ostakalja:
            msg = "Myyjä: 'Спасибо. Seuraava!'";
            ++player->inventory.kalja;
            player->updateMoney(-10);
            break;
        case InputNS::ostalonkka:
            ++player->inventory.lonkka;
            player->updateMoney(-15);
            msg = "Myyjä: 'Jaguar on kielletty. Se maksaa siksi vähän enemmän.'";
            break;
        default:
        case InputNS::poistualko:
            msg = "Myyjä: 'Hyvää päivänjatkoa. Tai mitä niitä nyt on.'";
            break;
    }
    // queue rage: nearby NPCs get angry waiting for their vodka
    if (common::random(25)) {
        static const char* rage[] = {
            "Jonossa olija: 'KIIREHDI! Vodkani odottaa!'",
            "Joku jonossa: 'Liiku jo, svinya!'",
            "Jonottaja lyö sinua takaraivoon!",
            "Vihainen asiakas tönäisee sinua!",
        };
        msg += "\n";
        msg += rage[std::rand()%4];
        player->damage(std::rand()%2 + 1);
    }
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

std::string PoliisiAsema::getWalkMsg() const { return "Isket pääsi poliisiaseman seinään. Joku kirjaa sen ylös."; }


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

bool Hypermarket::getEnterMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (onFire) {
        msg = "Sisälle palavaan hypermarkettiin! Vaarallista!";
        return true;
    }
    msg = "Vartijat estävät tiesi.";
    return false;
}

bool Hypermarket::getInteractMsg(PlayerNS::Player* player, std::string& msg) const
{
    if (onFire) {
        // inside the burning building - chance to find loot
        int roll = std::rand() % 100;
        if (roll < 20) {
            msg = "Löydät kaljaa savuiselta hyllyltä!";
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
    } else {
        msg = "Hypermarketti on suljettu pummeilta.";
    }
    return true;
}

void Hypermarket::interact(PlayerNS::Player* player, std::string& msg)
{
    getInteractMsg(player, msg);
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
