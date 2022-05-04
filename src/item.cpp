#include "item.hpp"
#include "player.hpp"
#include "level.hpp"
#include <string>
#include <vector>

using std::string;
using std::vector;

void Item::actThrow(Level* level)
{
    coord.x += throwVec.x;
    coord.y += throwVec.y;
}

Item* Item::createItem(uint32_t index, Coord coord) {
    switch(index) {
        case 1: return new Kalja(coord);
        case 2: return new Lonkka(coord);
        case 3: return new Lenkki(coord);
        case 4: return new Ketjut(coord);
        case 5: return new Veitsi(coord);
        case 6: return new Kivi(coord);
        case 7: return new Pamppu(coord);
        case 8: return new Bootsit(coord);
        case 9: return new Kala(coord);
        case 10: return new Omppo(coord);
        case 11: return new Banaani(coord);
        case 12: return new Oksennus(coord);
        default: return nullptr;
    }
}

// bona
Bona::Bona(const Coord& c) : Item(c, EBona) { }
std::string Bona::typeToChar() const { return "°"; }
std::string Bona::getMsg() const {
    return "Löysit bonan!";
}

// kalja
Kalja::Kalja(const Coord& c) : Item(c, EKalja) { }
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
Lonkka::Lonkka(const Coord& c) : Item(c, ELonkka) { }
std::string Lonkka::typeToChar() const { return "&"; }
std::string Lonkka::getMsg() const {
    return "Kappas, lonkan mittainen !!!";
}
bool Lonkka::interact(PlayerNS::Player* player)
{
    ++player->inventory.lonkka;
    return true;
}

// lenkki
Lenkki::Lenkki(const Coord& c) : Item(c, ELenkki) { }
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
Pamppu::Pamppu(const Coord& c) : Item(c, EPamppu) { }
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
Veitsi::Veitsi(const Coord& c) : Item(c, EVeitsi) { }
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
Ketjut::Ketjut(const Coord& c) : Item(c, EKetjut) { }
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
Kivi::Kivi(const Coord& c) : Item(c, EKivi) { }
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
Bootsit::Bootsit(const Coord& c) : Item(c, EBootsit) { }
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
Kala::Kala(const Coord& c) : Item(c, EKala) { }
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
Omppo::Omppo(const Coord& c) : Item(c, EOmppo) { }
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
Banaani::Banaani(const Coord& c) : Item(c, EBanaani) { }
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
Raha::Raha(const Coord& c) : Item(c, ERaha) {
    value = std::rand() % 29 + 1;
}

Raha::Raha(const Coord& c, uint32_t value) : Item(c, ERaha)
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
    player->updateMoney(value);
    return true;
}

// paska
Paska::Paska(const Coord& c) : Item(c, EPaska) { }
std::string Paska::typeToChar() const { return "p"; }
std::string Paska::getMsg() const {
    return "Huomaat sen liian myohaan... liukastut paskaan.";
}

bool Paska::interact(PlayerNS::Player* player)
{
    player->damage(1);
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

Viesti::Viesti(const Coord& c) : Item(c, EViesti) { }
std::string Viesti::typeToChar() const { return "?"; }
std::string Viesti::getMsg() const {
    const vector<string> viestit = {
        "Mummoilla on rahaa.",
        "Poliisit on käyny punttisalilla.",
        "Alcossa myydään tavaraa.",
        "Varkaille kannattaa antaa pamppua.",
        "Sven Olof on niitä miehiä.",
        "Vankikarkuri kupsahtaa vasta kolmannella pampulla",
        "PULTSARI   -  Written By Mika Maaranen                          28.12.1991",
        "Kalaa kannattaa syodä harkiten.",
        "Sinulla on 50 % mahdollisuus,siihen,että omena on herkullinen",
        "Poliisien kanssa ei ole leikkimistä. ",
        "Mummot saattavat huitaista käsilaukulla.",
        "Tässäkin pelissä alco ottaa ylihintaa.",
        "Varkaat voi nuijia vain heittämallä.",
        "Vauhtia siihen peliin !!",
        "Mitas oikein kuppaat ? Olet hakannut vasta pari mummoa !",
        "Mummelit väistelevät heittoja.",
        "Skinhead on saattanut sekoontua eläimiin...",
        "Kankkusta kannattaa vältellä. Tai ainakin tässä pelissä.",
        "K-RAUTAa ei ole ohjelmoitu loppuun.",
        "Tavatessasi kaljapullon etikettiä , tavaat: L-A-H-D-E-N IIIIIIIIIIIIIIII",
        "Varo paskaa.Siihen liukastuu.",
        "Oispa tässä pelissä vessa.",
        "Alco ei myy humaltuneille.",
        "Divari ei osta juopuneilta.",
        "PULTSARI:  -  Idea By Mika -,Jari -,Tero Maaranen and Kimmo Korhonen.",
        "Mummot on vaarallisia.",
        "AIDS tappaa.",
        "Jos otat,et aja.",
        "Poliisi asemalla on putka,kokeile sitä(vasta viidennen levelin jalkeen)",
        "Burb !!",
        "Tama ohjelma on laadittu kayttamalla Qbasic 4.5 - ohjelmaa.",
        "Vankikarkurit ne heittelleepi ilkeesti kivillä - jos ne loytää sellasen.",
        "By Miksu Soft. Eikun pulsusoftware 5.4.2022.",
        "Tiesittekö,että radiota kuunnellessa on hyvä ohjelmoida ?",
        "Konekirjoitustaidosta on hyötyä.",
        "Mietit viimeöista untasi,ja sitten muistat,ettet nukkunut ollenkaan.",
        "kakakakalaa.",
        "Muistat KIROSANAN,jonka joku joskus sulle selitti : P E L I  K O N S O L I",
        "Ultima 6 onnesi uus",
        "Kysy uudempaa versiota,soita 12345... (hp vain vaivaiset 999999999999999 mk.)",
        "We wish you a merry cristmas and a happy new year...1992.",
        "Zak McKracken voisi olla tän pelin kunkku.",
        "Leisuresuit Larryssa on puhtia.",
        "Putkassa mieli sekoaa.",
        "Punaista päin !!!",
        "Vinkki :poliisit vaarantavat terveytesi !!!",
        "The Last Zone...sorry soor, The Last Bone.",
        "Pultsari C++ conversion by Tero Maaranen 04/2022."
        };

    auto r = *select_randomly(viestit.begin(), viestit.end());
    return "Ilma alkaa väreilla ja kuulet taivaallisen äänen :\n" + r;
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

// oksennus
Oksennus::Oksennus(const Coord& c) : Item(c, EOksennus) { }
std::string Oksennus::typeToChar() const { return "‘"; }
std::string Oksennus::getMsg() const {
    return "Maassa on oma oksennuksesi. Sulaneita kebabia ja ranskalaisilta. Sinun tulee taas nälkä.";
}

bool Oksennus::interact(PlayerNS::Player* player)
{
    return true;
}

// next level
NextLevel::NextLevel(const Coord& c, Level* level) : Item(c, ENextLevel), level(level) { }
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
