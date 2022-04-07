#include "item.hpp"
#include "player.hpp"
#include "level.hpp"
#include <string>

using std::string;

// bona
Bona::Bona(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Bona::typeToChar() const { return "°"; }
std::string Bona::getMsg() const {
    return "Löysit bonan!";
}

// kalja
Kalja::Kalja(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Kalja::typeToChar() const { return "%"; }
std::string Kalja::getMsg() const {
    return "Katohhan, miehen mittainen.";
}
bool Kalja::interact(PlayerNS::Player* player)
{
    ++player->inventory.kalja;
    return true;
}

// lonkka
Lonkka::Lonkka(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Lonkka::typeToChar() const { return "%"; }
std::string Lonkka::getMsg() const {
    return "Kappas, lonkan mittainen !!!";
}
bool Lonkka::interact(PlayerNS::Player* player)
{
    ++player->inventory.lonkka;
    return true;
}

// lenkki
Lenkki::Lenkki(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Lenkki::typeToChar() const { return "("; }
std::string Lenkki::getMsg() const {
    return "Nam, lenkki makkaraa.";
}
bool Lenkki::interact(PlayerNS::Player* player)
{
    ++player->inventory.lenkki;
    return true;
}

// pamppu
Pamppu::Pamppu(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Pamppu::typeToChar() const { return "!"; }
std::string Pamppu::getMsg() const {
    return "Pamppu. Mahtaakohan tama olla kadoksissa joltain poltsulta ?";
}
bool Pamppu::interact(PlayerNS::Player* player)
{
    ++player->inventory.pamput;
    return true;
}

// veitset
Veitsi::Veitsi(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Veitsi::typeToChar() const { return "/"; }
std::string Veitsi::getMsg() const {
    return "Teroita veitsesi mummoihin.";
}
bool Veitsi::interact(PlayerNS::Player* player)
{
    ++player->inventory.veitset;
    return true;
}

// ketjut
Ketjut::Ketjut(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Ketjut::typeToChar() const { return "/"; }
std::string Ketjut::getMsg() const {
    return "Kutkuta pollaria riimuilla.";
}
bool Ketjut::interact(PlayerNS::Player* player)
{
    ++player->inventory.ketjut;
    return true;
}

// kivi
Kivi::Kivi(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Kivi::typeToChar() const { return "."; }
std::string Kivi::getMsg() const {
    return "Löysit kiven. Onnitteluni.";
}
bool Kivi::interact(PlayerNS::Player* player)
{
    ++player->inventory.kivet;
    return true;
}

// Bootsit
Bootsit::Bootsit(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Bootsit::typeToChar() const { return "B"; }
std::string Bootsit::getMsg() const {
    return "Camel boots,miehille,jotka kusevat omille jaloileen.";
}
bool Bootsit::interact(PlayerNS::Player* player)
{
    ++player->inventory.kivet;
    return true;
}

// Kala
Kala::Kala(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Kala::typeToChar() const { return "¥"; }
std::string Kala::getMsg() const {
    return "Kalasi lemuaa.";
}
bool Kala::interact(PlayerNS::Player* player)
{
    ++player->inventory.kalat;
    return true;
}

// omppo
Omppo::Omppo(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Omppo::typeToChar() const { return "¥"; }
std::string Omppo::getMsg() const {
    return "Omppo.Sen voi syoda, jos uskaltaa.";
}
bool Omppo::interact(PlayerNS::Player* player)
{
    ++player->inventory.omppo;
    return true;
}

// banaani
Banaani::Banaani(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Banaani::typeToChar() const { return ")"; }
std::string Banaani::getMsg() const {
    return "Loysit banaanin. Kokeileppas syoda se.";
}
bool Banaani::interact(PlayerNS::Player* player)
{
    ++player->inventory.bansku;
    return true;
}

// Raha
Raha::Raha(uint32_t x, uint32_t y) : Item(x, y) {
    value = rand() % 29 + 1;
}
std::string Raha::typeToChar() const { return "$"; }
std::string Raha::getMsg() const {
    string raha;
    {
        char str[10];
        sprintf(str, "%d", value);
        raha = string(str);
    }
    return "Kukkarosi lihavoituu " + raha + " markalla.";
}
bool Raha::interact(PlayerNS::Player* player)
{
    player->money += value;
    return true;
}

// paska
Paska::Paska(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Paska::typeToChar() const { return "p"; }
std::string Paska::getMsg() const {
    return "Huomaat sen liian myohaan... liukastut paskaan.";
}

bool Paska::interact(PlayerNS::Player* player)
{
    player->health -= 1;
    return true;
}
