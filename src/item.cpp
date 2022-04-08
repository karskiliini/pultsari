#include "item.hpp"
#include "player.hpp"
#include "level.hpp"
#include <string>
#include <vector>

using std::string;
using std::vector;

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
std::string Ketjut::typeToChar() const { return "¨"; }
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
    ++player->inventory.bootsit;
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
std::string Omppo::typeToChar() const { return "o"; }
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

Raha::Raha(uint32_t x, uint32_t y, uint32_t value)  : Item(x, y)
{
    Raha::value = value;
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

// taivaallinen viesti
#include  <random>
#include  <iterator>

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    return select_randomly(start, end, gen);
}

Viesti::Viesti(uint32_t x, uint32_t y) : Item(x, y) { }
std::string Viesti::typeToChar() const { return "?"; }
std::string Viesti::getMsg() const {
    const vector<string> viestit = {
        "Mummoilla on rahaa.",
        "Poliisit on kayny punttisalilla.",
        "Alcossa myydaan tavaraa.",
        "Varkaille kannattaa antaa pamppua.",
        "Sven Olof on niita miehia .",
        "Vankikarkuri kupsahtaa vasta kolmannella pampulla",
        "PULTSARI   -  Written By Mika Maaranen                          28.12.1991",
        "Kalaa kannatta syoda harkiten.",
        "Sinulla on 50 % mahdollisuus,siihen,etta omena on herkullinen",
        "Poliisien kanssa ei ole leikkimista. ",
        "Mummot saattavat huitaista kasilaukulla.",
        "Tassakin pelissa alco ottaa ylihintaa.",
        "Varkaat voi nuijia vain heittamalla.",
        "Vauhtia siihen peliin !!",
        "Mitas oikein kuppaat ? Olet hakannut vasta pari mummoa !",
        "Mummelit vaistelevat heittoja.",
        "Skinhead on saattanut sekoontua elaimiin...",
        "Kankkusta kannatta valtella. Tai ainakin tassa pelissa.",
        "K-RAUTAa ei ole ohjelmoitu loppuun.",
        "Tavatessasi kaljapullon etikettia , tavaat: L-A-H-D-E-N IIIIIIIIIIIIIIII",
        "Varo paskaa.Siihen liukastuu.",
        "Oispa tassa pelissa vessa.",
        "Alco ei myy humaltuneille.",
        "Divari ei osta juopuneilta.",
        "PULTSARI:  -  Idea By Mika -,Jari -,Tero Maaranen and Kimmo Korhonen.",
        "Mummot on vaarallisia.",
        "AIDS tappaa.",
        "Jos otat,et aja.",
        "Poliisi asemalla on putka,kokeile sita(vasta viidennen levelin jalkeen)",
        "Burb !!",
        "Tama ohjelma on laadittu kayttamalla Qbasic 4.5 - ohjelmaa.",
        "Vankikarkurit ne heittelleepi ilkeesti kivilla - jos ne loytaa sellasen.",
        "By Miksu Soft. Eikun pulsusoftware 5.4.2022.",
        "Tiesitteko,etta radiota kuunnellessa on hyva ohjelmoida ?",
        "Konekirjoitustaidosta on hyotya.",
        "Mietit viimeoista untasi,ja sitten muistat,ettet nukkunut ollenkaan.",
        "kakakakalaa.",
        "Muistat KIROSANAN,jonka joku joskus sulle selitti : P E L I  K O N S O L I",
        "Ultima 6 onnesi uus",
        "Kysy uudempaa versiota,soita 12345... (hp vain vaivaiset 999999999999999 mk.)",
        "We wish you a merry cristmas and a happy new year...1992.",
        "Zak McKracken voisi olla tan pelin kunkku.",
        "Leisuresuit Larryssa on puhtia.",
        "Putkassa mieli sekoaa.",
        "Punaista pain !!!",
        "Vinkki :poliisit vaarantavat terveytesi !!!",
        "The Last Zone...sorry soor, The Last Bone."
        };

    auto r = *select_randomly(viestit.begin(), viestit.end());
    return "Ilma alkaa vareilla ja kuulet taivaallisen aanen :\n" + r;
}

bool Viesti::interact(PlayerNS::Player* player)
{
    return true;
}

static bool checkBonas(Level* level)
{
    bool ok = true;
    for (const auto& i : level->items) {
        if (i->typeToChar() == "°")
        {
            ok = false;
            break;
        }
    }
    return ok;
}

// next level
NextLevel::NextLevel(uint32_t x, uint32_t y, Level* level) : Item(x, y), level(level) { }
std::string NextLevel::typeToChar() const { return "^"; }
std::string NextLevel::getMsg() const {

    bool ok = checkBonas(level);
    if (ok) {
        return "Seuraavaan kenttaan...";
    } else {
        return "Keraa pisteet ensin....";
    }
}

bool NextLevel::interact(PlayerNS::Player* player)
{
    bool ok = checkBonas(level);
    return ok;
}
