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
        case 13: return new Vodka(coord);
        case 14: return new Granata(coord);
        default: return nullptr;
    }
}

// bona
Bona::Bona(const Coord& c) : Item(c, EBona) { }
std::string Bona::typeToChar() const { return "°"; }
std::string Bona::getMsg() const {
    return "Löysit tyhjän pullon! Panttiraha on Moskovassa pyhä.";
}

// kalja
Kalja::Kalja(const Coord& c) : Item(c, EKalja) { }
std::string Kalja::typeToChar() const { return "%"; }
std::string Kalja::getMsg() const {
    return "Baltika 9! Kahdeksan prosenttia ja miehen mittainen.";
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
    return "Jaguar-tölkki! Kiellettyä energia-alkoholia. Sydän kiittää myöhemmin.";
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
    return "Doktorskaja-makkaraa! Lääkärin määräämää.";
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
    return "Pamppu. Mahtaakohan tämä olla kadoksissa joltain politsijalta?";
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
    return "Ketjut! Gopnikin koru ja argumentti.";
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
    return "Adidas-verkkarit! Kolme raitaa. Nyt olet virallisesti gopnik.";
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
    return "Voblaa! Kuivattua kalaa oluen kanssa. Haisee kuin sotilasvarasto.";
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
    return "Omppo. Sen voi syödä, jos uskaltaa.";
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
    return "Löysit banaanin. Tuontikorvattu, mutta syötävä.";
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
    return "Kukkarosi lihavoituu " + raha + " ruplalla. Inflaatio laihduttaa sen huomenna.";
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
    return "Huomaat sen liian myöhään... liukastut paskaan.";
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
        "Mummoilla on rahaa. Ja eläke, jota ei makseta.",
        "Politsijat ovat käyneet punttisalilla. Ja ottaneet sinun lompakkosi.",
        "Rupla on vakaa. Vakaasti alaspäin.",
        "Älä seiso ikkunan lähellä.",
        "Это другое.",
        "Propusk avaa Kremlin portit.",
        "Всё идёт по плану.",
        "Vodka ei jäädy. Sinä jäädyt.",
        "Mummot tietävät, missä FSB asuu.",
        "Границы России нигде не заканчиваются.",
        "Импортозамещение: iPhone on nyt puinen.",
        "Kremlin viinakellari on Putinin pöydän päässä.",
        "Varkaille kannattaa antaa pamppua.",
        "Vankikarkuri kupsahtaa vasta kolmannella pampulla.",
        "Voblaa kannattaa syödä harkiten.",
        "Mummot saattavat huitaista käsilaukulla.",
        "Viinakauppa ei myy humaltuneille. Teoriassa.",
        "Panttilainaamo ei osta juopuneilta.",
        "Varo paskaa. Siihen liukastuu.",
        "Gopnikit kyykkivät. Älä katso heitä silmiin.",
        "Vasja lainaa kaljaa. Vasja ei koskaan palauta.",
        "Vokzalilta lähtee juna rintamalle. Paluulippuja ei myydä.",
        "IKEA palaa. Joskus. Мы вернёмся.",
        "Televisio kertoo, että kaikki on hyvin. Televisio ei valehtele.",
        "Jos otat, et aja. Jos et ota, sinut värvätään.",
        "Vinkki: politsijat vaarantavat terveytesi!",
        "Груз 200 ei ole kaljamerkki.",
        "Burb!",
        };

    auto r = *select_randomly(viestit.begin(), viestit.end());
    return "Ilma alkaa väreillä ja kuulet taivaallisen äänen:\n" + r;
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

// pistooli
Pistooli::Pistooli(const Coord& c) : Item(c, EPistooli) { }
std::string Pistooli::typeToChar() const { return "T"; }
std::string Pistooli::getMsg() const {
    return "Löysit pistoolin! Vaarallinen kapistus.";
}
bool Pistooli::interact(PlayerNS::Player* player) {
    ++player->inventory.pistooli;
    player->inventory.ammukset += (std::rand() % 4 + 2);
    return true;
}

// bus stop
BusStop::BusStop(const Coord& c) : Item(c, EBusStop) { }
std::string BusStop::typeToChar() const { return "H"; }
std::string BusStop::getMsg() const {
    return "Bussipysäkki. Odota bussia.";
}
bool BusStop::interact(PlayerNS::Player* player) {
    return false;
}

// pensas
Pensas::Pensas(const Coord& c) : Item(c, EPensas) { blocksLOS = true; }
std::string Pensas::typeToChar() const { return "#"; }
std::string Pensas::getMsg() const {
    return "Pensas. Voit piiloutua tänne.";
}
bool Pensas::interact(PlayerNS::Player* player) {
    return false;
}

// roskis
Roskis::Roskis(const Coord& c) : Item(c, ERoskis) { blocksLOS = true; }
std::string Roskis::typeToChar() const { return "U"; }
std::string Roskis::getMsg() const {
    return "Roskis. Haisee, mutta voit piiloutua taakse.";
}
bool Roskis::interact(PlayerNS::Player* player) {
    return false;
}

// oksennus
Oksennus::Oksennus(const Coord& c) : Item(c, EOksennus) { }
std::string Oksennus::typeToChar() const { return "‘"; }
std::string Oksennus::getMsg() const {
    return "Maassa on oma oksennuksesi. Sulaneita pelmenejä ja šaurmaa. Sinulle tulee taas nälkä.";
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
        return "Seuraavaan kenttään...";
    } else {
        return "Kerää ensin kaikki tyhjät pullot...";
    }
}

bool NextLevel::interact(PlayerNS::Player* player)
{
    bool ok = checkBonas(level);
    return ok;
}

// propusk (Kremlin pass)
Propusk::Propusk(const Coord& c) : Item(c, EPropusk) { }
std::string Propusk::typeToChar() const { return "P"; }
std::string Propusk::getMsg() const {
    return "Kremlin kulkulupa! Pääset nyt Kremliin!";
}
bool Propusk::interact(PlayerNS::Player* player) {
    ++player->inventory.propusk;
    player->hasKremlinPass = true;
    return true;
}

// granata (grenade)
Granata::Granata(const Coord& c) : Item(c, EGranata) { }
std::string Granata::typeToChar() const { return "G"; }
std::string Granata::getMsg() const {
    return "Kranaatti! Vaarallinen löytö.";
}
bool Granata::interact(PlayerNS::Player* player) {
    ++player->inventory.granata;
    return true;
}

// vodka
Vodka::Vodka(const Coord& c) : Item(c, EVodka) { }
std::string Vodka::typeToChar() const { return "V"; }
std::string Vodka::getMsg() const {
    return "Vodka! Venäjän vesijohtovettä.";
}
bool Vodka::interact(PlayerNS::Player* player) {
    ++player->inventory.vodka;
    return true;
}

// viina kellari (wine cellar door - victory item)
ViinaKellari::ViinaKellari(const Coord& c, Level* level) : Item(c, EViinaKellari), level(level) { }
std::string ViinaKellari::typeToChar() const { return "\xD0\x96"; }
std::string ViinaKellari::getMsg() const {
    return "Putinin viinakellariin johtava ovi!";
}
bool ViinaKellari::interact(PlayerNS::Player* player) {
    if (player->hasKremlinPass) {
        player->wonGame = true;
        return true;
    }
    return false;
}

// PUTININ PITKÄ PÖYTÄ
PutinPoyta::PutinPoyta(const Coord& c, const char* glyph, bool putinsDesk)
    : Item(c, EPutinPoyta), glyph(glyph), putinsDesk(putinsDesk) { }
std::string PutinPoyta::typeToChar() const { return glyph; }
std::string PutinPoyta::getMsg() const {
    if (!putinsDesk)
        return "Karttapöytä. Siinä on kahvitahroja, vodkarenkaita ja liian monta nuolta.";

    // Putin plays with dolls on his desk, like Dark Helmet in Spaceballs
    static const char* dolls[] = {
        "Pöydällä on leikkinukkeja: Zelenskyi ja von der Leyen. Putin on ilmeisesti leikkinyt niillä.",
        "Zelenskyi- ja von der Leyen -nuket suutelevat. Joku on piirtänyt niille sydämen tussilla.",
        "Pöydän toisessa päässä istuu pieni Macron-nukke. Hyvin, hyvin kaukana.",
        "Rutte-nukella on kädessä lappu 'Artikla 5'. Putin on yrittänyt repiä sen irti.",
        "Stubb-nukke on puettu saunatakkiin. Vieressä lappu: 'NATO?! MIKSI?!'",
        "Pienoistankki on ajettu Zelenskyi-nuken yli. Zelenskyi-nukke seisoo silti.",
        "Pöydällä on Putin-nukke hevosen selässä ilman paitaa. Se on kuluneempi kuin muut.",
        "Pöytä on naurettavan pitkä. Toisessa päässä on nukeille katettu teekutsut.",
    };
    return dolls[std::rand() % (sizeof(dolls) / sizeof(dolls[0]))];
}
bool PutinPoyta::interact(PlayerNS::Player* player) {
    return false;
}

// war map
Kartta::Kartta(const Coord& c, const char* glyph) : Item(c, EKartta), glyph(glyph) { }
std::string Kartta::getMsg() const {
    return "Sotakartta. Nuolet osoittavat yh\xC3\xA4 Kiovaan. Joku on piirt\xC3\xA4nyt ne tussilla uudestaan.";
}
