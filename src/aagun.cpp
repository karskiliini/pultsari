#include "aagun.hpp"
#include "animation.hpp"
#include "bradley.hpp"
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

AAGun::AAGun(const Coord& left, Model m) : Vehicle(left, left.y), model(m)
{
    ammo = maxAmmo();
    length = 3;
    horizontal = true;
    direction = -1;
}

// cells: the 3x3 core row by row (index 4 is the centre), then the sandbags
std::vector<Coord> AAGun::cells() const
{
    std::vector<Coord> out;
    const Coord c = centre();
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            const int x = (int)c.x + dx, y = (int)c.y + dy;
            if (x >= 0 && y >= 0) out.push_back(Coord{ (uint32_t)x, (uint32_t)y });
        }
    out.insert(out.end(), sandbags.begin(), sandbags.end());
    return out;
}

bool AAGun::isCore(const Coord& c) const
{
    const Coord m = centre();
    return std::abs((int)c.x - (int)m.x) <= 1 && std::abs((int)c.y - (int)m.y) <= 1;
}

void AAGun::laySandbags(const Level& l)
{
    sandbags.clear();
    const Coord m = centre();
    for (int i = 0; i < 40; ++i) {
        const float a = i / 40.0f * 6.2831853f;
        // the gap toward the west: the way in from the trench
        if (std::fabs(std::remainder(a - 3.1415927f, 6.2831853f)) < 0.5f) continue;
        const int x = (int)std::lround(m.x + std::cos(a) * RING_X), y = (int)std::lround(m.y + std::sin(a) * RING_Y);
        if (x < 0 || y < 0 || x >= (int)common::SIZEX || y >= (int)common::SIZEY) continue;
        const Coord c { (uint32_t)x, (uint32_t)y };
        if (isCore(c) || std::find(sandbags.begin(), sandbags.end(), c) != sandbags.end()) continue;
        const Trench t = l.trenchAt(c);
        if (l.hit(c) || l.getVehicleAt(c) || t == Trench::Own || t == Trench::Enemy) continue;
        const Road* h = nullptr; const Road* v = nullptr;
        l.roadsAt(c, h, v);
        if (h || v) continue;
        sandbags.push_back(c);
    }
}

std::string AAGun::glyphAt(uint32_t index) const
{
    if (index < 9) {
        if (wrecked) return index == 4 ? "#" : "\xE2\x96\x93";   // a twisted heap
        if (index != 4) return "\xE2\x96\x93";                    // ▓ the mount (the sprite draws over it)
        switch (model) {
            case Model::Zu23:     return "\xD0\x93";   // Г: one barrel
            case Model::Vierling: return "\xD0\xA8";   // Ш: four barrels
            default:              return "\xD0\x96";   // Ж: twin barrels on the mount
        }
    }
    return "\xE2\x96\x92";                                  // ▒ sandbags
}

const char* AAGun::colorAt(uint32_t index) const
{
    if (index < 9) return wrecked ? "38;5;240" : index == 4 ? "1;38;5;228;48;5;58" : "38;5;64;48;5;58";
    return "1;38;5;180;48;5;94";      // sand, on its own shadow
}

void AAGun::frame(uint32_t ms)
{
    // the mount swings round 12 degrees a frame until it points where the gunner aims
    const float d = std::remainder(angle - shown, 6.2831853f);
    shown = std::fabs(d) <= TURN_STEP ? angle : shown + (d > 0 ? TURN_STEP : -TURN_STEP);
    for (int k = 0; k < 4; ++k) {
        if (flashDelay[k] > 0) { flashDelay[k] -= (int32_t)ms; continue; }
        if (flashMs[k] > 0) flashMs[k] -= (int32_t)ms;
    }
}

uint32_t AAGun::barrels() const
{
    return model == Model::Zu23 ? 1 : model == Model::Sergei ? 2 : 4;
}

float AAGun::barrelOffset(uint32_t k) const
{
    // the Flakvierling: two pairs with the gunner's seat between them
    static const float sergei[2] = { -0.5f, 0.5f };
    static const float vierling[4] = { -1.6f, -1.0f, 1.0f, 1.6f };
    if (model == Model::Sergei) return sergei[k % 2];
    if (model == Model::Vierling) return vierling[k % 4];
    return 0.0f;
}

float AAGun::barrelLength() const
{
    return model == Model::Vierling ? 3.0f : 4.0f;
}

void AAGun::barrelTip(uint32_t k, float& x, float& y, float at) const
{
    const Coord m = centre();
    const float o = barrelOffset(k), r = barrelLength();
    // a barrel in recoil is half a cell shorter
    const float len = flashing(k) ? r - 0.5f : r;
    x = m.x + len * std::cos(at) - o * std::sin(at);
    y = m.y + len * std::sin(at) + o * std::cos(at);
}

void AAGun::flashBarrel(uint32_t k, uint32_t delayMs)
{
    if (k >= 4) return;
    flashDelay[k] = (int32_t)delayMs;
    flashMs[k] = (int32_t)FLASH_MS;
}

// a line glyph for a direction (cell space), nearest of the four; doubled for the Sergei
static const char* lineGlyph(float a, bool dbl)
{
    const int d = (((int)std::lround(a / 0.78539816f)) % 4 + 4) % 4;
    static const char* single[4] = { "\xE2\x94\x80", "\\", "\xE2\x94\x82", "/" };              // ─ \ │ /
    static const char* twin[4] = { "\xE2\x95\x90", "\xE2\x95\xB2", "\xE2\x95\x91", "\xE2\x95\xB1" }; // ═ ╲ ║ ╱
    return dbl ? twin[d] : single[d];
}

std::vector<AAGun::Glyph> AAGun::sprite() const
{
    std::vector<Glyph> out;
    const Coord m = centre();
    // the screen shows it in 12-degree steps
    const float a = std::lround(shown / TURN_STEP) * TURN_STEP;
    auto put = [&](float x, float y, const std::string& g, const char* color) {
        const long cx = std::lround(x), cy = std::lround(y);
        if (cx < 0 || cy < 0) return;
        for (auto& o : out) if (o.x == (int)cx && o.y == (int)cy) { o.g = g; o.color = color; return; }
        out.push_back(Glyph{ (int)cx, (int)cy, g, color });
    };
    // four splayed legs, flat on the ground (half height: the cells are tall)
    static const float legs[4] = { 0.8f, 2.35f, 3.95f, 5.5f };
    for (float k : legs)
        for (float r = 1.2f; r <= 2.3f; r += 1.0f)
            put(m.x + std::cos(k + a) * r * 1.4f, m.y + std::sin(k + a) * r * 0.7f, lineGlyph(k + a, false), "1;38;5;143;48;5;58");
    // the body: the breech block behind the mount, the mount itself
    put(m.x - std::cos(a), m.y - std::sin(a), "\xE2\x96\x88", "38;5;70;48;5;58");
    // the barrels, each with its muzzle; a firing one flashes
    for (uint32_t k = 0; k < barrels(); ++k) {
        const float o = barrelOffset(k), len = flashing(k) ? barrelLength() - 0.5f : barrelLength();
        for (float r = 1.0f; r < len; r += 1.0f)
            put(m.x + r * std::cos(a) - o * std::sin(a), m.y + r * std::sin(a) + o * std::cos(a),
                lineGlyph(a, model == Model::Sergei), "1;38;5;191;48;5;58");
        put(m.x + len * std::cos(a) - o * std::sin(a), m.y + len * std::sin(a) + o * std::cos(a),
            flashing(k) ? "\xE2\x9C\xB1" : "\xE2\x80\xA2", flashing(k) ? "1;38;5;231;48;5;208" : "1;38;5;214");   // ✱ / •
    }
    const char* mountColor = gunner ? "1;38;5;226;48;5;58" : "1;38;5;228;48;5;58";
    put((float)m.x, (float)m.y, glyphAt(4), mountColor);
    return out;
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
        if (player->inventory.count(Thing::Vodka) >= awake.size()) {
            for (size_t k = 0; k < awake.size(); ++k) player->inventory.take(Thing::Vodka);
            for (auto m : awake) {
                m->passedOut = true;
                m->say("Vodka? ...Davai. Ammu vaikka Kuuta, min\xC3\xA4 pid\xC3\xA4n tauon.");
            }
            msg = awake.size() == 1
                ? "Ojennat ampujalle pullon vodkaa. H\xC3\xA4n kulauttaa sen kerralla ja sammuu hiekkas\xC3\xA4kkien viereen. Tykki on sinun!"
                : "Jokainen saa oman pullonsa. Miehist\xC3\xB6 juo ja sammuu rivissä. Tykki on sinun!";
        } else {
            awake.front()->say(player->inventory.count(Thing::Vodka) == 0
                ? "H\xC3\xA4ivy, juoppo! Sotilasalue! ...Ellei sinulla sitten ole pulloa?"
                : "Meit\xC3\xA4 on useampi, juoppo. Pullo jokaiselle tai h\xC3\xA4ivy!");
            msg = "Miehist\xC3\xB6 t\xC3\xB6n\xC3\xA4isee sinut kauemmas tykist\xC3\xA4.";
            player->damage(1);
            return false;
        }
    } else if (!all.empty()) {
        msg = "Ampuja kuorsaa hiekkas\xC3\xA4kkien v\xC3\xA4liss\xC3\xA4. Istut tykin taakse, eik\xC3\xA4 kukaan est\xC3\xA4!";
    } else {
        if (level && level->isWarLevel) {
            // at the front nobody asks: the crew lies where the last drone found them
            msg = "Miehist\xC3\xB6 on kaatunut hiekkas\xC3\xA4kkien taakse. Otat IT-tykin haltuusi!";
        } else {
            msg = "Miehist\xC3\xB6 on poissa. Kaappaat IT-tykin! Sotilaat eiv\xC3\xA4t ilahdu.";
            if (level) level->alertCops(player, 20);
        }
    }
    gunner = player;
    player->manning = this;
    msg += string(" ") + modelName() + ". (WASD k\xC3\xA4\xC3\xA4nt\xC3\xA4\xC3\xA4 45\xC2\xB0, v\xC3\xA4lily\xC3\xB6nti ampuu, Esc poistuu. Ammuksia " + std::to_string(ammo) + ".)";
    return true;
}

void AAGun::leave()
{
    if (gunner && gunner->manning == this) gunner->manning = nullptr;
    gunner = nullptr;
}

uint32_t AAGun::shotMs() const
{
    switch (model) {
        case Model::Zu23:     return 400;
        case Model::Vierling: return 150;
        default:              return 200;
    }
}

uint32_t AAGun::maxAmmo() const
{
    return model == Model::Vierling ? 400 : AMMO;
}

const char* AAGun::modelName() const
{
    switch (model) {
        case Model::Zu23:     return "ZU-23";
        case Model::Vierling: return "FLAKVIERLING 7,62";
        default:              return "SERGEI ZU-23-2";
    }
}

ShellHit AAGun::shoot(float a, Person* source, string& boom)
{
    const Coord from = centre();
    if (model == Model::Zu23)
        level->audio.addSound(from, SoundType::Gunshot,
            "BUUM! Yksiputkinen IT-tykki jyr\xC3\xA4ht\xC3\xA4\xC3\xA4!", "Tykin laukaus!", "...tykin jyrin\xC3\xA4\xC3\xA4 kaukaa...", 60);
    else
        level->audio.addSound(from, SoundType::Gunshot,
            "BADABUM! Sergein putket vuorotellen!", "Ilmatorjuntatulta!", "...tykin jyrin\xC3\xA4\xC3\xA4 kaukaa...", 60);
    const ShellHit hit = traceShell(*level, from, a, this);
    // the barrels take turns: the shell leaves from this one's muzzle
    const uint32_t k = nextBarrel % barrels();
    nextBarrel = (k + 1) % barrels();
    flashBarrel(k);
    float tx, ty;
    barrelTip(k, tx, ty, a);
    common::shellEvent(Coord{ (uint32_t)std::max(0L, std::lround(tx)), (uint32_t)std::max(0L, std::lround(ty)) }, hit.at);
    boom = aaExplosion(*level, hit.at, source);
    return hit;
}

bool AAGun::fire(string& msg)
{
    if (wrecked) { msg = "Tykki on romuna."; return false; }
    if (ammo == 0) {
        msg = "*KLIK* Ammukset loppuivat. IT-tykki on nyt pelkk\xC3\xA4\xC3\xA4 rautaa.";
        return false;
    }
    if (model == Model::Vierling) {
        // four barrels of rifle rounds: they mow men down and bounce off armour
        Person* shooter = gunner ? (Person*)gunner : nullptr;
        if (!shooter) return false;
        level->audio.addSound(centre(), SoundType::Gunshot,
            "TAKATAKATAKA! Flakvierling sylkee luoteja!", "Konekiv\xC3\xA4\xC3\xA4rien sarjatulta!", "...sarjatulta kaukaa...", 50);
        uint32_t hits = 0;
        bool bounced = false;
        // one barrel after another, each from its own muzzle
        for (uint32_t k = 0; k < 4 && ammo > 0; ++k) {
            --ammo;
            const float spread = ((std::rand() % 13) - 6) / 100.0f;
            const Vehicle* stopped = nullptr;
            flashBarrel(k, k * VIERLING_GAP_MS);
            float tx, ty;
            barrelTip(k, tx, ty, angle);
            // the barrels are harmonised: their streams cross twelve cells out, on the aim line
            const Coord m = centre();
            const float aim = std::atan2(m.y + 12.0f * std::sin(angle) - ty, m.x + 12.0f * std::cos(angle) - tx);
            if (level->fireBulletFrom(shooter, tx, ty, aim + spread, 30, 3 + std::rand() % 3, 70, &stopped, k * VIERLING_GAP_MS)) ++hits;
            if (stopped) bounced = true;
        }
        level->alertCops(shooter, 30);
        msg = "TAKATAKATAKA! ";
        if (bounced) msg += "Luodit kilpistyv\xC3\xA4t Bradleyn panssarista! ";
        else if (hits) msg += hits == 1 ? "Osuma! " : std::to_string(hits) + " osumaa! ";
        msg += "[" + std::to_string(ammo) + "/" + std::to_string(maxAmmo()) + "]";
        return true;
    }
    --ammo;
    string boom;
    const ShellHit hit = shoot(angle, gunner, boom);
    msg = "BUUM! ";
    if (hit.drone) msg += "Osuma drooniin! ";
    else if (!hit.what.empty()) msg += "Osuma: " + hit.what + "! ";
    else msg += "Ammus r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4 ilmassa. ";
    if (!boom.empty()) msg += boom + " ";
    msg += "[" + std::to_string(ammo) + "/" + std::to_string(maxAmmo()) + "]";
    return true;
}

bool AAGun::aimAtDrone()
{
    const Coord c = centre();
    const Drone* best = nullptr;
    float bestDist = (float)RANGE;
    for (const auto& d : level->drones) {
        const float r = std::hypot(d.x - c.x, d.y - c.y);
        if (r < bestDist && r >= ARMING) { best = &d; bestDist = r; }
    }
    if (!best) return false;
    angle = std::atan2(best->y - c.y, best->x - c.x);
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
    // a drone within reach: the crew always goes for it, and a shell that misses bursts
    // wherever it comes down, just like the drunk's
    if (gun->aimAtDrone()) {
        if (fireCooldown > 0) { --fireCooldown; return; }
        fireCooldown = 3 + std::rand() % 3;
        if (common::random(30)) say("Kohde havaittu! OGON!");
        const float spread = ((std::rand() % 21) - 10) / 100.0f;   // +-0.1 rad
        string boom;
        gun->shoot(gun->angle + spread, nullptr, boom);
        return;
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
        const AAGun* gun = dynamic_cast<const AAGun*>(v);
        for (const auto& c : v->cells()) near = near || ((!gun || gun->isCore(c)) && dist(c.x, c.y) <= 2.0f);   // a gun: the sandbags take it
        if (!near) continue;
        if (Bradley* br = dynamic_cast<Bradley*>(v)) {
            if (++br->hits < Bradley::ARMOUR) {
                const uint32_t left = Bradley::ARMOUR - br->hits;
                worst = std::string(left > 10 ? "Osuma Bradleyyn! Panssari kest\xC3\xA4\xC3\xA4. "
                                  : left > 3 ? "Osuma Bradleyyn! Se savuaa jo. "
                                             : "Osuma! Bradley liekehtii, viel\xC3\xA4 vähän! ") + worst;
                level.addSmoke(br->coord);
                continue;
            }
        }
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

    // at the front: a barrier machine gun nest in the burst is silenced
    if (level.isWarLevel && level.silenceMgNests(at, 2.5f)) worst = "Sulkuosaston pes\xC3\xA4ke vaikeni! " + worst;

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
    // the mount, its ring of sandbags and room for the crew inside: clear of buildings, roads,
    // cars and people. (x, y) is the left of the mount's middle row.
    if (x < 7 || y < 4) return false;
    for (uint32_t yy = y - 3; yy <= y + 3; ++yy) {
        for (uint32_t xx = x - 5; xx <= x + 7; ++xx) {
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
                gun->laySandbags(level);
                level.addVehicle(gun);
                const int crew = 1 + std::rand() % 2;
                for (int k = 0; k < crew; ++k) {
                    // inside the ring, either side of the mount
                    ITMies* m = new ITMies(Coord{ (uint32_t)gx + (k ? 4u : 0u) - 2u, (uint32_t)gy }, gun);
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

// a gun just before our line on row y or near it: sandbags on the open ground east of the
// trench's lip, barrels to the east. Its crew lies dead; whoever gets to it can have it.
static AAGun* placeFrontGun(Level& level, uint32_t y)
{
    // whatever was left here: a single barrel, a Sergei or a quad of machine guns
    const int roll = std::rand() % 100;
    const AAGun::Model model = roll < 40 ? AAGun::Model::Zu23 : roll < 75 ? AAGun::Model::Sergei : AAGun::Model::Vierling;
    if (level.ownLineX.size() < common::SIZEY) return nullptr;
    for (int dy = 0; dy < 14; ++dy) {
        for (int sign = 1; sign >= -1; sign -= 2) {
            const int row = (int)y + dy * sign;
            if (row < 2 || row >= (int)common::SIZEY - 2) continue;
            // the mount some seven cells out from the trench's middle: the ring's west gap opens on it
            for (int cx = level.ownLineX[row] + 7; cx <= level.ownLineX[row] + 9; ++cx) {
                bool free = true;
                for (int yy = row - 1; yy <= row + 1 && free; ++yy)
                    for (int x = cx - 1; x <= cx + 1 && free; ++x) {
                        const Coord c { (uint32_t)x, (uint32_t)yy };
                        const Trench t = level.trenchAt(c);
                        free = !level.hit(c) && !level.getVehicleAt(c) && t != Trench::Own && t != Trench::Wire;
                    }
                if (!free) continue;
                AAGun* gun = new AAGun(Coord{ (uint32_t)cx - 1, (uint32_t)row }, model);
                gun->angle = gun->shown = 0.0f;
                gun->laySandbags(level);
                level.addVehicle(gun);
                return gun;
            }
        }
    }
    return nullptr;
}

void populateAAGuns(Level& level, uint32_t moscowStage)
{
    if (level.isWarLevel) {
        // from the second level on: one by where a new man arrives, sometimes another further along
        if (level.warStage < 2) return;
        const PlayerNS::Player* p = level.getPlayer();
        const uint32_t y = p ? p->coord.y : common::PLAYER_START_Y;
        placeFrontGun(level, y);
        if (common::random(60)) {
            const int off = (30 + std::rand() % 31) * (common::random(50) ? 1 : -1);
            placeFrontGun(level, (uint32_t)std::max(10, std::min((int)common::SIZEY - 10, (int)y + off)));
        }
        return;
    }
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
        // 45 degrees a press, like every other weapon
        case 'a': case 'j': case 'd': case 'l':
        case 'w': case 'i': case 's': case 'k':
            gun->angle = common::aimStep(gun->angle, c);
            break;
        case ' ': {
            if (msSinceShot < gun->shotMs()) break;
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

// ============ everyone shoots at drones ============

void downDrone(Level& level, size_t index)
{
    if (index >= level.drones.size()) return;
    const Drone d = level.drones[index];
    level.drones.erase(level.drones.begin() + index);
    const Coord at { (uint32_t)std::max(0.0f, std::min(d.x, common::SIZEX - 1.0f)),
                     (uint32_t)std::max(0.0f, std::min(d.y, common::SIZEY - 1.0f)) };
    common::boomEvent(at, 2);
    level.addAnimation(new Animation(Coord{ at.x > 4 ? at.x - 4 : 0, at.y }, "**PAM**", 6));
    level.audio.addSound(at, SoundType::Explosion,
        "PAM! Drooni putoaa liekeiss\xC3\xA4!", "R\xC3\xA4j\xC3\xA4hdys taivaalla!", "...r\xC3\xA4j\xC3\xA4hdys kaukaa...", 50);
    // what it carried comes down with it
    for (auto p : level.persons) {
        if (p->stats.health > 0 && std::hypot((float)p->coord.x - at.x, (float)p->coord.y - at.y) <= 2.0f)
            p->damage(2 + std::rand() % 4);
    }
    if (level.events) level.events->addFiresAround(at);
    level.addSmoke(at);
}

void antiDroneFire(Level& level)
{
    constexpr float RIFLE_RANGE = 18.0f;
    const PlayerNS::Player* player = level.getPlayer();
    for (size_t pi = 0; pi < level.persons.size() && !level.drones.empty(); ++pi) {
        Person* p = level.persons[pi];
        if (p->stats.health == 0) continue;
        // soldiers, air defence, and the Kremlin guards (the ones with radios)
        const bool kremlinGuard = p->type == vartija && p->hasRadio;
        if (p->type != sotilas && p->type != ilmatorjunta && !kremlinGuard) continue;
        // only those out under the open sky can see a drone
        if (const Building* b = level.buildingAt(p->coord)) if (!b->hitWall(p->coord)) continue;
        if (const ITMies* m = dynamic_cast<const ITMies*>(p)) {
            if (m->asleep() || (m->gun && !m->gun->wrecked)) continue;   // the gun does the shooting
        }
        // the nearest drone in rifle range
        size_t target = level.drones.size();
        float best = RIFLE_RANGE;
        for (size_t i = 0; i < level.drones.size(); ++i) {
            const float r = std::hypot(level.drones[i].x - p->coord.x, level.drones[i].y - p->coord.y);
            if (r < best) { best = r; target = i; }
        }
        if (target == level.drones.size() || !common::random(70)) continue;

        const Drone& d = level.drones[target];
        if (kremlinGuard && common::random(25)) p->say("*r\xC3\xA4tin\xC3\xA4* Drooni Kremlin yll\xC3\xA4! Avatkaa tuli!");
        else if (common::random(15)) p->say(common::random(50) ? "Drooni! Ampukaa se alas!" : "Bespilotnik! OGON!");
        level.audio.addSound(p->coord, SoundType::Gunshot,
            "R\xC3\x84K\xC3\x84K\xC3\x84K\xC3\x84! Sotilaat ampuvat droonia!", "Sarjatulta!", "...sarjatulta kaukaa...", 35);
        if (common::random(d.fpv ? 10 : 15)) {   // a small, darting FPV is harder to hit
            level.addTracer((float)p->coord.x, (float)p->coord.y, d.x, d.y);
            downDrone(level, target);
            continue;
        }
        // a miss: the round flies on and comes down on whoever is in the way
        const float angle = std::atan2(d.y - p->coord.y, d.x - p->coord.x) + ((std::rand() % 41) - 20) / 100.0f;
        Person* hit = level.fireBullet(p, angle, 25, 2 + std::rand() % 3, 30);
        if (hit && hit == player) {
            level.printer->showMessage("Harhaluoti! Sotilaat ampuivat droonia ja osuivat sinuun!", &level, false);
        } else if (hit && player) {
            hit->report("Harhaluoti osui ohikulkijaan!");
        }
    }
}

// ============ a drone on a petrol station ============

std::string petrolStationBlast(Level& level, Building* hit)
{
    // the whole forecourt: the kiosk and its pumps
    Building* station = hit;
    if (hit->type == EGasPump) {
        for (auto b : level.buildings) {
            if (b->type != EGasStation) continue;
            if (hit->wallLeft + 2 >= b->wallLeft && hit->wallLeft <= b->wallRight + 2 &&
                hit->wallTop >= b->wallBot && hit->wallTop <= b->wallBot + 4) station = b;
        }
    }
    const Coord mid { (station->wallLeft + station->wallRight) / 2, (station->wallTop + station->wallBot) / 2 };
    std::vector<Coord> bursts { mid };
    for (auto b : level.buildings) {
        if (b->type != EGasPump) continue;
        const Coord pump { b->wallLeft, b->wallTop };
        if (std::hypot((float)pump.x - mid.x, (float)pump.y - mid.y) <= 10.0f) bursts.push_back(pump);
    }
    common::boomEvent(mid, 7);
    level.addAnimation(new Animation(Coord{ mid.x > 8 ? mid.x - 8 : 0, mid.y }, "#####KABOOM#####", 14));
    level.audio.addSound(mid, SoundType::Explosion,
        "KA-BOOOOM! Bensa-asema r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4 tulipalloksi!", "Valtava r\xC3\xA4j\xC3\xA4hdys!", "...valtava jyrin\xC3\xA4 kaukaa...", 90);
    for (const auto& c : bursts) aaExplosion(level, c, nullptr);
    if (level.events) level.events->setAblaze(station->wallLeft, station->wallTop, station->wallRight, station->wallBot + 5);

    // the fireball reaches the cars queueing at the pumps, and the fire runs down the queue
    std::vector<Vehicle*> burning;
    for (auto v : level.vehicles) {
        if (v->wrecked) { burning.push_back(v); continue; }
        if (std::hypot((float)v->coord.x - mid.x, (float)v->coord.y - mid.y) <= 14.0f) {
            v->wrecked = true;
            burning.push_back(v);
        }
    }
    for (size_t i = 0; i < burning.size(); ++i) {
        const Coord at = burning[i]->coord;
        if (std::hypot((float)at.x - mid.x, (float)at.y - mid.y) > 60.0f) continue;
        for (const auto& c : burning[i]->cells()) {
            if (level.events && common::random(60)) level.events->addFiresAround(c);
            if (common::random(30)) level.addSmoke(c);
        }
        for (auto v : level.vehicles) {
            if (v->wrecked || !common::random(70)) continue;
            if (std::hypot((float)v->coord.x - at.x, (float)v->coord.y - at.y) <= 5.0f) {
                v->wrecked = true;
                if (AAGun* g = dynamic_cast<AAGun*>(v)) g->leave();
                common::boomEvent(v->coord, 2);
                burning.push_back(v);
            }
        }
    }
    // people caught in the fireball
    for (auto p : level.persons) {
        if (p->stats.health == 0) continue;
        const float r = std::hypot((float)p->coord.x - mid.x, (float)p->coord.y - mid.y);
        if (r <= 7.0f) p->damage(r < 4.0f ? 15 + std::rand() % 10 : 5 + std::rand() % 6);
    }
    for (int i = 0; i < 12; ++i) level.addSmoke(Coord{ mid.x + std::rand() % 9 - 4, mid.y + std::rand() % 5 });
    return station->getName();
}
