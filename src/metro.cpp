#include "metro.hpp"
#include "player.hpp"
#include "level.hpp"
#include "printer.hpp"
#include "input.hpp"
#include "common.hpp"
#include "translate.hpp"
#include "scoreboard.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdlib>

using std::string;
using std::vector;

namespace MetroNS {

namespace {

// tunnel rows, top to bottom: wall, walkway, live rail, track, gravel, track, live rail, walkway, wall
constexpr int T_WALLN = 5, T_WALKN = 6, T_LIVEA = 7, T_TRACKA = 8, T_MID = 11, T_TRACKB = 12,
              T_LIVEB = 15, T_WALKS = 16, T_WALLS = 17;
constexpr float TUNNEL_SPEED = 0.04f;   // cells per ms: a train crosses the screen in three seconds

// platform rows: mosaic wall, track (the train), yellow edge, marble floor, wall
constexpr int P_WALL = 2, P_TRACK = 3, P_EDGE = 6, P_TOP = 7, P_BOT = 20, P_EXIT = 21;
constexpr int STOP_X = 4, CAR_LEN = 24, CAR_STEP = 25, CARS = 3;
constexpr int DOOR_OFFS[] = { 6, 7, 16, 17 };

// carriage rows: wall, seats, aisle, seats, wall
constexpr int R_WALLT = 5, R_SEATT = 6, R_TOP = 7, R_BOT = 13, R_SEATB = 14, R_WALLB = 15;
constexpr int R_DOORS[] = { 9, 29, 49, 69 };   // left column of each two-wide door

int rnd(int n) { return n > 0 ? std::rand() % n : 0; }
bool chance(int pct) { return rnd(100) < pct; }
int sgn(int v) { return (v > 0) - (v < 0); }

// UTF-8 code points of a string, one glyph each
vector<string> glyphs(const string& s)
{
    vector<string> out;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = s[i];
        const size_t n = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
        out.push_back(s.substr(i, n));
        i += n;
    }
    return out;
}

// a paragraph cut into lines of at most `width` glyphs
vector<string> wrap(const string& text, size_t width)
{
    vector<string> lines;
    string line, word;
    auto len = [](const string& s) { return glyphs(s).size(); };
    auto flushWord = [&]() {
        if (word.empty()) return;
        if (!line.empty() && len(line) + 1 + len(word) > width) { lines.push_back(line); line.clear(); }
        line += (line.empty() ? "" : " ") + word;
        word.clear();
    };
    for (char c : text) {
        if (c == ' ') flushWord();
        else word += c;
    }
    flushWord();
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// "äkkipikainen" -> "Äkkipikainen"
string capitalized(const string& s)
{
    if (s.compare(0, 2, "\xC3\xA4") == 0) return "\xC3\x84" + s.substr(2);
    string out = s;
    if (!out.empty()) out[0] = (char)std::toupper((unsigned char)out[0]);
    return out;
}

// Russian, romanised the way the player hears it, with the Finnish after it
string speech(const string& raw) { return latinize(withTranslation(raw)); }

} // namespace

const char* MetroGame::kindName(Kind k)
{
    switch (k) {
        case Kind::Citizen:    return "kansalainen";
        case Kind::Babushka:   return "mummo";
        case Kind::Drunk:      return "juoppo";
        case Kind::Gopnik:     return "jengiläinen";
        case Kind::Dealer:     return "diileri";
        case Kind::Hothead:    return "äkkipikainen";
        case Kind::Cop:        return "kyttä";
        case Kind::Fsb:        return "FSB:n värvääjä";
        case Kind::Pickpocket: return "taskuvaras";
        case Kind::Rat:        return "rotta";
    }
    return "joku";
}

MetroGame::MetroGame(PlayerNS::Player* player, const string& from, const string& to, const string& after, bool dodgedFare)
    : player(player), from(from), to(to), after(after), dodged(dodgedFare)
{
    startMoney = money();
    startTunnel();
}

uint32_t MetroGame::money() const { return player->stats.money; }

void MetroGame::pay(uint32_t amount)
{
    player->stats.money -= std::min(amount, player->stats.money);
}

void MetroGame::say(const string& text)
{
    msg = speech(text);
    msgMs = 4000;
}

void MetroGame::finish(Outcome o, const string& why)
{
    if (phase == Phase::Over) return;
    phase = Phase::Over;
    outcome = o;
    cause = why;
    prompt = Prompt();
    t = 0;
}

void MetroGame::hurt(uint32_t dmg, const string& why)
{
    auto& hp = player->stats.health;
    hp = hp > dmg ? hp - dmg : 0;
    common::sound("splat", 0.8f);
    shake = std::max(shake, 250);
    if (hp == 0) finish(Outcome::Dead, why);
}

// ---------------------------------------------------------------- setup

void MetroGame::addNpc(Kind k, int x, int y, bool seated)
{
    static const char* citizenColors[] = { "38;5;180", "38;5;145", "38;5;110", "38;5;181", "38;5;144", "38;5;109", "38;5;175" };
    Npc n;
    n.kind = k;
    n.x = x;
    n.y = y;
    n.seated = seated;
    n.cooldown = rnd(600);
    n.waitMs = -1;
    switch (k) {
        case Kind::Citizen: {
            // any letter but the ones the troublemakers wear
            static const char letters[] = "abcehiklnopstuwyz";
            n.glyph = string(1, letters[rnd(sizeof(letters) - 1)]);
            n.color = citizenColors[rnd(7)];
            break;
        }
        case Kind::Babushka:   n.glyph = "M"; n.color = "38;5;168"; break;
        case Kind::Drunk:      n.glyph = "J"; n.color = "38;5;137"; break;
        case Kind::Gopnik:     n.glyph = "g"; n.color = "1;38;5;27"; break;
        case Kind::Dealer:     n.glyph = "D"; n.color = "38;5;135"; break;
        case Kind::Hothead:    n.glyph = "O"; n.color = "1;38;5;223"; break;
        case Kind::Cop:        n.glyph = "C"; n.color = "1;38;5;68"; break;
        case Kind::Fsb:        n.glyph = "F"; n.color = "1;38;5;245"; break;
        case Kind::Pickpocket: n.glyph = "v"; n.color = "38;5;246"; break;
        case Kind::Rat:        n.glyph = "r"; n.color = "38;5;246"; break;
    }
    npcs.push_back(n);
}

void MetroGame::startTunnel()
{
    phase = Phase::Tunnel;
    t = 0;
    npcs.clear();
    prompt = Prompt();
    tiles.assign(H, vector<uint8_t>(W, TVoid));
    for (int x = 0; x < W; ++x) {
        tiles[T_WALLN][x] = TWall;
        tiles[T_WALKN][x] = TWalk;
        tiles[T_LIVEA][x] = TLive;
        tiles[T_TRACKA][x] = tiles[T_TRACKA + 2][x] = TRail;
        tiles[T_TRACKA + 1][x] = TSleeper;
        tiles[T_MID][x] = x % 6 == 3 ? TPillar : TGravel;
        tiles[T_TRACKB][x] = tiles[T_TRACKB + 2][x] = TRail;
        tiles[T_TRACKB + 1][x] = TSleeper;
        tiles[T_LIVEB][x] = TLive;
        tiles[T_WALKS][x] = x >= 74 ? TWall : TWalk;   // the south walkway ends in a dead end
        tiles[T_WALLS][x] = TWall;
    }
    tiles[T_WALKN][W - 1] = TExit;

    // boards over the live rail where a man can cross; junk on the walkways next to them,
    // so going round it means stepping onto a board
    for (int row : { T_LIVEA, T_LIVEB }) {
        for (int k = 0; k < 4; ++k) {
            const int x = 6 + k * 18 + rnd(8);
            for (int i = 0; i < 3; ++i) tiles[row][x + i] = TCover;
            const int walk = row == T_LIVEA ? T_WALKN : T_WALKS;
            if (k > 0 && tiles[walk][x + 1] == TWalk) tiles[walk][x + 1] = TJunk;
        }
    }

    px = 1;
    py = T_WALKS;
    for (int i = 0; i < 6; ++i) {
        const int rows[] = { T_WALKN, T_WALKS, T_MID, T_TRACKA + 1, T_TRACKB + 1 };
        int x, y;
        do { x = 10 + rnd(W - 14); y = rows[rnd(5)]; } while (!walkable(x, y) || npcAt(x, y) >= 0);
        addNpc(Kind::Rat, x, y);
    }

    // the nearer track gets its train soon, so the player sees what one does
    trains[0] = Train();
    trains[0].dir = -1;
    trains[0].nextMs = 7000 + rnd(4000);
    trains[1] = Train();
    trains[1].dir = 1;
    trains[1].nextMs = 3500;
    say("Liukuportaat ovat rikki. Laiturille pääsee vain huoltotunnelin kautta - pimeää, ja junat kulkevat.");
}

void MetroGame::startPlatform()
{
    phase = Phase::Platform;
    t = 0;
    npcs.clear();
    prompt = Prompt();
    tiles.assign(H, vector<uint8_t>(W, TVoid));
    for (int x = 0; x < W; ++x) {
        tiles[P_WALL][x] = TWall;
        tiles[P_TRACK][x] = tiles[P_TRACK + 2][x] = TRail;
        tiles[P_TRACK + 1][x] = TSleeper;
        tiles[P_EDGE][x] = TEdge;
        for (int y = P_TOP; y <= P_BOT; ++y) tiles[y][x] = TFloor;
        tiles[P_EXIT][x] = TWall;
    }
    // marble pillars between the carriages
    for (int y = P_TOP + 3; y <= P_BOT - 3; ++y)
        for (int x : { 1, 2, 27, 28, 52, 53, 77, 78 }) tiles[y][x] = TPillar;

    px = 40;
    py = P_BOT;
    for (int i = 0; i < 46; ++i) {
        const int r = rnd(100);
        const Kind k = r < 58 ? Kind::Citizen : r < 76 ? Kind::Babushka : r < 86 ? Kind::Drunk : Kind::Gopnik;
        int x, y;
        do { x = 1 + rnd(W - 2); y = P_EDGE + rnd(P_BOT - P_EDGE - 1); } while (!walkable(x, y) || npcAt(x, y) >= 0 || (x == px && y == py));
        addNpc(k, x, y);
    }
    addNpc(Kind::Cop, 15, P_TOP + 6);
    addNpc(Kind::Cop, 65, P_TOP + 8);

    trains[0] = Train();
    stop = Stop::Waiting;
    stopMs = 0;
    say("Kiipeät huoltoportaita laiturille. Ihmisiä on kuin sillejä. Pääse junaan - lyömättä ketään.");
}

void MetroGame::startRide()
{
    phase = Phase::Ride;
    t = 0;
    npcs.clear();
    prompt = Prompt();
    tiles.assign(H, vector<uint8_t>(W, TVoid));
    for (int x = 0; x < W; ++x) {
        tiles[R_WALLT][x] = tiles[R_WALLB][x] = TCarWall;
        tiles[R_SEATT][x] = tiles[R_SEATB][x] = TSeat;
        for (int y = R_TOP; y <= R_BOT; ++y) tiles[y][x] = TFloor;
    }
    for (int d : R_DOORS) {
        for (int i = 0; i < 2; ++i) {
            tiles[R_WALLT][d + i] = tiles[R_WALLB][d + i] = TDoor;
            tiles[R_SEATT][d + i] = tiles[R_SEATB][d + i] = TFloor;   // standing room by the doors
        }
    }
    for (int y = R_WALLT; y <= R_WALLB; ++y) tiles[y][0] = tiles[y][W - 1] = TCarWall;
    for (int x : { 19, 39, 59 }) tiles[R_TOP + 1][x] = tiles[R_BOT - 1][x] = TPole;

    px = 30;
    py = R_BOT;

    // seated: the quiet ones, mostly
    for (int i = 0; i < 16; ++i) {
        const int r = rnd(100);
        const Kind k = r < 50 ? Kind::Citizen : r < 78 ? Kind::Babushka : Kind::Drunk;
        int x, y;
        do { x = 1 + rnd(W - 2); y = chance(50) ? R_SEATT : R_SEATB; } while (tiles[y][x] != TSeat || npcAt(x, y) >= 0);
        addNpc(k, x, y, true);
    }
    auto standing = [&](Kind k, int xmin, int xmax) -> Npc& {
        int x, y;
        do { x = xmin + rnd(xmax - xmin + 1); y = R_TOP + rnd(R_BOT - R_TOP + 1); } while (!walkable(x, y) || npcAt(x, y) >= 0 || (std::abs(x - px) < 3 && std::abs(y - py) < 3));
        addNpc(k, x, y);
        return npcs.back();
    };
    for (int i = 0; i < 3; ++i) standing(Kind::Gopnik, 64, 76);
    standing(Kind::Drunk, 34, 46).waitMs = 4000 + rnd(8000);
    standing(Kind::Dealer, 4, 16).waitMs = 6000 + rnd(10000);
    standing(Kind::Hothead, 40, 54);
    standing(Kind::Pickpocket, 20, 40).waitMs = 3000 + rnd(6000);
    standing(Kind::Fsb, 1, 6).waitMs = 9000;
    standing(Kind::Cop, 74, 78).waitMs = 15000 + rnd(5000);
    for (int i = 0; i < 5; ++i) standing(chance(70) ? Kind::Citizen : Kind::Babushka, 2, 77);

    rideLeft = RIDE_MS;
    doorsMs = 0;
    overshot = false;
    joltMs = 7000 + rnd(3000);
    say("Ovet paukahtavat kiinni selkäsi takana. «Осторожно, двери закрываются!» Seuraava asema: " + to + ".");
}

// ---------------------------------------------------------------- queries

bool MetroGame::walkable(int x, int y) const
{
    if (x < 0 || y < 0 || x >= W || y >= H) return false;
    switch (tiles[y][x]) {
        case TWalk: case TLive: case TCover: case TRail: case TSleeper: case TGravel: case TExit:
        case TFloor: case TEdge:
            return true;
        default:
            return false;
    }
}

int MetroGame::npcAt(int x, int y) const
{
    for (size_t i = 0; i < npcs.size(); ++i)
        if (!npcs[i].gone && npcs[i].x == x && npcs[i].y == y) return (int)i;
    return -1;
}

int MetroGame::trainX() const
{
    const float p = std::min(1.0f, stopMs / 2500.0f);
    switch (stop) {
        case Stop::Arriving: return STOP_X - (int)(95.0f * (1 - p) * (1 - p));
        case Stop::Leaving:  return STOP_X + (int)(95.0f * p * p);
        default:             return STOP_X;
    }
}

bool MetroGame::trainAt(int x, int y) const
{
    if (phase == Phase::Tunnel) {
        for (int k = 0; k < 2; ++k) {
            const Train& tr = trains[k];
            const int top = k == 0 ? T_TRACKA : T_TRACKB;
            if (!tr.active || y < top || y > top + 2) continue;
            const int x0 = (int)std::floor(tr.x);
            if (x >= x0 && x < x0 + tr.len) return true;
        }
        return false;
    }
    if (phase == Phase::Platform) {
        if (!trains[0].active || y < P_TRACK || y > P_TRACK + 2) return false;
        const int rel = x - trainX();
        return rel >= 0 && rel < CAR_STEP * CARS - 1 && rel % CAR_STEP < CAR_LEN;
    }
    return false;
}

bool MetroGame::doorsOpen() const
{
    if (phase == Phase::Platform) return stop == Stop::Unloading || stop == Stop::Boarding || stop == Stop::Closing;
    if (phase == Phase::Ride) return doorsMs > 0;
    return false;
}

// the platform door nearest to a column
int MetroGame::stationDoorX(int x) const
{
    int best = STOP_X + DOOR_OFFS[0], bestD = 1000;
    for (int c = 0; c < CARS; ++c)
        for (int o : DOOR_OFFS) {
            const int d = STOP_X + c * CAR_STEP + o;
            if (std::abs(d - x) < bestD) { bestD = std::abs(d - x); best = d; }
        }
    return best;
}

static bool isPlatformDoor(int x, int y)
{
    if (y != P_TRACK + 2) return false;
    const int rel = x - STOP_X;
    if (rel < 0 || rel >= CAR_STEP * CARS) return false;
    const int o = rel % CAR_STEP;
    return o == 6 || o == 7 || o == 16 || o == 17;
}

bool MetroGame::near(const Npc& n, int d) const
{
    return std::abs(n.x - px) <= d && std::abs(n.y - py) <= d;
}

// ---------------------------------------------------------------- the player

void MetroGame::key(char c)
{
    if (phase == Phase::Over) return;
    if (prompt.id) {
        if (c >= '1' && c <= '0' + (int)prompt.options.size()) answer(c - '1');
        return;
    }
    switch (c) {
        case 'w': case 'i': case 's': case 'k': case 'a': case 'j': case 'd': case 'l': queued = c; break;
        default: break;
    }
}

void MetroGame::step(int dx, int dy)
{
    if (phase == Phase::Over) return;
    const int nx = px + dx, ny = py + dy;

    const int i = npcAt(nx, ny);
    if (i >= 0) {
        if (npcs[i].kind == Kind::Rat) {
            npcs[i].gone = true;
            say("Potkaiset rotan pimeään. Se vinkaisee.");
        } else {
            hitNpc(i);
        }
        return;
    }

    switch (phase) {
        case Phase::Tunnel: {
            if (nx < 0) {
                finish(Outcome::TurnedBack, "Käännyit takaisin kadulle. Lippu meni hukkaan, mutta henki säilyi.");
                return;
            }
            if (!walkable(nx, ny)) return;
            px = nx;
            py = ny;
            if (tiles[py][px] == TExit) { startPlatform(); return; }
            if (tiles[py][px] == TLive) {
                say("ZZZT! Astuit virtakiskolle! 825 volttia kulkee läpi kehon.");
                common::sound("zap");
                stunMs = 1200;
                sinceShock = 0;
                hurt(player->stats.promilles > 20 ? 5 : 4, "Astuit metrotunnelissa virtakiskolle. 825 volttia, eikä kukaan tullut katsomaan.");
            }
            if (trainAt(px, py)) finish(Outcome::Dead, "Kävelit tunnelissa suoraan junan eteen.");
            return;
        }
        case Phase::Platform:
            if (doorsOpen() && isPlatformDoor(nx, ny)) {
                startRide();
                return;
            }
            if (!walkable(nx, ny) || trainAt(nx, ny)) return;
            px = nx;
            py = ny;
            return;
        case Phase::Ride:
            if (doorsMs > 0 && ny >= 0 && ny < H && nx >= 0 && nx < W && tiles[ny][nx] == TDoor) {
                if (overshot) finish(Outcome::Overshot, "Nousit pois vasta asemalla " + after + ". Kävelet takaisin, tai sitten et.");
                else finish(Outcome::Arrived, "Nousit pois asemalla " + to + ". Ehjänä, melkein.");
                return;
            }
            if (!walkable(nx, ny)) return;
            px = nx;
            py = ny;
            return;
        default:
            return;
    }
}

// Walking into someone is a punch, as everywhere in the game, and the metro remembers.
void MetroGame::hitNpc(int i)
{
    Npc& n = npcs[i];
    ++hits;
    ++player->scoreBoard.kaikki;
    if (n.kind == Kind::Babushka) ++player->scoreBoard.mummot;
    common::sound("splat", 0.6f);
    shake = std::max(shake, 150);

    // the second punch brings every cop in earshot, wherever they are
    auto alarm = [&]() {
        for (auto& c : npcs) {
            if (c.gone || c.kind != Kind::Cop || c.chasing) continue;
            if (std::abs(c.x - px) > 30 && hits < 2) continue;
            c.chasing = true;
            msg += " Kyttä lähtee perääsi!";
            return;
        }
    };

    switch (n.kind) {
        case Kind::Cop:
            finish(Outcome::Arrested, "Löit kyttää metrossa. Rautakourat napsahtavat, ja tuomari odottaa.");
            return;
        case Kind::Fsb:
            finish(Outcome::Recruited, "Löit FSB:n miestä. Hän vain hymyilee: sopimus on jo kirjoitettu valmiiksi, ja nyt sinulla on kiire rintamalle.");
            return;
        case Kind::Gopnik:
            say("«Ты чё, охренел?! Пацаны!» Jengiläiset käyvät kimppuusi.");
            for (auto& g : npcs) if (g.kind == Kind::Gopnik && !g.gone) g.angryMs = 12000;
            return;
        case Kind::Hothead:
            say("«Ну всё, тебе конец!» Äkkipikainen räjähtää.");
            n.angryMs = 12000;
            return;
        case Kind::Dealer:
            say("Diileri sihahtaa ja lyö takaisin.");
            n.angryMs = 6000;
            return;
        case Kind::Pickpocket:
            if (n.stolen > 0) {
                player->stats.money += n.stolen;
                say("Isket taskuvarasta - rahasi tippuvat lattialle ja nappaat ne takaisin.");
                n.stolen = 0;
            } else {
                say("Lyöt laihaa miestä. Hän katoaa väkijoukkoon.");
            }
            n.gone = true;
            return;
        default:
            break;
    }

    // ordinary people: the first punch they take, the third one they give back
    if (hits >= 3) {
        say("Matkustajat ovat saaneet tarpeekseen ja lyövät takaisin!");
        n.angryMs = 6000;
        n.seated = false;
        n.stunMs = 0;
        if (n.glyph == "_") n.glyph = "J";
        alarm();
        return;
    }
    switch (n.kind) {
        case Kind::Drunk:
            say("Juoppo kaatuu kuin säkki. Hän ei edes herää.");
            n.stunMs = 6000;
            n.glyph = "_";
            alarm();
            return;
        case Kind::Babushka:
            say("«Ой! Хулиган! Милиция!» Mummo kirkuu, ja kaikki kääntyvät katsomaan.");
            alarm();
            return;
        default:
            say("«Милиция!» Joku huutaa kyttää.");
            if (hits >= 2 || chance(60)) alarm();
            return;
    }
}

// ---------------------------------------------------------------- questions

void MetroGame::openPrompt(int npc, int id)
{
    prompt = Prompt();
    prompt.id = id;
    prompt.npc = npc;
    switch (id) {
        case 1:
            prompt.text = speech("«Братан, есть сотка?»") + " Jengiläiset piirittävät sinut.";
            prompt.options = { "Anna 100 ₽", "Sano ettei ole", "Lyö" };
            break;
        case 2:
            prompt.text = speech("«Есть кое-что. Хочешь?»") + " Diileri raottaa takkiaan.";
            prompt.options = { "Osta 150 ₽", "Ei kiitos" };
            break;
        case 3:
            prompt.text = speech("«Будешь?»") + " Juoppo ojentaa pulloa. Siinä on jotain kirkasta.";
            prompt.options = { "Ota huikka", "Kieltäydy" };
            break;
        case 4:
            prompt.text = speech("«Билет!»") + " Lipuntarkastaja. Hyppäsit porttien yli.";
            prompt.options = { "Maksa sakko 300 ₽", "Lahjo 100 ₽", "Juokse" };
            break;
        case 5:
            prompt.text = speech("«Документы!»") + " Kyttä haistaa viinan. Humalassa julkisilla on rikos.";
            prompt.options = { "Maksa \"sakko\" 100 ₽", "Väitä olevasi selvä" };
            break;
        case 6:
            prompt.text = speech("«Мужик, хочешь заработать? Контракт!»") + " 200 000 ₽ kuussa, ruokaa ja kunniaa.";
            prompt.options = { "Allekirjoita", "Kieltäydy" };
            break;
        case 7:
            prompt.text = "FSB:n mies tarttuu käsivarteesi. " + speech("«Подпиши. Сейчас.»");
            prompt.options = { "Allekirjoita", "Riuhtaise irti" };
            break;
    }
    msg = prompt.text;
    msgMs = 100000;
}

void MetroGame::answer(int option)
{
    const int id = prompt.id;
    const int who = prompt.npc;
    prompt = Prompt();
    msgMs = 0;
    Npc* n = who >= 0 && who < (int)npcs.size() ? &npcs[who] : nullptr;
    auto copNear = [&](int d) {
        for (const auto& c : npcs) if (!c.gone && c.kind == Kind::Cop && near(c, d)) return true;
        return false;
    };
    auto gopniksAngry = [&]() { for (auto& g : npcs) if (g.kind == Kind::Gopnik && !g.gone) { g.angryMs = 12000; g.talked = true; } };
    auto gopniksDone = [&]() { for (auto& g : npcs) if (g.kind == Kind::Gopnik) { g.talked = true; g.waitMs = -1; } };

    switch (id) {
        case 1:
            if (option == 0) {
                if (money() >= 100) {
                    pay(100);
                    say("Jengiläinen ottaa satasen. «Нормально.» He palaavat nurkkaansa.");
                    gopniksDone();
                } else {
                    say("Sinulla ei ole edes satasta. Se oli virhe.");
                    gopniksAngry();
                }
            } else if (option == 1) {
                if (chance(50)) { say("«Ну ладно.» Jengiläiset kohauttavat olkapäitään."); gopniksDone(); }
                else { say("«Врёшь!» Jengiläiset eivät usko sinua."); gopniksAngry(); }
            } else if (n) {
                hitNpc(who);
            }
            break;
        case 2:
            if (option == 1) { say("«Как хочешь.» Diileri katoaa väkijoukkoon."); break; }
            if (money() < 150) { say("«Без денег - без товара.» Diileri kääntyy pois."); break; }
            pay(150);
            if (copNear(12)) {
                finish(Outcome::Arrested, "Ostit kamaa kytän nenän edessä. Diileri oli tietenkin kytän serkku.");
            } else if (chance(50)) {
                player->stats.promilles += 6;
                player->stats.health = std::min<uint32_t>(PlayerNS::INITIAL_HP, player->stats.health + 2);
                say("Maailma kirkastuu. Vaunu hehkuu kultaisena ja mummot hymyilevät.");
            } else {
                say("Kamassa oli jotain outoa. Suonissa polttaa.");
                hurt(3, "Diilerin kamassa oli rotanmyrkkyä. Kuolit metrovaunun lattialle, eikä kukaan noussut penkiltään.");
            }
            break;
        case 3:
            if (option == 0) {
                player->stats.promilles += 4;
                common::sound("glug");
                say("Huikka polttaa kurkkua. Juoppo halaa sinua ja itkee.");
            } else if (n && chance(30)) {
                say("«Ты меня не уважаешь?!» Juoppo suuttuu.");
                n->angryMs = 3000;
            } else {
                say("Juoppo nyökkää ymmärtäväisesti ja nukahtaa.");
            }
            break;
        case 4:
            if (option == 0) {
                if (money() >= 300) { pay(300); say("Maksat sakon. Kyttä kirjoittaa kuitin, jota et saa."); }
                else finish(Outcome::Arrested, "Et pystynyt maksamaan sakkoa. Kyttä raahasi sinut asemalle.");
            } else if (option == 1) {
                if (money() < 100) finish(Outcome::Arrested, "Yritit lahjoa kyttää ilman rahaa. Se ei ole lahjus, se on loukkaus.");
                else {
                    pay(100);
                    if (chance(65)) say("Kyttä pistää setelin taskuunsa ja katsoo muualle.");
                    else finish(Outcome::Arrested, "Yritit lahjoa väärää kyttää. Hän otti rahat ja sinut.");
                }
            } else if (n) {
                n->chasing = true;
                n->chaseMs = 15000;
                n->stunMs = 1500;   // the head start: he was writing the fine
                say("Juokset! Kyttä lähtee perään. Pysy loitolla, kunnes ovet aukeavat tai hän väsyy.");
            }
            break;
        case 5:
            if (option == 0) {
                if (money() >= 100) { pay(100); say("Satanen vaihtaa omistajaa. «Проходи.»"); }
                else finish(Outcome::Arrested, "Olit humalassa julkisilla etkä pystynyt maksamaan. Kyttä vei sinut putkaan.");
            } else if (player->stats.promilles > 30 && chance(50)) {
                finish(Outcome::Arrested, "Väitit olevasi selvä, mutta et osannut sanoa omaa nimeäsi. Kyttä vei sinut putkaan.");
            } else {
                say("Kyttä tuhahtaa ja jatkaa matkaa.");
            }
            break;
        case 6:
            if (option == 0) {
                common::sound("pencil", 1.0f);
                finish(Outcome::Recruited, "Allekirjoitit sopimuksen metrossa. FSB:n mies taputtaa olkapäälle: «Молодец!» Seuraava pysäkki: rintama.");
            } else if (player->stats.promilles >= 25 && chance(25)) {
                common::sound("pencil", 1.0f);
                finish(Outcome::Recruited, "Yritit kieltäytyä, mutta kynä oli jo kädessäsi. Allekirjoitit humalassa. Seuraava pysäkki: rintama.");
            } else if (n) {
                ++n->refusals;
                n->talked = false;
                n->stunMs = 7000;   // backs off, for now
                say("«Подумай.» FSB:n mies perääntyy, mutta ei kauas.");
            }
            break;
        case 7:
            if (option == 0) {
                finish(Outcome::Recruited, "FSB:n mies ohjasi kätesi paperille. Seuraava pysäkki: rintama.");
            } else if (copNear(20)) {
                finish(Outcome::Arrested, "Riuhtaisit FSB:n miestä kytän nähden. Virkavallan vastustaminen.");
            } else if (n) {
                n->talked = true;
                n->stunMs = 100000;
                say("Riuhtaiset irti. FSB:n mies kirjoittaa jotain muistikirjaansa. Nimesi, luultavasti.");
            }
            break;
    }
}

// ---------------------------------------------------------------- time

void MetroGame::tick(int ms)
{
    if (phase == Phase::Over) { t += ms; return; }
    if (msgMs > 0 && (msgMs -= ms) <= 0) msg.clear();
    if (prompt.id) return;      // the world waits for the answer
    t += ms;
    shake = std::max(0, shake - ms);
    sinceStep += ms;
    if (stunMs > 0) stunMs = std::max(0, stunMs - ms);

    // the player's step: slower and less straight the drunker you are
    const uint32_t prom = player->stats.promilles;
    if (queued && stunMs == 0 && sinceStep >= 80 + (int)prom * 5) {
        int dx = 0, dy = 0;
        switch (queued) {
            case 'w': case 'i': dy = -1; break;
            case 's': case 'k': dy = 1; break;
            case 'a': case 'j': dx = -1; break;
            case 'd': case 'l': dx = 1; break;
        }
        queued = 0;
        sinceStep = 0;
        // from about one per mille up, the legs start to have their own idea
        if (chance(std::max(0, std::min<int>(35, (int)prom - 10)))) {
            // the legs have their own idea
            if (dx) { dy = chance(50) ? 1 : -1; dx = 0; }
            else { dx = chance(50) ? 1 : -1; dy = 0; }
            say("Jalat menevät omia teitään.");
        }
        step(dx, dy);
        if (phase == Phase::Over) return;
    }

    switch (phase) {
        case Phase::Tunnel:   tickTunnel(ms); break;
        case Phase::Platform: tickPlatform(ms); break;
        case Phase::Ride:     tickRide(ms); break;
        default: break;
    }
}

void MetroGame::tickTunnel(int ms)
{
    for (int k = 0; k < 2; ++k) {
        Train& tr = trains[k];
        if (!tr.active) {
            tr.nextMs -= ms;
            if (!tr.warned && tr.nextMs <= 2500) {
                tr.warned = true;
                say(k == 0 ? "Ylemmät kiskot alkavat laulaa - juna tulee oikealta!"
                           : "Alemmat kiskot värisevät - juna tulee vasemmalta!");
                common::sound("train", 0.5f);
            }
            if (tr.nextMs <= 0) {
                tr.active = true;
                tr.x = tr.dir < 0 ? (float)W : (float)-tr.len;
                common::sound("train");
            }
        } else {
            tr.x += tr.dir * TUNNEL_SPEED * ms;
            shake = std::max(shake, 120);
            if ((tr.dir < 0 && tr.x + tr.len < 0) || (tr.dir > 0 && tr.x > W)) {
                tr.active = false;
                tr.warned = false;
                tr.nextMs = 4500 + rnd(6000);
            }
        }
    }
    for (auto& n : npcs) if (!n.gone && trainAt(n.x, n.y)) n.gone = true;
    if (trainAt(px, py)) {
        finish(Outcome::Dead, "Juna tuli tunnelissa. Kuljettaja ei ehtinyt edes jarruttaa. Sinusta ei jäänyt paljon kerättävää.");
        return;
    }
    // standing on the live rail keeps on hurting, with a moment to get off it first
    sinceShock += ms;
    if (tiles[py][px] == TLive && stunMs == 0 && sinceShock >= 1700) {
        say("ZZZT! Olet yhä virtakiskolla!");
        common::sound("zap");
        stunMs = 1200;
        sinceShock = 0;
        hurt(4, "Jäit seisomaan virtakiskolle. 825 volttia, eikä kukaan tullut katsomaan.");
        if (phase == Phase::Over) return;
    }
    moveNpcs(ms);
}

void MetroGame::tickPlatform(int ms)
{
    stopMs += ms;
    switch (stop) {
        case Stop::Waiting: {
            const int wait = missed == 0 ? 4500 : 7000;
            if (!trains[0].warned && stopMs >= wait - 2000) {
                trains[0].warned = true;
                say("Tunnelista puhaltaa lämmin tuuli. Juna tulee! Tungos painautuu reunalle.");
                common::sound("train", 0.6f);
            }
            if (stopMs >= wait) { stop = Stop::Arriving; stopMs = 0; trains[0].active = true; }
            break;
        }
        case Stop::Arriving:
            shake = std::max(shake, 100);
            if (stopMs >= 2500) {
                stop = Stop::Unloading;
                stopMs = 0;
                common::sound("chime");
                say("Juna pysähtyy ja ovet aukeavat. Ihmisiä tulvii ulos.");
                for (int k = 0; k < 7; ++k) {
                    const int c = rnd(CARS);
                    const int x = STOP_X + c * CAR_STEP + DOOR_OFFS[rnd(4)];
                    if (npcAt(x, P_EDGE) < 0 && !(px == x && py == P_EDGE)) {
                        addNpc(chance(70) ? Kind::Citizen : Kind::Babushka, x, P_EDGE);
                        npcs.back().exiting = true;
                    }
                }
                for (auto& n : npcs) if (!n.exiting && n.kind != Kind::Cop && chance(75)) n.boarding = true;
            }
            break;
        case Stop::Unloading:
            if (stopMs >= 1500) { stop = Stop::Boarding; stopMs = 0; }
            break;
        case Stop::Boarding:
            if (stopMs >= 8000) {
                stop = Stop::Closing;
                stopMs = 0;
                common::sound("chime");
                say("«Осторожно, двери закрываются!»");
            }
            break;
        case Stop::Closing:
            if (stopMs >= 2000) { stop = Stop::Leaving; stopMs = 0; }
            break;
        case Stop::Leaving:
            shake = std::max(shake, 100);
            if (stopMs >= 2500) {
                stop = Stop::Waiting;
                stopMs = 0;
                trains[0] = Train();
                ++missed;
                say("Juna lähti ilman sinua. Laiturille valuu lisää väkeä. Seuraava juna tulee pian.");
                for (auto& n : npcs) n.boarding = false;
                for (int k = 0; k < 14; ++k) {
                    const int x = 3 + rnd(W - 6);
                    if (walkable(x, P_BOT) && npcAt(x, P_BOT) < 0 && !(px == x && py == P_BOT))
                        addNpc(chance(65) ? Kind::Citizen : chance(50) ? Kind::Babushka : Kind::Drunk, x, P_BOT);
                }
            }
            break;
    }

    // the crowd leans in as the train comes, and someone always shoves
    if (stop == Stop::Arriving && py == P_EDGE && t % 300 < ms) {
        int pushers = 0;
        for (const auto& n : npcs) if (!n.gone && near(n, 1)) ++pushers;
        if (pushers > 0 && chance(4 + (int)player->stats.promilles / 2 + 5 * pushers)) {
            py = P_TRACK + 2;
            shake = 400;
            say("Tungos tönäisee selästä - putoat raiteille! Kiipeä ylös!");
        }
    }
    if ((stop == Stop::Arriving || stop == Stop::Leaving) && trainAt(px, py)) {
        finish(Outcome::Dead, "Putosit laiturilta junan alle. Metro kulki silti aikataulussa.");
        return;
    }
    moveNpcs(ms);
}

void MetroGame::jolt()
{
    const int dir = chance(50) ? 1 : -1;
    shake = 450;
    common::sound("train", 0.4f);
    for (auto& n : npcs) if (!n.gone && !n.seated && n.stunMs == 0) npcStep(n, dir, 0);

    bool holding = false;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            const int x = px + dx, y = py + dy;
            if (x >= 0 && y >= 0 && x < W && y < H && tiles[y][x] == TPole) holding = true;
        }
    if (holding) { say("Juna nytkähtää! Pidät tangosta kiinni."); return; }

    const int i = npcAt(px + dir, py);
    if (i >= 0) {
        Npc& n = npcs[i];
        if (n.kind == Kind::Hothead || n.kind == Kind::Gopnik) {
            say(string("Juna nytkähtää ja horjahdat ") + (n.kind == Kind::Hothead ? "äkkipikaisen" : "jengiläisen") + " päälle. «Ты чё толкаешься?!»");
            n.angryMs = 9000;
        } else if (n.kind == Kind::Cop) {
            say("Juna nytkähtää ja horjahdat kytän päälle. Hän katsoo sinua pitkään.");
        } else {
            say("Juna nytkähtää ja horjahdat jonkun päälle. Mutinaa.");
        }
        return;
    }
    if (walkable(px + dir, py)) px += dir;
    if (player->stats.promilles > 25 && chance(30)) {
        say("Juna nytkähtää ja kaadut lattialle!");
        hurt(1, "Kaaduit metrovaunun lattialle etkä enää noussut.");
    } else {
        say("Juna nytkähtää! Pidä kiinni tangosta.");
    }
}

void MetroGame::tickRide(int ms)
{
    if (doorsMs == 0) {
        rideLeft -= ms;
        if ((joltMs -= ms) <= 0) {
            jolt();
            joltMs = 7000 + rnd(4000);
            if (phase == Phase::Over) return;
        }
        if (rideLeft <= 0) {
            doorsMs = DOORS_MS;
            common::sound("chime");
            say("«Станция» " + (overshot ? after : to) + ". Ovet aukeavat - ulos ovesta, nopeasti!");
        }
    } else {
        doorsMs -= ms;
        if (doorsMs <= 0) {
            doorsMs = 0;
            if (!overshot) {
                overshot = true;
                rideLeft = 16000;
                say("«Осторожно, двери закрываются!» Ovet sulkeutuivat. Seuraava asema: " + after + ".");
            } else {
                finish(Outcome::Overshot, "Torkahdit. Siivooja lakaisi sinut ulos vaunusta asemalla " + after + ".");
                return;
            }
        }
    }

    // who wants a word: the gopniks once you come close, the others when they're ready
    for (auto& n : npcs) {
        if (n.kind == Kind::Gopnik && !n.talked && n.waitMs < 0 && near(n, 6)) {
            n.waitMs = 0;
            break;
        }
    }
    moveNpcs(ms);
    if (phase == Phase::Over) return;

    for (size_t i = 0; i < npcs.size(); ++i) {
        Npc& n = npcs[i];
        if (n.gone || n.talked || n.stunMs > 0 || n.angryMs > 0 || n.chasing || n.waitMs < 0 || t < n.waitMs) continue;
        if (!near(n, 1)) continue;
        switch (n.kind) {
            case Kind::Gopnik: openPrompt((int)i, 1); n.talked = true; return;
            case Kind::Dealer: openPrompt((int)i, 2); n.talked = true; return;
            case Kind::Drunk:  openPrompt((int)i, 3); n.talked = true; return;
            case Kind::Cop:
                n.talked = true;
                if (dodged) { openPrompt((int)i, 4); return; }
                if (player->stats.promilles >= 25) { openPrompt((int)i, 5); return; }
                say("Kyttä vilkaisee lippuasi. «Проходи.»");
                break;
            case Kind::Fsb:
                n.talked = true;
                openPrompt((int)i, n.refusals >= 2 ? 7 : 6);
                return;
            case Kind::Pickpocket:
                n.nearMs += ms;
                if (n.nearMs >= 900) {
                    n.talked = true;
                    const uint32_t take = money() == 0 ? 0 : std::max<uint32_t>(1, money() * (20 + rnd(30)) / 100);
                    pay(take);
                    n.stolen = take;
                    if (take > 0 && chance(50)) say("Joku hipaisi taskuasi... Lompakko tuntuu kevyemmältä!");
                }
                break;
            default: break;
        }
    }
}

// ---------------------------------------------------------------- the others

bool MetroGame::npcStep(Npc& n, int dx, int dy)
{
    const int nx = n.x + dx, ny = n.y + dy;
    if ((dx == 0 && dy == 0) || !walkable(nx, ny)) return false;
    if (nx == px && ny == py) return false;
    if (npcAt(nx, ny) >= 0 || trainAt(nx, ny)) return false;
    const uint8_t tile = tiles[ny][nx];
    if (n.kind != Kind::Rat && phase == Phase::Platform && (tile == TRail || tile == TSleeper)) return false;
    if (n.kind == Kind::Rat && (tile == TExit || tile == TLive)) return false;
    n.x = nx;
    n.y = ny;
    return true;
}

void MetroGame::npcTowards(Npc& n, int tx, int ty)
{
    const int dx = sgn(tx - n.x), dy = sgn(ty - n.y);
    const bool xFirst = std::abs(tx - n.x) >= std::abs(ty - n.y);
    if (xFirst ? npcStep(n, dx, 0) : npcStep(n, 0, dy)) return;
    if (xFirst ? npcStep(n, 0, dy) : npcStep(n, dx, 0)) return;
    // round whoever is in the way
    if (dx == 0) npcStep(n, chance(50) ? 1 : -1, 0);
    else if (dy == 0) npcStep(n, 0, chance(50) ? 1 : -1);
}

void MetroGame::moveNpcs(int ms)
{
    for (size_t i = 0; i < npcs.size() && phase != Phase::Over; ++i) {
        Npc& n = npcs[i];
        if (n.gone) continue;
        n.cooldown -= ms;
        if (n.stunMs > 0 && (n.stunMs -= ms) <= 0) {
            n.stunMs = 0;
            if (n.glyph == "_") n.glyph = "J";
        }
        if (n.angryMs > 0) n.angryMs = std::max(0, n.angryMs - ms);
        // a cop on your heels runs out of breath in the end
        if (n.chasing && n.chaseMs > 0 && (n.chaseMs -= ms) <= 0) {
            n.chasing = false;
            n.talked = true;
            say("Kyttä luovuttaa, puuskuttaa ja kiroilee. Hän ei jaksa enempää.");
        }
        if (n.seated || n.stunMs > 0 || n.cooldown > 0) continue;

        if (n.chasing) {
            n.cooldown = 230;
            if (near(n, 1)) {
                finish(Outcome::Arrested, phase == Phase::Platform
                    ? "Kyttä kaatoi sinut laiturille. «Документы!» Lyönneistä tulee tuomio."
                    : "Kyttä sai sinut kiinni vaunussa. Rautakourat napsahtavat.");
                return;
            }
            npcTowards(n, px, py);
            continue;
        }
        // angry: comes at you and hits
        if (n.angryMs > 0) {
            if (near(n, 1)) {
                n.cooldown = 800;
                const uint32_t dmg = n.kind == Kind::Hothead ? 3 : n.kind == Kind::Drunk ? 1 : 2;
                say(capitalized(kindName(n.kind)) + " lyö sinua!");
                hurt(dmg, n.kind == Kind::Hothead ? "Äkkipikainen hakkasi sinut hengiltä metrossa. Matkustajat katsoivat puhelimiaan."
                        : n.kind == Kind::Gopnik ? "Jengiläiset potkivat sinut hengiltä metrovaunun lattialla."
                        : "Sinut hakattiin hengiltä metrossa.");
            } else {
                n.cooldown = 280;
                npcTowards(n, px, py);
            }
            continue;
        }

        switch (phase) {
            case Phase::Tunnel: {
                // rats: scurry about, and bite whatever stands still
                n.cooldown = 250 + rnd(300);
                if (near(n, 1) && chance(25)) {
                    n.cooldown = 1200;
                    say("Rotta puree nilkkaan!");
                    hurt(1, "Rotat söivät sinut metrotunnelissa. Ne olivat isoja rottia.");
                    break;
                }
                static const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
                const int* d = dirs[rnd(4)];
                npcStep(n, d[0], d[1]);
                break;
            }
            case Phase::Platform: {
                if (n.exiting) {
                    n.cooldown = 220 + rnd(150);
                    if (!npcStep(n, 0, 1)) npcStep(n, chance(50) ? 1 : -1, 0);
                    if (n.y >= P_BOT - 4) n.exiting = false;
                } else if (n.boarding && (stop == Stop::Boarding || stop == Stop::Closing)) {
                    n.cooldown = 180 + rnd(220);
                    const int door = stationDoorX(n.x);
                    if (n.y == P_EDGE && n.x == door) { n.gone = true; break; }
                    if (n.x != door && n.y <= P_EDGE + 1) npcTowards(n, door, n.y);
                    else npcTowards(n, door, P_EDGE);
                } else {
                    n.cooldown = 450 + rnd(700);
                    if (n.kind == Kind::Cop) break;
                    if (stop == Stop::Arriving && chance(40)) npcStep(n, 0, -1);
                    else if (chance(25)) {
                        static const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
                        const int* d = dirs[rnd(4)];
                        npcStep(n, d[0], d[1]);
                    }
                }
                break;
            }
            case Phase::Ride: {
                if (n.kind == Kind::Pickpocket && n.talked) {
                    n.cooldown = 300;   // slips away to the far end
                    npcStep(n, n.x < px ? -1 : 1, 0) || npcStep(n, 0, chance(50) ? 1 : -1);
                    break;
                }
                const bool approaching = !n.talked && n.waitMs >= 0 && t >= n.waitMs;
                if (approaching) {
                    n.cooldown = n.kind == Kind::Fsb ? 550 : n.kind == Kind::Cop ? 420 : 350;
                    if (!near(n, 1)) npcTowards(n, px, py);
                    break;
                }
                if (n.kind == Kind::Hothead) {
                    // stand too close for too long and he takes it personally
                    n.cooldown = 200;
                    if (near(n, 2)) n.nearMs += 200; else n.nearMs = 0;
                    if (n.nearMs >= 1800) {
                        n.nearMs = 0;
                        n.angryMs = 10000;
                        say("«Чё уставился?!» Äkkipikainen käy kuumana.");
                    }
                    // he wanders, but not while he is glaring at you
                    if (n.nearMs == 0 && n.angryMs == 0 && chance(10)) npcStep(n, chance(50) ? 1 : -1, 0);
                    break;
                }
                n.cooldown = 700 + rnd(900);
                if (chance(15)) {
                    static const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
                    const int* d = dirs[rnd(4)];
                    npcStep(n, d[0], d[1]);
                }
                break;
            }
            default: break;
        }
    }
}

// ---------------------------------------------------------------- drawing

string MetroGame::title() const
{
    string part;
    switch (phase) {
        case Phase::Tunnel:   part = "HUOLTOTUNNELI"; break;
        case Phase::Platform: part = "LAITURI"; break;
        case Phase::Ride:     part = "VAUNU"; break;
        case Phase::Over:     part = "MATKAN LOPPU"; break;
    }
    return "METRO · " + part + " → " + (phase == Phase::Ride && overshot ? after : to);
}

Rows MetroGame::render() const
{
    Rows rows(H, vector<Cell>(W, Cell(" ", "")));
    auto put = [&](int x, int y, const string& g, const string& c) {
        if (x >= 0 && y >= 0 && x < W && y < H) rows[y][x] = Cell(g, c);
    };
    auto text = [&](int x, int y, const string& s, const string& c) {
        for (const auto& g : glyphs(s)) put(x++, y, g, c);
    };
    const bool blink = (t / 160) % 2 == 0;
    const string me = player->typeToChar() == "x" ? "@" : player->typeToChar();
    const string meColor = stunMs > 0 ? (blink ? "1;93" : "1;97") : "1;38;5;180";

    auto box = [&](int top, int bottom) {
        put(1, top, "┌", "38;5;240");
        put(W - 2, top, "┐", "38;5;240");
        put(1, bottom, "└", "38;5;240");
        put(W - 2, bottom, "┘", "38;5;240");
        for (int x = 2; x < W - 2; ++x) { put(x, top, "─", "38;5;240"); put(x, bottom, "─", "38;5;240"); }
        for (int y = top + 1; y < bottom; ++y) { put(1, y, "│", "38;5;240"); put(W - 2, y, "│", "38;5;240"); }
    };

    if (phase == Phase::Over) {
        string head, color;
        switch (outcome) {
            case Outcome::Arrived:    head = "PERILLÄ · " + to; color = "1;92"; break;
            case Outcome::Overshot:   head = "AJOIT OHI · " + after; color = "1;93"; break;
            case Outcome::TurnedBack: head = "TAKAISIN KADULLE"; color = "1;97"; break;
            case Outcome::Arrested:   head = "PIDÄTETTY"; color = "1;94"; break;
            case Outcome::Recruited:  head = "VÄRVÄTTY · RINTAMALLE"; color = "1;91"; break;
            case Outcome::Dead:       head = "KUOLIT METROSSA"; color = "1;97;41"; break;
            default:                  head = "METRO"; color = "1;97"; break;
        }
        box(6, 20);
        text((W - (int)glyphs(head).size()) / 2, 8, head, color);
        int y = 11;
        for (const auto& line : wrap(cause, 70)) text(5, y++, line, "97");
        ++y;
        const int lost = (int)startMoney - (int)player->stats.money;
        text(5, ++y, "Rahaa " + string(lost >= 0 ? "meni " : "tuli ") + std::to_string(std::abs(lost)) + " ₽", "38;5;250");
        text(32, y, "Lyöntejä: " + std::to_string(hits), "38;5;250");
        text(52, y, "Voima: " + std::to_string(player->stats.health), "38;5;250");
        if (t > 800) text(26, 19, "Paina mitä tahansa näppäintä", "2;37");
        return rows;
    }

    if (phase == Phase::Tunnel) {
        text(2, 1, "HUOLTOTUNNELI · " + from, "1;97");
        text(2, 2, "Pääse tunnelin päähän ▶ laiturille. Junat kulkevat molempiin suuntiin.", "38;5;250");
        auto lit = [&](int x, int y) {
            if (std::abs(x - px) <= 13 && std::abs(y - py) <= 6) return true;
            if (y <= T_TRACKA && std::abs(x % 16 - 8) <= 3) return true;    // under the lamps
            if (y >= T_TRACKB + 2 && std::abs((x + 8) % 16 - 8) <= 3) return true;
            return false;
        };
        for (int y = T_WALLN; y <= T_WALLS; ++y) {
            for (int x = 0; x < W; ++x) {
                string g = " ", c;
                switch (tiles[y][x]) {
                    case TWall:
                        if (y == T_WALLN && x % 16 == 8) { g = "◦"; c = "1;93"; }
                        else if (y == T_WALLS && x % 16 == 0) { g = "◦"; c = "1;93"; }
                        else { g = "█"; c = "38;5;236"; }
                        break;
                    case TWalk:    g = "·"; c = "38;5;244"; break;
                    case TLive:    g = "≡"; c = "1;33"; break;
                    case TCover:   g = "▬"; c = "38;5;250"; break;
                    case TRail:    g = "═"; c = "38;5;246"; break;
                    case TSleeper: g = "╪"; c = "38;5;94"; break;
                    case TGravel:  g = "∙"; c = "38;5;240"; break;
                    case TPillar:  g = "▓"; c = "38;5;245"; break;
                    case TJunk:    g = "#"; c = "38;5;130"; break;
                    case TExit:    g = "▶"; c = "1;92"; break;
                    default: break;
                }
                // singing rails: the track ahead of a coming train glints
                const int track = y >= T_TRACKA && y <= T_TRACKA + 2 ? 0 : y >= T_TRACKB && y <= T_TRACKB + 2 ? 1 : -1;
                if (track >= 0 && trains[track].warned && !trains[track].active) {
                    const bool fromRight = trains[track].dir < 0;
                    const int edge = fromRight ? W - 1 - x : x;
                    if (edge < 8) { g = "░"; c = blink ? "1;93" : "33"; }
                    else if (tiles[y][x] == TRail && blink) c = "33";
                }
                if (!lit(x, y) && tiles[y][x] != TExit && !(tiles[y][x] == TWall && g == "◦")) {
                    c = tiles[y][x] == TLive ? "38;5;58" : "38;5;235";
                }
                put(x, y, g, c);
            }
        }
        for (int k = 0; k < 2; ++k) {
            const Train& tr = trains[k];
            if (!tr.active) continue;
            const int top = k == 0 ? T_TRACKA : T_TRACKB;
            const int x0 = (int)std::floor(tr.x);
            for (int i = 0; i < tr.len; ++i) {
                const int x = x0 + i;
                const bool front = tr.dir < 0 ? i == 0 : i == tr.len - 1;
                put(x, top, front ? "●" : "▄", front ? "1;93" : "38;5;25");
                put(x, top + 1, front ? "█" : (i % 4 == 1 || i % 4 == 2) ? "▒" : "█", front ? "38;5;25" : (i % 4 == 1 || i % 4 == 2) ? "1;93;44" : "38;5;25");
                put(x, top + 2, front ? "●" : "▀", front ? "1;93" : "38;5;25");
            }
        }
        for (const auto& n : npcs) if (!n.gone && lit(n.x, n.y)) put(n.x, n.y, n.glyph, n.color);
        put(px, py, me, meColor);

        text(2, 20, "@ sinä   ≡ virtakisko - tappaa   ▬ suojalevy - siitä voi ylittää   # romua", "38;5;250");
        text(2, 21, "═╪═ raide - junat jyräävät   ░ junan valot tulossa   r rotta - puree", "38;5;250");
        text(2, 22, "▓ pilari   ∙ raiteiden väli on turvassa   ◦ lamppu", "38;5;250");
        text(2, 24, "WASD liiku · vasemmasta reunasta takaisin kadulle (lippu menee hukkaan)", "2;37");
        if (player->stats.promilles >= 12) text(2, 25, "Olet humalassa: askel horjuu sivulle. Älä kävele virtakiskon vieressä.", "1;91");
        return rows;
    }

    if (phase == Phase::Platform) {
        text(2, 0, "ASEMA " + from, "1;97");
        for (int x = 0; x < W; ++x) {
            put(x, 1, x % 10 == 5 ? "✱" : " ", "1;93");   // chandeliers
            put(x, P_WALL, "▓", (x / 2) % 3 == 0 ? "38;5;136" : (x / 2) % 3 == 1 ? "38;5;94" : "38;5;179");
            put(x, P_TRACK, "═", "38;5;240");
            put(x, P_TRACK + 1, "╪", "38;5;238");
            put(x, P_TRACK + 2, "═", "38;5;240");
            put(x, P_EDGE, "═", "1;33");
            for (int y = P_TOP; y <= P_BOT; ++y) put(x, y, tiles[y][x] == TPillar ? "█" : "·", tiles[y][x] == TPillar ? "38;5;252" : "38;5;238");
            put(x, P_EXIT, "▀", "38;5;240");
        }
        text(36, P_EXIT, " ↑ ВЫХОД ", "1;97;44");
        if (stop == Stop::Waiting && trains[0].warned) {
            for (int y = P_TRACK; y <= P_TRACK + 2; ++y)
                for (int x = 0; x < 6; ++x) put(x, y, "░", blink ? "1;93" : "33");
        }
        if (trains[0].active) {
            const int x0 = trainX();
            const bool open = doorsOpen();
            for (int c = 0; c < CARS; ++c) {
                for (int i = 0; i < CAR_LEN; ++i) {
                    const int x = x0 + c * CAR_STEP + i;
                    const bool front = c == CARS - 1 && i == CAR_LEN - 1;
                    const bool door = i == 6 || i == 7 || i == 16 || i == 17;
                    put(x, P_TRACK, front ? "●" : "▄", front ? "1;93" : "38;5;25");
                    put(x, P_TRACK + 1, (i % 4 == 1 || i % 4 == 2) && !door ? "▒" : "█",
                        (i % 4 == 1 || i % 4 == 2) && !door ? "1;93;44" : "38;5;25");
                    if (door) put(x, P_TRACK + 2, open ? "▯" : "▐", open ? "1;97;42" : "38;5;250");
                    else put(x, P_TRACK + 2, front ? "●" : "▀", front ? "1;93" : "38;5;25");
                }
            }
        }
        for (const auto& n : npcs) {
            if (n.gone) continue;
            const string c = n.chasing ? "1;91" : n.angryMs > 0 ? "1;91" : n.color;
            put(n.x, n.y, n.glyph, c);
        }
        put(px, py, me, meColor);

        string state;
        switch (stop) {
            case Stop::Waiting:   state = trains[0].warned ? "Juna tulee!" : "Odotetaan junaa..."; break;
            case Stop::Arriving:  state = "Juna saapuu - pysy pois reunalta!"; break;
            case Stop::Unloading: state = "Ovet auki, matkustajat tulevat ulos."; break;
            case Stop::Boarding:  state = "Ovet auki! Mene ▯ ovesta sisään: " + std::to_string((8000 - stopMs) / 1000 + 2) + " s"; break;
            case Stop::Closing:   state = "Ovet sulkeutuvat!"; break;
            case Stop::Leaving:   state = "Juna lähtee."; break;
        }
        text(2, 23, state, stop == Stop::Boarding || stop == Stop::Closing ? "1;92" : "1;97");
        text(2, 24, "Kävely päin ihmistä on lyönti. Lyöntejä: " + std::to_string(hits) + "   C kyttä on laiturilla.", "38;5;250");
        text(2, 25, "Keltainen reuna ═: kun juna tulee, tungos voi tönäistä raiteille.", "38;5;250");
        if (missed > 0) text(2, 26, "Junia mennyt ohi: " + std::to_string(missed), "2;37");
        return rows;
    }

    // the ride
    {
        const int total = overshot ? 16000 : RIDE_MS;
        const int left = std::max(0, rideLeft);
        text(2, 0, "Seuraava asema: " + (overshot ? after : to), "1;97");
        const int bar = 30;
        const int done = doorsMs > 0 ? bar : bar - left * bar / total;
        for (int i = 0; i < bar; ++i) put(40 + i, 0, i < done ? "█" : "░", i < done ? "1;34" : "38;5;238");
        text(72, 0, doorsMs > 0 ? "ASEMA" : std::to_string(left / 1000 + 1) + " s", "1;97");

        const bool atStation = doorsMs > 0;
        for (int x = 0; x < W; ++x) {
            for (int y : { 2, 3, 4, 16, 17, 18 }) {
                if (atStation) put(x, y, "▓", (x / 2 + y) % 3 == 0 ? "38;5;136" : "38;5;94");
                else {
                    const bool lamp = (x + t / 25) % 23 == 0 && (y == 3 || y == 17);
                    const bool cable = y == 2 || y == 18;
                    put(x, y, lamp ? "•" : cable ? "─" : " ", lamp ? "1;93" : "38;5;237");
                }
            }
        }
        for (int y = R_WALLT; y <= R_WALLB; ++y) {
            for (int x = 0; x < W; ++x) {
                switch (tiles[y][x]) {
                    case TCarWall:
                        put(x, y, (y == R_WALLT || y == R_WALLB) && x % 6 >= 2 && x % 6 <= 3 && x > 0 && x < W - 1 ? "▒" : "█",
                            (y == R_WALLT || y == R_WALLB) && x % 6 >= 2 && x % 6 <= 3 ? "38;5;67" : "38;5;24");
                        break;
                    case TDoor:  put(x, y, atStation ? "▯" : "▐", atStation ? "1;97;42" : "38;5;250"); break;
                    case TSeat:  put(x, y, "▄", "38;5;94"); break;
                    case TPole:  put(x, y, "│", "1;37"); break;
                    case TFloor: put(x, y, "·", "38;5;237"); break;
                    default: break;
                }
            }
        }
        for (const auto& n : npcs) {
            if (n.gone) continue;
            put(n.x, n.y, n.glyph, n.chasing || n.angryMs > 0 ? "1;91" : n.color);
        }
        put(px, py, me, meColor);

        text(1, 20, "J juoppo  g jengiläinen  D diileri  O äkkipikainen  C kyttä  F FSB  v varas", "38;5;250");
        text(1, 21, "│ tanko: pysy vieressä kun juna nytkähtää   ▯ ovi aukeaa asemalla: ulos siitä", "38;5;250");

        if (prompt.id) {
            box(22, 27);
            const auto lines = wrap(prompt.text, 72);
            int y = 23;
            for (size_t i = 0; i < lines.size() && i < 2; ++i) text(3, y++, lines[i], "1;97");
            int x = 3;
            for (size_t i = 0; i < prompt.options.size(); ++i) {
                const string o = "[" + std::to_string(i + 1) + "] " + prompt.options[i];
                text(x, 26, o, "1;93");
                x += (int)glyphs(o).size() + 4;
            }
        } else {
            text(1, 23, "WASD liiku · kävely päin ihmistä on lyönti · älä tuijota äkkipikaista", "2;37");
        }
    }
    return rows;
}

// ---------------------------------------------------------------- summary and the loop

string summarize(const MetroGame& g)
{
    string s;
    switch (g.outcome) {
        case Outcome::Arrived:    s = "Metro toi sinut asemalle " + g.to + "."; break;
        case Outcome::Overshot:   s = "Ajoit ohi: nousit pois asemalla " + g.after + "."; break;
        case Outcome::TurnedBack: s = "Palasit kadulle. Metro saa odottaa."; break;
        default:                  s = g.cause; break;
    }
    const int lost = (int)g.startMoney - (int)g.player->stats.money;
    if (lost > 0 && (g.outcome == Outcome::Arrived || g.outcome == Outcome::Overshot)) s += " Matka maksoi " + std::to_string(lost) + " ₽ ylimääräistä.";
    if (g.hits > 0) s += " Löit matkalla " + std::to_string(g.hits) + " ihmistä.";
    return s;
}

Result runMetroMinigame(PlayerNS::Player* player, Level* level, const string& from, const string& to,
                        const string& after, bool dodgedFare)
{
    MetroGame game(player, from, to, after, dodgedFare);
    Printer* printer = level->printer;
    Rows rows;
    printer->scene = &rows;

    InputNS::Input::raw();
    while (InputNS::Input::getRawCharNonBlocking() != 0) { }

    constexpr int FRAME_MS = 50;
    auto draw = [&]() {
        rows = game.render();
        printer->sceneTitle = game.title();
        printer->sceneShake = game.shake;
        printer->msg = game.msg;
        printer->msgTtl = Printer::MSG_FRAMES;
        printer->print(level);
    };
    while (!game.over()) {
        for (char k; (k = InputNS::Input::getRawCharNonBlocking()) != 0;) game.key(k);
        game.tick(FRAME_MS);
        draw();
        common::frameSleep(FRAME_MS);
    }

    // the end card stays until a key, or a few seconds
    printer->msg = "";
    for (int ms = 0; ms < 6000; ms += FRAME_MS) {
        bool key = false;
        for (char k; (k = InputNS::Input::getRawCharNonBlocking()) != 0;) key = true;
        if (key && ms > 800) break;
        game.tick(FRAME_MS);
        game.msg.clear();
        draw();
        common::frameSleep(FRAME_MS);
    }

    printer->scene = nullptr;
    printer->sceneShake = 0;
    printer->msg = "";
    printer->msgTtl = 0;

    Result r;
    r.outcome = game.outcome;
    r.cause = game.cause;
    r.summary = summarize(game);
    return r;
}

} // namespace MetroNS
