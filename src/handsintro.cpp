#include "handsintro.hpp"
#include "common.hpp"
#include "input.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "printer.hpp"
#include "armart.hpp"
#include "subtitles.hpp"
#include <algorithm>

namespace HandsIntroNS {

using std::string;
using PlayerNS::INITIAL_HP;

namespace {

constexpr int W = (int)common::VIEWPORT_W, H = (int)common::VIEWPORT_H;
constexpr int STREET_Y = 14;            // the little street of the pick-up, damage and rifle parts
constexpr int WALK_FROM = 10, KALJA_X = 24, KIVI_X = 36, WALK_TO = 40;
constexpr uint32_t STEP_MS = 170;       // the demo drunk's pace
constexpr uint32_t HITS[] = { 1200, 3000, 4800 };   // into the damage part
constexpr uint32_t HEALTH_AFTER[] = { 7, 4, 1 };    // a sleeve stage each: torn, in rags, bloody
constexpr uint32_t AK_AT = 1500;        // into the rifle part

struct Cue { uint32_t at; const char* text; };
// the captions, on cue; each stays until the next one is due
const Cue CUES[] = {
    { 0,     "K\xC3\xA4""det ovat inventaariosi: vasen, oikea, tasku, vy\xC3\xB6 ja selk\xC3\xA4." },
    { 3600,  "Mit\xC3\xA4 ei voi kantaa, j\xC3\xA4\xC3\xA4 maahan. Omaisuus kuuluu kansalle." },
    { 7000,  "K\xC3\xA4vele tavaran p\xC3\xA4\xC3\xA4lle: se nousee vapaaseen k\xC3\xA4teen." },
    { 7000 + (KALJA_X - WALK_FROM) * STEP_MS, "Kalja oikeaan k\xC3\xA4teen. El\xC3\xA4m\xC3\xA4 hymyilee." },
    { 7000 + (KIVI_X - WALK_FROM) * STEP_MS,  "Kivi vasempaan. K\xC3\xA4""det t\xC3\xA4ynn\xC3\xA4: enemp\xC3\xA4\xC3\xA4 ei mahdu." },
    { 14000, "K\xC3\xA4sivarsi kertoo voimasi. Katso hihaa." },
    { 14000 + 1200, "Saat osuman: Politsija. Hiha repe\xC3\xA4\xC3\xA4." },
    { 14000 + 3000, "Toinen osuma. Hiha on riekaleina." },
    { 14000 + 4800, "Kolmas. Verta hihassa: loppu on l\xC3\xA4hell\xC3\xA4." },
    { 22000, "T\xC3\xA4llainen tekstitys kertoo ruudun alalaidassa, mit\xC3\xA4 juuri tapahtui." },
    { 24300, "Heit\xC3\xA4t: KIVI." },
    { 26300, "Miliisi. Hymyile: olet jo syyllinen." },
    { 29000, "Rintamalla saat AK:n. Mit\xC3\xA4 k\xC3\xA4siss\xC3\xA4 oli, putoaa maahan." },
    { 29000 + AK_AT, "AK olalla, molemmat k\xC3\xA4""det kiinni. Ladattu, toisin kuin lupaukset." },
    { 33000, "[FPV-droonien p\xC3\xB6rin\xC3\xA4\xC3\xA4 taivaalla. Joku niist\xC3\xA4 etsii sinua.]" },
    { 36000, "Siin\xC3\xA4 kaikki. Takaisin Moskovaan: suunnitelma etenee." },
};

struct Canvas {
    Rows rows;
    Canvas() : rows(H, std::vector<std::pair<string, string>>(W, std::make_pair(string(" "), string()))) { }
    // text from column x on row y, one cell per UTF-8 character
    void put(int x, int y, const string& s, const string& colour)
    {
        if (y < 0 || y >= H) return;
        for (size_t i = 0; i < s.size(); ) {
            size_t len = 1;
            const unsigned char c = s[i];
            if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
            if (x >= 0 && x < W) rows[y][x] = std::make_pair(s.substr(i, len), colour);
            ++x;
            i += len;
        }
    }
    void centre(int y, const string& s, const string& colour) { put((W - columns(s)) / 2, y, s, colour); }
    static int columns(const string& s)
    {
        int n = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
        return n;
    }
    // a typewriter: the first `shown` characters of s
    static string first(const string& s, int shown)
    {
        string out;
        int n = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            if (((unsigned char)s[i] & 0xC0) != 0x80 && n++ == shown) break;
            out += s[i];
        }
        return out;
    }
};

const char* CHAPTER_TITLES[] = {
    "1/5 \xC2\xB7 K\xC3\x84""DET OVAT INVENTAARIO",
    "2/5 \xC2\xB7 TAVARAN POIMIMINEN",
    "3/5 \xC2\xB7 K\xC3\x84SIVARSI ON TERVEYTESI",
    "4/5 \xC2\xB7 TEKSTITYKSET",
    "5/5 \xC2\xB7 RINTAMALLA: AK",
    "\xD0\x92\xD0\xA1\xD0\x81 \xD0\x98\xD0\x94\xD0\x81\xD0\xA2 \xD0\x9F\xD0\x9E \xD0\x9F\xD0\x9B\xD0\x90\xD0\x9D\xD0\xA3",
};

void street(Canvas& c)
{
    for (int x = 4; x < W - 4; ++x) {
        c.put(x, STREET_Y - 2, "\xE2\x96\x93", "38;5;240");
        c.put(x, STREET_Y + 2, "\xE2\x94\x80", "38;5;238");
    }
}

}

Show::Show()
{
    demo.printer = nullptr;
    apply();
    cue(0, 0);
}

Chapter Show::chapter() const
{
    int k = 0;
    for (int i = 0; i < 6; ++i) if (ms >= CHAPTER_START[i]) k = i;
    return (Chapter)k;
}

int Show::walkerX() const
{
    const uint32_t t = ms - CHAPTER_START[(int)Chapter::Pickup];
    return std::min(WALK_TO, WALK_FROM + (int)(t / STEP_MS));
}

void Show::tick(uint32_t dt)
{
    const uint32_t from = ms;
    ms = std::min(TOTAL_MS, ms + dt);
    apply();
    cue(from, ms);
}

void Show::skip()
{
    const int next = (int)chapter() + 1;
    ms = next < 6 ? CHAPTER_START[next] : TOTAL_MS;
    apply();
    if (next < 6) cue(ms - 1, ms);
}

void Show::cue(uint32_t from, uint32_t to)
{
    const size_t n = sizeof(CUES) / sizeof(CUES[0]);
    for (size_t i = 0; i < n; ++i) {
        const bool due = (from == to && CUES[i].at == to) || (CUES[i].at > from && CUES[i].at <= to);
        if (!due) continue;
        const uint32_t next = i + 1 < n ? CUES[i + 1].at : TOTAL_MS;
        Subtitles::say(CUES[i].text, "", 0, std::min<uint32_t>(next - CUES[i].at, 3800));
    }
}

void Show::apply()
{
    Inventory& inv = demo.inventory;
    inv.left = inv.right = inv.pocket = inv.belt = inv.back = Held();
    inv.hasBelt = false;
    inv.akRaised = false;
    demo.stats.health = INITIAL_HP;
    demo.usedCoat = false;
    demo.coat = Coat::Oma;

    const Chapter ch = chapter();
    const uint32_t t = ms - CHAPTER_START[(int)ch];
    switch (ch) {
        case Chapter::Inventory: {
            // each slot fills as its row is typed out
            if (t >= 600)  inv.left = Inventory::fresh(Thing::Lenkki);
            if (t >= 1300) inv.right = Inventory::fresh(Thing::Vodka);
            if (t >= 2000) inv.pocket = Inventory::fresh(Thing::Lipas);
            if (t >= 2700) { inv.hasBelt = true; inv.belt = Inventory::fresh(Thing::Pistooli); }
            if (t >= 3400) inv.back = Inventory::fresh(Thing::Rynkky);
            break;
        }
        case Chapter::Pickup:
            if (walkerX() >= KALJA_X) inv.right = Inventory::fresh(Thing::Kalja);
            if (walkerX() >= KIVI_X) inv.left = Inventory::fresh(Thing::Kivi);
            break;
        case Chapter::Damage:
            inv.right = Inventory::fresh(Thing::Kalja);
            inv.left = Inventory::fresh(Thing::Kivi);
            for (int i = 0; i < 3; ++i) if (t >= HITS[i]) demo.stats.health = HEALTH_AFTER[i];
            break;
        case Chapter::Captions:
            inv.right = Inventory::fresh(Thing::Kalja);
            if (t < 2300) inv.left = Inventory::fresh(Thing::Kivi);   // then thrown
            break;
        case Chapter::Rifle:
        case Chapter::End:
            if (ch == Chapter::Rifle && t < AK_AT) {
                inv.right = Inventory::fresh(Thing::Kalja);
                break;
            }
            inv.back = Inventory::fresh(Thing::Rynkky);
            inv.akRaised = true;
            break;
    }
}

string Show::title() const
{
    return "K\xC3\x84""DET \xC2\xB7 \xD0\xA0\xD0\xA3\xD0\x9A\xD0\x98";
}

string Show::message() const
{
    const string key(1, (char)(InputNS::HANDS_INTRO_KEY - 32));
    return "K\xC3\xA4sien esittely. Esc tai " + key + " sulkee, muu n\xC3\xA4pp\xC3\xA4in: seuraava osa.";
}

Rows Show::render() const
{
    Canvas c;
    const Chapter ch = chapter();
    const uint32_t t = ms - CHAPTER_START[(int)ch];
    const string title = string("  ") + CHAPTER_TITLES[(int)ch] + "  ";
    c.centre(1, title, "1;97;41");
    const string me = demo.typeToChar();
    const string white = "1;97", dim = "2;37", text = "37", yellow = "1;93";

    switch (ch) {
        case Chapter::Inventory: {
            c.centre(3, "K\xC3\xA4tesi n\xC3\xA4kyv\xC3\xA4t kartan sivulla. Niiss\xC3\xA4 on kaikki, mit\xC3\xA4 omistat.", text);
            static const char* slots[][2] = {
                { "VASEN K\xC3\x84SI", "yksi tavara: pullo, kivi, makkara..." },
                { "OIKEA K\xC3\x84SI", "yksi tavara lis\xC3\xA4\xC3\xA4" },
                { "TASKU",      "pieni tavara: lipas, kranaatti, veitsi" },
                { "VY\xC3\x96",        "makarov, jos sinulla on vy\xC3\xB6" },
                { "SELK\xC3\x84",      "rynn\xC3\xA4kk\xC3\xB6kiv\xC3\xA4\xC3\xA4ri, AK" },
            };
            for (int i = 0; i < 5; ++i) {
                const int start = 300 + i * 700;
                if ((int)t < start) break;
                const int shown = ((int)t - start) / 25;
                const int y = 6 + i * 2;
                c.put(10, y, Canvas::first(slots[i][0], shown), yellow);
                if (shown > 10) c.put(22, y, Canvas::first("\xE2\x94\x80\xE2\x94\x80\xE2\x96\xBA  " + string(slots[i][1]), shown - 10), white);
            }
            if (t >= 3600) c.centre(18, "Muuta ei mahdu. Kaikki muu j\xC3\xA4\xC3\xA4 maahan.", dim);
            if (t >= 4600) c.centre(20, "Q vasen k\xC3\xA4si, E oikea k\xC3\xA4si, F heitt\xC3\xA4\xC3\xA4, Tab avaa tavarat.", text);
            break;
        }
        case Chapter::Pickup: {
            c.centre(4, "K\xC3\xA4vele tavaran p\xC3\xA4\xC3\xA4lle: se nousee vapaaseen k\xC3\xA4teen.", text);
            c.centre(5, "Katso, miten k\xC3\xA4si sivulla tarttuu siihen.", dim);
            street(c);
            const int x = walkerX();
            if (x < KALJA_X) c.put(KALJA_X, STREET_Y, "%", white);
            if (x < KIVI_X) c.put(KIVI_X, STREET_Y, ".", white);
            c.put(x, STREET_Y, me, "1;38;5;180");
            if (x >= KALJA_X) c.put(KALJA_X - 3, STREET_Y + 4, "oikea k\xC3\xA4si: PIVO", yellow);
            if (x >= KIVI_X) c.put(KIVI_X - 3, STREET_Y + 5, "vasen k\xC3\xA4si: KIVI", yellow);
            if (x >= WALK_TO) c.centre(21, "Molemmat k\xC3\xA4""det t\xC3\xA4ynn\xC3\xA4. Seuraava tavara j\xC3\xA4\xC3\xA4 maahan.", dim);
            break;
        }
        case Chapter::Damage: {
            c.centre(4, "K\xC3\xA4sivarren kunto on terveytesi. Mit\xC3\xA4 repaleisempi hiha, sit\xC3\xA4 l\xC3\xA4hemp\xC3\xA4n\xC3\xA4 loppu.", text);
            street(c);
            c.put(30, STREET_Y, me, "1;38;5;180");
            c.put(32, STREET_Y, "C", "1;38;5;68");
            for (int i = 0; i < 3; ++i) {
                if (t >= HITS[i] && t < HITS[i] + 500) {
                    c.put(28, STREET_Y - 1, "*PAM*", "1;91");
                    c.put(26, STREET_Y + 1, "-" + std::to_string((i ? HEALTH_AFTER[i - 1] : INITIAL_HP) - HEALTH_AFTER[i]) + "hp", "1;91");
                }
            }
            const uint32_t hp = demo.stats.health;
            string bar;
            for (uint32_t k = 0; k < INITIAL_HP; ++k) bar += k < hp ? "\xE2\x96\x88" : "\xE2\x96\x91";
            c.put(22, 19, "VOIMA ", dim);
            c.put(28, 19, bar, hp >= 8 ? "1;92" : hp >= 5 ? "1;93" : "1;91");
            c.put(40, 19, std::to_string(hp) + "/" + std::to_string(INITIAL_HP), white);
            static const char* sleeves[] = { "ehj\xC3\xA4", "revennyt", "riekaleina", "verinen" };
            const int stage = sleeveStage(hp, demo.usedCoat);
            c.put(22, 21, "HIHA  ", dim);
            c.put(28, 21, sleeves[stage], stage == 3 ? "1;91" : white);
            break;
        }
        case Chapter::Captions: {
            c.centre(4, "Ruudun alalaitaan tulee lyhyt tekstitys, kuin elokuvassa:", text);
            c.centre(5, "mit\xC3\xA4 juuri tapahtui, kenet kohtasit, miksi joudut rintamalle.", text);
            c.centre(7, "Viestirivi kartan yl\xC3\xA4puolella kertoo yksityiskohdat.", dim);
            street(c);
            c.put(30, STREET_Y, me, "1;38;5;180");
            if (t >= 2300 && t < 3300) c.put(31 + (int)(t - 2300) / 120, STREET_Y, ".", white);   // the stone flies
            if (t >= 4300) c.put(52, STREET_Y, "C", "1;38;5;68");
            // an arrow down at the caption, which sits on the map's second-to-last row
            if ((t / 400) % 2 == 0) {
                c.centre(H - 4, "\xE2\x96\xBC", yellow);
                c.centre(H - 3, "\xE2\x96\xBC", yellow);
            }
            break;
        }
        case Chapter::Rifle:
        case Chapter::End: {
            if (ch == Chapter::End) {
                c.centre(10, "K\xC3\xA4""det ovat inventaario. Hiha on terveys.", white);
                c.centre(12, "Tekstitys kertoo, mit\xC3\xA4 tapahtui. Rintamalla saat AK:n.", white);
                c.centre(15, "\xD0\x92\xD1\x81\xD1\x91 \xD0\xB8\xD0\xB4\xD1\x91\xD1\x82 \xD0\xBF\xD0\xBE \xD0\xBF\xD0\xBB\xD0\xB0\xD0\xBD\xD1\x83. Kaikki menee suunnitelman mukaan.", dim);
                c.centre(H - 6, "Paina mit\xC3\xA4 tahansa n\xC3\xA4pp\xC3\xA4int\xC3\xA4.", yellow);
                break;
            }
            c.centre(4, "Rintamalle joutuessa saat ladatun AK:n olalle.", text);
            c.centre(5, "Se vie molemmat k\xC3\xA4""det: mit\xC3\xA4 niiss\xC3\xA4 oli, putoaa maahan.", dim);
            street(c);
            c.put(40, STREET_Y, me, "1;38;5;107");
            if (t >= AK_AT) {
                const int fall = std::min<int>(1, (int)(t - AK_AT) / 150);
                c.put(38, STREET_Y - 1 + fall, "%", white);   // the beer falls out of the hand
            }
            // FPV drones come in from the west
            if (t >= 3000) {
                const int dx = (int)(t - 3000) / 60;
                c.put(-6 + dx, 7, "-o-", "1;37");
                c.put(-20 + dx * 3 / 2, 9, "-o-", "1;37");
                if ((t / 150) % 2) c.put(-6 + dx + 1, 8, "'", "2;37");
            }
            break;
        }
    }
    const string key(1, (char)(InputNS::HANDS_INTRO_KEY - 32));
    c.centre(H - 1, "Esc / " + key + " sulkee \xC2\xB7 muu n\xC3\xA4pp\xC3\xA4in: seuraava osa", dim);
    return c.rows;
}

void run(Printer* printer, Level* level)
{
    Show show;
    PlayerNS::Player* real = printer->player;
    Rows rows;
    printer->scene = &rows;
    printer->player = &show.demo;
    const bool wasRaw = common::realtimeEnabled;
    InputNS::Input::raw();

    constexpr uint32_t FRAME_MS = 50;
    while (!show.over()) {
        // one key a frame, so each part gets drawn at least once
        const char k = InputNS::Input::getRawCharNonBlocking();
        if (k == 27 || k == 'q' || k == InputNS::HANDS_INTRO_KEY || k == InputNS::HANDS_INTRO_KEY - 32) break;
        if (k != 0) show.skip();
        else show.tick(FRAME_MS);
        Subtitles::tick(k ? 0 : FRAME_MS);
        if (show.over()) break;
        rows = show.render();
        printer->sceneTitle = show.title();
        printer->msg = show.message();
        printer->msgTtl = Printer::MSG_FRAMES;
        printer->print(level);
        common::frameSleep(FRAME_MS);
    }

    printer->scene = nullptr;
    printer->sceneTitle.clear();
    printer->player = real;
    printer->msg = "";
    printer->msgTtl = 0;
    Subtitles::reset();
    if (!wasRaw) InputNS::Input::noraw();
}

}
