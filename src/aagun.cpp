#include "aagun.hpp"
#include "animation.hpp"
#include "building.hpp"
#include "common.hpp"
#include "events.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using PlayerNS::Player;
using std::string;

// ============ the gun ============

AAGun::AAGun(const Coord& left) : Vehicle(left, left.y)
{
    length = 3;
    horizontal = true;
    direction = -1;     // the body trails to the right: sandbag, gun, sandbag
}

std::string AAGun::glyphAt(uint32_t index) const
{
    if (index == 1) return wrecked ? "#" : "\xD0\x96";      // Ж: twin barrels on the mount
    return "\xE2\x96\x92";                                  // ▒ sandbags
}

const char* AAGun::colorAt(uint32_t index) const
{
    if (wrecked) return "38;5;240";
    if (index == 1) return gunner ? "1;38;5;148" : "1;38;5;64";
    return "38;5;180";      // sand
}

std::vector<ITMies*> AAGun::crew() const
{
    std::vector<ITMies*> out;
    if (!level) return out;
    for (auto p : level->persons) {
        ITMies* m = dynamic_cast<ITMies*>(p);
        if (m && m->gun == this && m->stats.health > 0) out.push_back(m);
    }
    return out;
}

std::vector<ITMies*> AAGun::awakeCrew() const
{
    std::vector<ITMies*> out;
    for (auto m : crew()) if (!m->asleep()) out.push_back(m);
    return out;
}

bool AAGun::approach(Player* player, string& msg)
{
    if (wrecked) {
        msg = "IT-tykki on savuava romukasa. Siit\xC3\xA4 ei en\xC3\xA4\xC3\xA4 ammuta.";
        return false;
    }
    if (gunner == player) return true;
    if (!common::realtimeEnabled) {
        msg = "Tykin kammet ovat j\xC3\xA4\xC3\xA4ss\xC3\xA4. (IT-tykki toimii vain reaaliaikatilassa, --rt.)";
        return false;
    }

    const auto awake = awakeCrew();
    const auto all = crew();
    if (!awake.empty()) {
        // a bottle each and the crew takes a break; without, they chase the drunk off
        if (player->inventory.vodka >= awake.size()) {
            player->inventory.vodka -= awake.size();
            for (auto m : awake) {
                m->passedOut = true;
                m->say("Vodka? ...Davai. Ammu vaikka Kuuta, min\xC3\xA4 pid\xC3\xA4n tauon.");
            }
            msg = awake.size() == 1
                ? "Ojennat ampujalle pullon vodkaa. H\xC3\xA4n kulauttaa sen kerralla ja sammuu hiekkas\xC3\xA4kkien viereen. Tykki on sinun!"
                : "Jokainen saa oman pullonsa. Miehist\xC3\xB6 juo ja sammuu rivissä. Tykki on sinun!";
        } else {
            awake.front()->say(player->inventory.vodka == 0
                ? "H\xC3\xA4ivy, juoppo! Sotilasalue! ...Ellei sinulla sitten ole pulloa?"
                : "Meit\xC3\xA4 on useampi, juoppo. Pullo jokaiselle tai h\xC3\xA4ivy!");
            msg = "Miehist\xC3\xB6 t\xC3\xB6n\xC3\xA4isee sinut kauemmas tykist\xC3\xA4.";
            player->damage(1);
            return false;
        }
    } else if (!all.empty()) {
        msg = "Ampuja kuorsaa hiekkas\xC3\xA4kkien v\xC3\xA4liss\xC3\xA4. Istut tykin taakse, eik\xC3\xA4 kukaan est\xC3\xA4!";
    } else {
        msg = "Miehist\xC3\xB6 on poissa. Kaappaat IT-tykin! Sotilaat eiv\xC3\xA4t ilahdu.";
        if (level) level->alertCops(player, 20);
    }
    gunner = player;
    player->manning = this;
    msg += " (A/D k\xC3\xA4\xC3\xA4nt\xC3\xA4\xC3\xA4, v\xC3\xA4lily\xC3\xB6nti ampuu, Esc poistuu. Ammuksia " + std::to_string(ammo) + ".)";
    return true;
}

void AAGun::leave()
{
    if (gunner && gunner->manning == this) gunner->manning = nullptr;
    gunner = nullptr;
}

bool AAGun::fire(string& msg)
{
    if (wrecked) { msg = "Tykki on romuna."; return false; }
    if (ammo == 0) {
        msg = "*KLIK* Ammukset loppuivat. IT-tykki on nyt pelkk\xC3\xA4\xC3\xA4 rautaa.";
        return false;
    }
    --ammo;
    const Coord from = centre();
    level->audio.addSound(from, SoundType::Gunshot,
        "BUUM-BUUM! IT-tykki jyr\xC3\xA4ht\xC3\xA4\xC3\xA4!", "Ilmatorjuntatulta!", "...tykin jyrin\xC3\xA4\xC3\xA4 kaukaa...", 60);
    const ShellHit hit = traceShell(*level, from, angle, this);
    common::shellEvent(from, hit.at);
    const string boom = aaExplosion(*level, hit.at, gunner);
    msg = "BUUM! ";
    if (hit.drone) msg += "Osuma drooniin! ";
    else if (!hit.what.empty()) msg += "Osuma: " + hit.what + "! ";
    else msg += "Ammus r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4 ilmassa. ";
    if (!boom.empty()) msg += boom + " ";
    msg += "[" + std::to_string(ammo) + "/" + std::to_string(AMMO) + "]";
    return true;
}

// ============ the crew ============

ITMies::ITMies(const Coord& pos, AAGun* g) : ITYksikko(pos), gun(g)
{
    clothing = "38;5;64";
}

void ITMies::npcAct()
{
    if (passedOut) {
        if (common::random(4)) say("Zzz... *hik*");
        return;
    }
    if (sleepActs > 0) {
        if (--sleepActs == 0) {
            if (gun && gun->gunner) {
                say("HEI! Se on MINUN tykkini! Apua, varas tykill\xC3\xA4!");
                level->alertCops(gun->gunner, 25);
            } else {
                say("*haukotus* ...en nukkunut. Tarkkailin.");
            }
        } else if (common::random(6)) {
            say("Zzz...");
        }
        return;
    }
    if (!gun || gun->wrecked || gun->gunner) {
        if (gun && gun->gunner && common::random(5)) say("Tuo juoppo ampuu meid\xC3\xA4n tykill\xC3\xA4mme...");
        if (!gun || gun->wrecked) ITYksikko::npcAct();
        return;
    }
    // on duty, and on the bottle: now and then the gunner nods off at his post
    if (std::rand() % 1000 < 6) {
        doze(60 + std::rand() % 80);
        say("*hik* ...vain pienet nokkaunet...");
        return;
    }
    // a drone within reach: the crew opens up on it
    if (fireCooldown == 0) {
        const Coord c = gun->centre();
        for (size_t i = 0; i < level->drones.size(); ++i) {
            const Drone& d = level->drones[i];
            if (std::hypot(d.x - c.x, d.y - c.y) > 25.0f) continue;
            fireCooldown = 6 + std::rand() % 6;
            say("Kohde havaittu! OGON!");
            level->audio.addSound(c, SoundType::Gunshot,
                "BUUM BUUM! Ilmatorjunta ampuu droonia!", "Ilmatorjuntatulta!", "...jyrin\xC3\xA4\xC3\xA4 kaukaa...", 50);
            gun->angle = std::atan2(d.y - c.y, d.x - c.x);
            if (common::random(35)) {
                const Coord at { (uint32_t)std::max(0.0f, d.x), (uint32_t)std::max(0.0f, d.y) };
                level->drones.erase(level->drones.begin() + i);
                common::shellEvent(c, at);
                level->audio.addSound(at, SoundType::Explosion,
                    "PAM! Drooni putoaa liekeiss\xC3\xA4!", "R\xC3\xA4j\xC3\xA4hdys taivaalla!", "...r\xC3\xA4j\xC3\xA4hdys kaukaa...", 50);
                common::boomEvent(at, 2);
                if (level->events) level->events->addFiresAround(at);
            }
            return;
        }
    }
    ITYksikko::npcAct();
}

bool ITMies::interact(string& msg, Person* source)
{
    Player* player = dynamic_cast<Player*>(source);
    if (!player) return true;
    if (gun && !gun->wrecked) {
        gun->approach(player, msg);
        return true;
    }
    msg = asleep() ? "Ampuja kuorsaa. Tykki on romuna, ei h\xC3\xA4nt\xC3\xA4 kannata her\xC3\xA4tt\xC3\xA4\xC3\xA4." : "Ilmatorjuntamies tuijottaa romuttunutta tykki\xC3\xA4\xC3\xA4n.";
    return true;
}

// ============ shells ============

ShellHit traceShell(Level& level, const Coord& from, float angle, const Vehicle* self)
{
    // the same steps as the aim line, so the shell goes where the line shows
    const float dx = std::cos(angle), dy = std::sin(angle);
    float fx = (float)from.x, fy = (float)from.y;
    Coord last = from;
    for (uint32_t step = 1; step <= AAGun::RANGE; ++step) {
        fx += dx;
        fy += dy;
        const int ix = (int)(fx + 0.5f), iy = (int)(fy + 0.5f);
        if (ix < 0 || iy < 0 || ix >= (int)common::SIZEX || iy >= (int)common::SIZEY) break;
        const Coord c { (uint32_t)ix, (uint32_t)iy };
        last = c;
        if (level.hitWall(c)) {
            const Building* b = level.buildingAt(c);
            const string name = b ? b->getName() : string();
            return { c, name.empty() ? string("talon sein\xC3\xA4") : name + " (sein\xC3\xA4)" };
        }
        if (step < AAGun::ARMING) continue;
        for (const auto& d : level.drones) {
            if (std::hypot(d.x - fx, d.y - fy) < 1.6f) return { c, "drooni", true };
        }
        Vehicle* v = level.getVehicleAt(c);
        if (v && v != self) return { c, v->name() };
        Person* p = level.getPerson(c);
        if (p && p->stats.health > 0 && p->type != pelaaja) return { c, level.nameOf(p) };
    }
    return { last, "" };
}

std::string aaExplosion(Level& level, const Coord& at, Person* source)
{
    constexpr float RADIUS = 3.0f;
    string worst;
    common::boomEvent(at, 3);
    level.addAnimation(new Animation(Coord{ at.x > 5 ? at.x - 5 : 0, at.y }, "***BOOM***", 8));
    level.audio.addSound(at, SoundType::Explosion,
        "KRAAKABUUM! Ammus r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4!", "R\xC3\xA4j\xC3\xA4hdys!", "...r\xC3\xA4j\xC3\xA4hdys kaukaa...", 60);
    auto dist = [&](float x, float y) { return std::hypot(x - at.x, y - at.y); };

    // people: torn apart in the middle, knocked flat at the edge
    uint32_t killed = 0;
    for (auto p : level.persons) {
        if (p->stats.health == 0) continue;
        const float r = dist(p->coord.x, p->coord.y);
        if (r > RADIUS) continue;
        const uint32_t dmg = r < 0.5f ? 25 + std::rand() % 10 : r < 1.5f ? 12 + std::rand() % 8
                           : r < 2.5f ? 6 + std::rand() % 5 : 2 + std::rand() % 3;
        p->damage(dmg);
        if (p->stats.health == 0 && p->type != pelaaja) ++killed;
    }
    if (killed == 1) worst = "Yksi kuoli.";
    else if (killed > 1) worst = std::to_string(killed) + " kuoli.";

    // cars and guns: wrecked and burning
    for (auto v : level.vehicles) {
        if (v->wrecked) continue;
        bool near = false;
        for (const auto& c : v->cells()) near = near || dist(c.x, c.y) <= 2.0f;
        if (!near) continue;
        v->wrecked = true;
        if (AAGun* g = dynamic_cast<AAGun*>(v)) g->leave();
        for (const auto& c : v->cells()) {
            if (level.events) level.events->addFiresAround(c);
            level.addSmoke(c);
        }
        worst = v->name() + " r\xC3\xA4j\xC3\xA4hti liekkeihin! " + worst;
    }

    // walls blown open; what was hit catches fire
    std::vector<Building*> hit;
    for (int y = (int)at.y - 2; y <= (int)at.y + 2; ++y) {
        for (int x = (int)at.x - 2; x <= (int)at.x + 2; ++x) {
            if (x < 0 || y < 0 || x >= (int)common::SIZEX || y >= (int)common::SIZEY) continue;
            if (dist(x, y) > 2.0f) continue;
            const Coord c { (uint32_t)x, (uint32_t)y };
            Building* b = level.buildingAt(c);
            if (!b || !b->hitWall(c)) continue;
            if (std::find(hit.begin(), hit.end(), b) == hit.end()) hit.push_back(b);
            if (b->type == EKremlin) continue;      // five hundred years of brick
            if (!b->isBreach(c)) b->breaches.push_back(c);
        }
    }
    for (auto b : hit) {
        if (b->type == EKremlin) { worst += " Kremlin muuri kest\xC3\xA4\xC3\xA4. Toistaiseksi."; continue; }
        if (Jalostamo* j = dynamic_cast<Jalostamo*>(b)) { j->burning = 900; worst += " \xD0\x9D\xD0\x9F\xD0\x97 syttyi!"; continue; }
        if (level.events && common::random(40)) {
            level.events->setAblaze(b->wallLeft, b->wallTop, b->wallRight, b->wallBot);
            worst += " " + b->getName() + " syttyi palamaan!";
        } else {
            worst += " Sein\xC3\xA4\xC3\xA4n repesi aukko.";
        }
    }
    if (level.events) level.events->addFiresAround(at);
    for (int i = 0; i < 4; ++i) level.addSmoke(at);

    // drones caught in the burst fall out of the sky
    for (size_t i = 0; i < level.drones.size();) {
        if (dist(level.drones[i].x, level.drones[i].y) <= RADIUS) {
            level.drones.erase(level.drones.begin() + i);
            worst = "Drooni putoaa liekeiss\xC3\xA4! " + worst;
            continue;
        }
        ++i;
    }

    // shrapnel flies further, and hits whoever is in the way
    for (int k = 0; k < 10; ++k) {
        const float a = (std::rand() % 628) / 100.0f;
        const float sx = std::cos(a), sy = std::sin(a);
        float fx = at.x + 0.5f, fy = at.y + 0.5f;
        for (int step = 0; step < 7; ++step) {
            fx += sx; fy += sy;
            if (fx < 0 || fy < 0 || fx >= common::SIZEX || fy >= common::SIZEY) break;
            const Coord c { (uint32_t)fx, (uint32_t)fy };
            if (level.hitWall(c)) break;
            if (dist(c.x, c.y) <= RADIUS) continue;   // the blast already had them
            Person* p = level.getPerson(c);
            if (p && p->stats.health > 0) {
                common::setHitDirection(sx > 0.4f ? 1 : sx < -0.4f ? -1 : 0, sy > 0.4f ? 1 : sy < -0.4f ? -1 : 0);
                p->damage(1 + std::rand() % 3);
                break;
            }
        }
    }

    if (source) level.alertCops(source, 40);
    while (!worst.empty() && worst.front() == ' ') worst.erase(worst.begin());
    while (!worst.empty() && worst.back() == ' ') worst.pop_back();
    return worst;
}

// ============ placing the guns ============

static bool spotFree(Level& level, uint32_t x, uint32_t y)
{
    // the sandbags and a row below them for the crew, clear of buildings, roads, cars and people
    for (uint32_t yy = y; yy <= y + 1; ++yy) {
        for (uint32_t xx = x; xx <= x + 2; ++xx) {
            const Coord c { xx, yy };
            if (xx >= common::SIZEX || yy >= common::SIZEY) return false;
            if (level.hit(c) || level.getVehicleAt(c)) return false;
            const Road* h = nullptr; const Road* v = nullptr;
            level.roadsAt(c, h, v);
            if (h || v) return false;
        }
    }
    return true;
}

static AAGun* placeGun(Level& level, uint32_t x, uint32_t y)
{
    // search outwards from the wanted spot for open ground
    for (uint32_t r = 0; r < 12; ++r) {
        for (int dy = -(int)r; dy <= (int)r; ++dy) {
            for (int dx = -(int)r; dx <= (int)r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != (int)r) continue;
                const int gx = (int)x + dx, gy = (int)y + dy;
                if (gx < 1 || gy < 1) continue;
                if (!spotFree(level, gx, gy)) continue;
                AAGun* gun = new AAGun(Coord{ (uint32_t)gx, (uint32_t)gy });
                level.addVehicle(gun);
                const int crew = 1 + std::rand() % 2;
                for (int k = 0; k < crew; ++k) {
                    ITMies* m = new ITMies(Coord{ (uint32_t)gx + (k ? 2u : 0u), (uint32_t)gy + 1 }, gun);
                    m->printer = level.printer;
                    m->pathFinding = level.pathFinding;
                    m->setLevel(&level);
                    // some crews have been at the bottle since morning
                    if (common::random(30)) m->doze(40 + std::rand() % 120);
                    if (!level.addPerson(m)) delete m;
                }
                return gun;
            }
        }
    }
    return nullptr;
}

void populateAAGuns(Level& level, uint32_t moscowStage)
{
    if (level.isWarLevel) return;
    for (auto b : level.buildings) {
        if (b->type == EKremlin) {
            // west and east of the walls, and both sides of the gate
            placeGun(level, b->wallLeft > 8 ? b->wallLeft - 7 : 1, b->wallTop + 6);
            placeGun(level, b->wallRight + 5, b->wallTop + 6);
            const uint32_t gate = (b->wallLeft + b->wallRight) / 2;
            placeGun(level, gate - 12, b->wallBot + 4);
            placeGun(level, gate + 10, b->wallBot + 4);
        }
        // the refineries are drone magnets: further into the game they get a gun too
        if (b->type == ERefinery && moscowStage >= 4) {
            placeGun(level, b->wallRight + 4, b->wallTop);
        }
    }
}

// ============ the gunner's controls ============

void gunnerKey(char c, Player& player, Level& level, Printer& printer, uint32_t& msSinceShot)
{
    AAGun* gun = player.manning;
    if (!gun) return;
    switch (c) {
        case 'a': case 'j': gun->angle -= 0.12f; break;
        case 'd': case 'l': gun->angle += 0.12f; break;
        case 'w': case 'i': gun->angle -= 0.04f; break;     // fine adjustment
        case 's': case 'k': gun->angle += 0.04f; break;
        case ' ': {
            if (msSinceShot < AAGun::SHOT_MS) break;
            msSinceShot = 0;
            string msg;
            gun->fire(msg);
            printer.showMessage(msg, &level, false);
            if (!player.manning) printer.showMessage(msg + " Tykki on romuna, hypp\xC3\xA4\xC3\xA4t pois.", &level, false);
            break;
        }
        case 27: case 'e': case 'q':
            gun->leave();
            printer.showMessage("Nouset tykin takaa. Korvat soivat viel\xC3\xA4 pitk\xC3\xA4\xC3\xA4n.", &level, false);
            break;
        default: break;
    }
}
