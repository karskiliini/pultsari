// Free driving: a car someone has taken over drives in 72 directions, with a
// gearbox, a clutch and an engine that stalls.
#include "vehicle.hpp"
#include "level.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "common.hpp"
#include "persons.hpp"
#include "building.hpp"
#include <cmath>
#include <algorithm>

using std::string;

namespace {

constexpr float PI = 3.14159265f;
constexpr float STEP = 2.0f * PI / Car::HEADINGS;
// the body in cell widths (a cell is twice as tall as it is wide)
constexpr float HALF_LEN = 1.9f;
constexpr float HALF_WID = 0.7f;
constexpr float WHEELBASE = 2.4f;
constexpr float MAX_STEER = 0.6f;       // radians of front-wheel angle at full lock

// --- drivetrain ---
constexpr float RPM_PER_SPEED = 250.0f; // engine rpm per (cell width per second) in a 1:1 gear
constexpr float IDLE = 820.0f;
constexpr float LIMITER = 6600.0f;
constexpr float STALL = 320.0f;
constexpr float ENGINE_PUSH = 16000.0f; // rpm/s at full throttle and peak torque, nothing attached
constexpr float CLUTCH_GRIP = 40.0f;    // clutch torque per rpm of slip
constexpr float CLUTCH_MAX = 12700.0f;  // the most the clutch can pass on
constexpr float CAR_INERTIA = 1.384e-4f;// how much the car moves for a unit of clutch torque

// gear -1 (R) .. 4
float ratio(int gear)
{
    static const float r[] = { 3.6f, 0.0f, 3.4f, 2.0f, 1.35f, 1.0f };
    return r[std::max(-1, std::min(4, gear)) + 1];
}

// torque curve, 0..1: weak at idle, best in the middle, fading at the top
float torque(float rpm)
{
    const float x = (rpm - 3600.0f) / 3600.0f;
    return std::max(0.45f, 1.0f - x * x);
}

// 2x2 sub-cell masks (bit 1 top left, 2 top right, 4 bottom left, 8 bottom right) as block glyphs
const char* quadrant(uint8_t m)
{
    static const char* g[16] = {
        " ", "\xE2\x96\x98", "\xE2\x96\x9D", "\xE2\x96\x80",
        "\xE2\x96\x96", "\xE2\x96\x8C", "\xE2\x96\x9E", "\xE2\x96\x9B",
        "\xE2\x96\x97", "\xE2\x96\x9A", "\xE2\x96\x90", "\xE2\x96\x9C",
        "\xE2\x96\x84", "\xE2\x96\x99", "\xE2\x96\x9F", "\xE2\x96\x88",
    };
    return g[m & 15];
}

float wrapAngle(float a)
{
    while (a > PI) a -= 2 * PI;
    while (a < -PI) a += 2 * PI;
    return a;
}

} // namespace

const char* Car::gearName(int g)
{
    static const char* n[] = { "R", "N", "1", "2", "3", "4" };
    return n[std::max(-1, std::min(4, g)) + 1];
}

std::vector<Coord> Car::cells() const
{
    if (freeMode) return freeCells;
    return Vehicle::cells();
}

void Car::makeFree()
{
    if (freeMode) return;
    const auto body = Vehicle::cells();
    float sx = 0, sy = 0;
    for (const auto& c : body) { sx += c.x; sy += c.y; }
    px = sx / body.size() + 0.5f;
    py = (sy / body.size() + 0.5f) * 2.0f;
    if (horizontal) heading = direction > 0 ? 0 : HEADINGS / 2;
    else heading = direction > 0 ? HEADINGS / 4 : HEADINGS * 3 / 4;
    headingF = heading * STEP;
    velocity = 0;
    gear = 0;
    rpm = engineOn ? IDLE : 0;
    freeMode = true;
    script = Script::Parked;
    reckless = false;
    rasterize();
}

void Car::rasterizeAt(float x, float y, int h, std::vector<Coord>& out, std::vector<uint8_t>* masks, std::vector<bool>* glass) const
{
    out.clear();
    if (masks) masks->clear();
    if (glass) glass->clear();
    const float a = h * STEP;
    const float ca = std::cos(a), sa = std::sin(a);
    const float reach = std::sqrt(HALF_LEN * HALF_LEN + HALF_WID * HALF_WID) + 0.5f;
    const int x0 = (int)std::floor(x - reach), x1 = (int)std::floor(x + reach);
    const int y0 = (int)std::floor((y - reach) / 2.0f), y1 = (int)std::floor((y + reach) / 2.0f);
    // the windscreen sits a little ahead of the middle: that cell shows which end is the front
    const float gx = x + ca * 0.8f, gy = y + sa * 0.8f;
    for (int cy = y0; cy <= y1; ++cy) {
        for (int cx = x0; cx <= x1; ++cx) {
            if (cx < 0 || cy < 0 || cx >= (int)common::SIZEX || cy >= (int)common::SIZEY) continue;
            uint8_t m = 0;
            for (int sy = 0; sy < 2; ++sy) {
                for (int sx = 0; sx < 2; ++sx) {
                    const float dx = cx + 0.25f + 0.5f * sx - x;
                    const float dy = cy * 2.0f + 0.5f + sy - y;
                    const float along = dx * ca + dy * sa;
                    const float across = -dx * sa + dy * ca;
                    if (std::fabs(along) <= HALF_LEN && std::fabs(across) <= HALF_WID) m |= 1 << (sy * 2 + sx);
                }
            }
            if (!m) continue;
            out.push_back(Coord{ (uint32_t)cx, (uint32_t)cy });
            if (masks) masks->push_back(m);
            if (glass) glass->push_back((int)std::floor(gx) == cx && (int)std::floor(gy / 2.0f) == cy);
        }
    }
}

void Car::rasterize()
{
    rasterizeAt(px, py, heading, freeCells, &freeMasks, &freeGlass);
}

bool Car::fits(const std::vector<Coord>& body) const
{
    for (const auto& c : body) {
        if (c.x < 1 || c.y < 1 || c.x + 1 >= common::SIZEX || c.y + 1 >= common::SIZEY) return false;
        if (const Building* b = level->buildingAt(c)) if (!b->isBreach(c)) return false;
        if (Vehicle* v = level->getVehicleAt(c)) if (v != this) return false;
    }
    return true;
}

Coord Car::driverCell() const
{
    return Coord{ (uint32_t)std::max(0.0f, px), (uint32_t)std::max(0.0f, py / 2.0f) };
}

bool Car::exitCell(Coord& out) const
{
    // the driver's door is on the left of the car; failing that anywhere around it
    const float a = heading * STEP;
    const float side[] = { -PI / 2, PI / 2, 0, PI };
    for (float s : side) {
        for (float d = 1.2f; d < 3.5f; d += 0.6f) {
            const float x = px + std::cos(a + s) * d * (s == 0 || s == PI ? HALF_LEN : 1.0f);
            const float y = py + std::sin(a + s) * d * (s == 0 || s == PI ? HALF_LEN : 1.6f);
            if (x < 1 || y < 2) continue;
            const Coord c { (uint32_t)x, (uint32_t)(y / 2.0f) };
            if (c.x + 1 >= common::SIZEX || c.y + 1 >= common::SIZEY) continue;
            if (hitVehicle(c) || level->getVehicleAt(c) || level->hit(c)) continue;
            out = c;
            return true;
        }
    }
    return false;
}

void Car::changeGear(int newGear)
{
    newGear = std::max(-1, std::min(4, newGear));
    if (newGear == gear) return;
    gear = newGear;
    // the gear goes in with a jolt: the drive takes up again from slipping
    clutchBite = 0.0f;
    // whatever you do with the clutch, this gearbox crunches every single time
    grindMs = 350;
    static const char* crunch[] = { "KRRRZZK!", "KRAAAK!", "GRRRNK!", "KRZZT!", "RRRAAK!" };
    const string c = crunch[std::rand() % 5];
    common::sound("grind", 0.8f);
    level->audio.addSound(driverCell(), SoundType::Engine,
        c + " Vaihdelaatikko kirskuu", "Vaihdelaatikon kurahdus", "...vaihteiden rouskutusta...", 12);
    if (playerAtWheel && level->printer) {
        string m = c + " ";
        m += gear == 0 ? "Vapaalla." : gear < 0 ? "Pakki." : string(gearName(gear)) + ". vaihde.";
        level->printer->showMessage(m, level, false);
    }
}

bool Car::crank(string& msg)
{
    if (engineOn) return true;
    if (!keysIn) { msg = "Virtalukossa ei ole avainta."; return false; }
    if (gear != 0 && !clutchDown) {
        // starter against a gear: the car hops forward a little and dies
        velocity += gear > 0 ? 0.8f : -0.8f;
        msg = "Auto nytk\xC3\xA4ht\xC3\xA4\xC3\xA4 eteenp\xC3\xA4in! Vaihde on p\xC3\xA4\xC3\xA4ll\xC3\xA4.";
        return false;
    }
    common::sound("starter", 0.7f);
    if (common::random(65)) {
        engineOn = true;
        rpm = IDLE + 400;
        msg = "Rrrr-rrr-RRROOM! Moottori k\xC3\xA4ynnistyy.";
        level->audio.addSound(driverCell(), SoundType::Engine, "Moottori k\xC3\xA4ynnistyy", "Moottorin \xC3\xA4\xC3\xA4ni", "...moottori k\xC3\xA4ynnistyy...", 15);
        return true;
    }
    msg = "Rrr-rrr-rrr... Ei l\xC3\xA4hde. Pid\xC3\xA4 kiinni ja yrit\xC3\xA4 uudestaan.";
    return false;
}

void Car::drive(uint32_t ms)
{
    if (!level) return;
    const bool aiDriving = !playerAtWheel && ai != Ai::None && driverInside && !driverDead;
    const DriveControls c = playerAtWheel ? controls : aiDriving ? aiControls(ms) : DriveControls{};
    controls = DriveControls{};
    PlayerNS::Player* player = playerAtWheel ? level->getPlayer() : nullptr;
    Printer* printer = level->printer;
    const float dt = ms / 1000.0f;

    if (grindMs > ms) grindMs -= ms; else grindMs = 0;
    if (hitCooldownMs > ms) hitCooldownMs -= ms; else hitCooldownMs = 0;

    // --- pedals and the wheel ---
    clutchDown = c.clutch;
    clutchBite = clutchDown ? 0.0f : std::min(1.0f, clutchBite + dt * 1.8f);
    if (crankMs > ms) crankMs -= ms; else crankMs = 0;
    if (c.throttle) throttlePos = std::min(1.0f, throttlePos + 0.3f * c.throttle);
    else throttlePos = std::max(0.0f, throttlePos - dt * 1.6f);
    steerPos = std::max(-1.0f, std::min(1.0f, steerPos + 0.1f * c.steer));
    // the wheel creeps back to the middle as the car rolls
    steerPos -= steerPos * std::min(1.0f, dt * 0.05f * std::fabs(velocity));
    if (player && player->stats.promilles > 15) {
        // drunk hands drift on the wheel
        steerPos += ((std::rand() % 200) - 100) / 100.0f * player->stats.promilles / 3000.0f;
    }
    if (c.shift) changeGear(gear + c.shift);

    if (c.throttle && !engineOn && (player || aiDriving) && crankMs == 0) {
        crankMs = 1200;
        string msg;
        crank(msg);
        if (player && printer) printer->showMessage(msg, level, false);
    }

    // --- engine, clutch and wheels, in small steps so the clutch stays stable ---
    bool limiter = false;
    for (uint32_t t = 0; t < ms; t += 5) {
        const float h = 0.005f;
        const float r = ratio(gear);
        const float dir = gear < 0 ? -1.0f : 1.0f;
        const bool coupled = gear != 0 && !clutchDown;

        float push = 0.0f;
        if (engineOn) {
            float pedal = throttlePos;
            // the idle governor keeps a running engine from dying on its own
            pedal = std::max(pedal, std::max(0.0f, std::min(1.0f, (IDLE + 30.0f - rpm) / 150.0f)));
            if (rpm > LIMITER) { pedal = 0.0f; limiter = true; }
            push = pedal * torque(rpm) * ENGINE_PUSH;
        }
        const float friction = rpm > 1.0f ? 600.0f + rpm * 0.6f : 0.0f;

        float clutchTorque = 0.0f;
        if (coupled) {
            const float wheelRpm = velocity * dir * r * RPM_PER_SPEED;
            const float grip = CLUTCH_MAX * clutchBite;
            clutchTorque = std::max(-grip, std::min(grip, (rpm - wheelRpm) * CLUTCH_GRIP));
            velocity += clutchTorque * r * dir * CAR_INERTIA * h;
        }
        rpm += (push - friction - clutchTorque) * h;
        if (rpm < 0) rpm = 0;
    }
    if (engineOn && rpm < STALL) {
        engineOn = false;
        rpm = 0;
        if (player && printer) printer->showMessage("K\xC3\xB6h-k\xC3\xB6h... Moottori sammuu. (W k\xC3\xA4ynnist\xC3\xA4\xC3\xA4, kytkin pohjaan tai vapaalle)", level, false);
    }
    if (limiter && player && common::random(4)) {
        level->audio.addSound(driverCell(), SoundType::Engine, "BRAP-BRAP-BRAP! Rajoitin paukkuu", "Moottori ulvoo", "...moottorin ulvontaa...", 25);
    } else if (engineOn && rpm > 4500 && common::random(6)) {
        level->audio.addSound(driverCell(), SoundType::Engine, "VRUUUUM!", "Moottori ulvoo", "...moottorin ulvontaa...", 25);
    }

    // rolling resistance, air, brakes; a dead engine in gear holds the car
    float drag = 0.3f + 0.002f * velocity * velocity;
    if (c.brake) drag += 16.0f;
    if (!engineOn && gear != 0 && !clutchDown) drag += 6.0f;
    const float slow = std::min(std::fabs(velocity), drag * dt);
    velocity += velocity > 0 ? -slow : slow;

    // --- steering: the car turns about its rear axle ---
    headingF = wrapAngle(headingF + velocity / WHEELBASE * std::tan(steerPos * MAX_STEER) * dt);
    heading = ((int)std::lround(headingF / STEP) % HEADINGS + HEADINGS) % HEADINGS;

    // --- move along the snapped heading, a fraction of a cell at a time ---
    const float a = heading * STEP;
    float dist = velocity * dt;
    const int steps = std::max(1, (int)std::ceil(std::fabs(dist) / 0.35f));
    const float sx = std::cos(a) * dist / steps, sy = std::sin(a) * dist / steps;
    std::vector<Coord> body;
    for (int k = 0; k < steps; ++k) {
        const float nx = px + sx, ny = py + sy;
        rasterizeAt(nx, ny, heading, body, nullptr, nullptr);
        bool blocked = false;
        bool outside = false;
        Vehicle* other = nullptr;
        for (const auto& cell : body) {
            if (hitVehicle(cell)) continue;      // already ours
            if (cell.x < 1 || cell.y < 1 || cell.x + 1 >= common::SIZEX || cell.y + 1 >= common::SIZEY) { blocked = outside = true; break; }
            if (const Building* b = level->buildingAt(cell)) if (!b->isBreach(cell)) { blocked = true; break; }
            if (Vehicle* v = level->getVehicleAt(cell)) if (v != this) { blocked = true; other = v; break; }
        }
        if (blocked) {
            const float impact = std::fabs(velocity);
            velocity = -velocity * 0.25f;
            if (impact > 2.5f) {
                common::boomEvent(driverCell(), 1);
                common::sound("crash", std::min(1.0f, impact / 15.0f));
                level->audio.addSound(driverCell(), SoundType::Fight, "RYSÄYS! Pellit rytisee", "Kolarin ryskett\xC3\xA4", "...kolarin rysk\xC3\xA4ys...", 30);
                if (player) {
                    const uint32_t dmg = (uint32_t)((impact - 1.5f) / 4.0f) + 1;
                    HurtBy who(level, "rattijuoppous (kolaroit " + name() + ":lla)");
                    player->damage(dmg);
                    string what = outside ? "Kaupungin raja tulee vastaan" : other ? "Ajat kiinni: " + other->name() : "Ajat p\xC3\xA4in sein\xC3\xA4\xC3\xA4";
                    if (printer) printer->showMessage("RYS\xC3\x84YS! " + what + "! Otsa kolahtaa rattiin.", level, false);
                    Car* hit = dynamic_cast<Car*>(other);
                    if (hit && hit->police) level->alertCops(player, 25);
                }
                if (impact > 12.0f && engineOn) engineOn = false;
            }
            break;
        }

        // people in the way go under the wheels
        bool stop = false;
        for (const auto& cell : body) {
            if (hitVehicle(cell)) continue;
            Person* p = level->getPerson(cell);
            if (!p || p->stats.health == 0) continue;
            const bool hitPlayer = p->type == pelaaja;
            if (hitPlayer && (playerAtWheel || level->playerDead)) continue;
            const float impact = std::fabs(velocity);
            if (impact < 0.8f) { stop = true; break; }   // a nudge: they are in the way
            HurtBy who(level, playerAtWheel ? "oma autosi" : name() + ", joka ajoi ylitsesi");
            p->damage((uint32_t)(1 + impact / 3.0f));
            common::setHitDirection(std::cos(a) > 0.4f ? 1 : std::cos(a) < -0.4f ? -1 : 0, std::sin(a) > 0.4f ? 1 : std::sin(a) < -0.4f ? -1 : 0);
            common::hitEvent(cell, (uint32_t)(2 + impact / 3.0f));
            level->audio.addSound(cell, SoundType::Fight, "KOLAHDUS! Auto osuu ihmiseen", "Kolahdus", "...kolahdus...", 18);
            if (player) {
                if (printer) printer->showMessage("TÖMPS! Ajat " + level->nameOf(p) + ":n yli!", level, false);
                level->alertCops(player, 20);
            }
            // a ramming driver who got the shooter makes off
            if (ai == Ai::Ram && p == aiTarget) { ai = Ai::Flee; aiMs = 0; aiStuckMs = 0; }
            if (!player && hitPlayer && printer) {
                printer->showMessage("TÖMPS! " + name() + " ajaa p\xC3\xA4\xC3\xA4llesi! Lenn\xC3\xA4t konepellin yli.", level, false);
            }
            // thrown off to the side
            const float side = (std::rand() % 2) ? PI / 2 : -PI / 2;
            const Coord to { (uint32_t)std::max(0.0f, cell.x + std::cos(a + side) * 2.0f + std::cos(a) * impact / 4.0f),
                             (uint32_t)std::max(0.0f, cell.y + std::sin(a + side) * 1.0f + std::sin(a) * impact / 8.0f) };
            if (to.x < common::SIZEX && to.y < common::SIZEY && !level->hit(to) && !level->getVehicleAt(to)) p->coord = to;
            else stop = true;
            velocity *= 0.7f;
        }
        if (stop) { velocity = 0; break; }

        px = nx;
        py = ny;
    }
    rasterize();

    // old cars smoke when you floor them
    if (engineOn && throttlePos > 0.6f && common::random(25) && !freeCells.empty()) {
        const float bx = px - std::cos(a) * (HALF_LEN + 0.8f), by = py - std::sin(a) * (HALF_LEN + 0.8f);
        if (bx > 0 && by > 0) {
            const Coord behind { (uint32_t)bx, (uint32_t)(by / 2.0f) };
            if (!hitVehicle(behind) && behind.x < common::SIZEX && behind.y < common::SIZEY) level->addGas(behind, false);
        }
    }

    if (player) player->coord = driverCell();
}

// --- a driver under fire ---

void Car::bailOut(const Coord& danger, bool seen)
{
    makeFree();
    driverInside = false;
    throttlePos = 0;
    ai = Ai::None;
    Coord out;
    if (exitCell(out)) {
        Person* driver = level->createPerson(out, kansalainen);
        if (driver) {
            driver->say(common::random(50) ? "Ne strelyaite! Ottakaa auto!" : "Pomogite!");
            if (Kansalainen* k = dynamic_cast<Kansalainen*>(driver)) k->fleeFrom(danger);
        }
    }
    if (seen && level->printer)
        level->printer->showMessage("Kuski hypp\xC3\xA4\xC3\xA4 ulos ja juoksee karkuun! " + name() + " j\xC3\xA4\xC3\xA4 k\xC3\xA4ym\xC3\xA4\xC3\xA4n.", level, false);
}

bool Car::takeShot(Person* shooter, const Coord& at)
{
    if (armoured) return true;
    if (!driverInside || driverDead || playerAtWheel) return common::random(50);
    const bool byPlayer = shooter && shooter->type == pelaaja;
    Printer* printer = level->printer;
    auto killDriver = [&]() {
        driverDead = true;
        ai = Ai::None;
        throttlePos = 0;
        common::hitEvent(at, 4);
        level->audio.addSound(at, SoundType::Glass, "KLIRR! Luoti tuulilasin l\xC3\xA4pi", "Lasi s\xC3\xA4rkyy", "...lasin helin\xC3\xA4\xC3\xA4...", 20);
        if (byPlayer && printer)
            printer->showMessage("Osuit kuskiin! " + name() + " pys\xC3\xA4htyy. Kuski lys\xC3\xA4ht\xC3\xA4\xC3\xA4 rattia vasten.", level, false);
    };
    if (shotAt) {
        // already running or ramming: another bullet may still find the driver
        if (common::random(15)) { killDriver(); return true; }
        return common::random(50);
    }
    shotAt = true;
    const int r = std::rand() % 100;
    if (r < 13) {
        killDriver();
        return true;
    }
    if (police) return common::random(50);   // politsija does not run from a bullet
    if (r < 88) {
        // three in four just want out
        bailOut(shooter ? shooter->coord : at, byPlayer);
    } else if (shooter) {
        // the rest floor it: straight at the shooter, then away
        makeFree();
        ai = Ai::Ram;
        aiTarget = shooter;
        aiFrom = shooter->coord;
        aiMs = 0;
        aiStuckMs = 0;
        level->audio.addSound(driverCell(), SoundType::Engine, "VRUUUM! Moottori ulvoo", "Moottori ulvoo", "...renkaat kirskuvat...", 25);
        if (byPlayer && printer) printer->showMessage("Kuski painaa kaasun pohjaan ja ohjaa auton suoraan sinua kohti!", level, false);
    }
    return common::random(50);
}

DriveControls Car::aiControls(uint32_t ms)
{
    DriveControls c;
    aiMs += ms;
    if (!engineOn) { c.throttle = 1; c.clutch = true; return c; }   // crank it

    if (ai == Ai::Ram) {
        if (level->isAlive(aiTarget)) aiFrom = aiTarget->coord;
        else aiTarget = nullptr;
    }
    const float tx = aiFrom.x + 0.5f, ty = (aiFrom.y + 0.5f) * 2.0f;
    const float dx = tx - px, dy = ty - py;
    const float dist = std::sqrt(dx * dx + dy * dy);

    if (ai == Ai::Ram) {
        const bool passed = aiMs > 1500 && dist > 5.0f && dx * std::cos(headingF) + dy * std::sin(headingF) < 0;
        if (!aiTarget || passed || dist < 2.0f || aiMs > 6000) { ai = Ai::Flee; aiMs = 0; aiStuckMs = 0; }
    } else if (ai == Ai::Flee && aiMs > 9000) {
        ai = Ai::Stop;
    }

    if (ai == Ai::Stop) {
        c.brake = true;
        c.clutch = true;
        if (standing()) {
            // far enough: the driver gets out and leaves the car where it stands
            bailOut(aiFrom, false);
        }
        return c;
    }

    // at the shooter, or straight away from where the shots came from
    const float want = ai == Ai::Ram ? std::atan2(dy, dx) : std::atan2(-dy, -dx);
    const float diff = wrapAngle(want - headingF);
    const float aim = std::max(-1.0f, std::min(1.0f, diff * 2.0f));
    if (aim > steerPos + 0.05f) c.steer = 1;
    else if (aim < steerPos - 0.05f) c.steer = -1;

    // panic driving: first gear with the revs high, then up through the box
    if (gear <= 0) { c.shift = 1; c.clutch = true; c.throttle = 3; return c; }
    c.throttle = 3;
    if (gear == 1 && std::fabs(velocity) < 1.0f && rpm < 3500.0f && aiMs < 400) c.clutch = true;
    if (rpm > 5800.0f && gear < 3) { c.shift = 1; c.clutch = true; }

    // stuck against something: give up and run
    if (std::fabs(velocity) < 0.5f && aiMs > 1500) aiStuckMs += ms; else aiStuckMs = 0;
    if (aiStuckMs > 1500) {
        bailOut(aiFrom, false);
        return DriveControls{};
    }
    return c;
}

// --- getting in and out ---

namespace {

// the car next to the player (touching a side or a corner)
Car* carNextTo(Level* level, const Coord& at)
{
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            if ((dx < 0 && at.x == 0) || (dy < 0 && at.y == 0)) continue;
            const Coord c { at.x + dx, at.y + dy };
            if (Car* car = dynamic_cast<Car*>(level->getVehicleAt(c))) return car;
        }
    }
    return nullptr;
}

void sitDown(PlayerNS::Player* player, Car* car)
{
    car->makeFree();
    car->driverInside = true;
    car->driverDead = false;
    car->playerAtWheel = true;
    car->loot = false;
    player->driving = car;
    player->gaze = car->heading * STEP;
    player->coord = car->driverCell();
}

} // namespace

bool Car::standing() const
{
    if (freeMode) return std::fabs(velocity) < 0.5f;
    if (!driverInside || driverDead || script == Script::Parked || gone) return true;
    // a lane car moves a cell at a time: stopped is longer than two of its steps without one
    return msSinceMoved > 2 * 10000 / std::max<uint32_t>(1, speed);
}

bool enterNearbyCar(PlayerNS::Player* player, Level* level)
{
    Car* car = carNextTo(level, player->coord);
    if (!car) {
        level->printer->showMessage("Vieress\xC3\xA4 ei ole autoa.", level, false);
        return false;
    }
    return enterCar(player, level, car);
}

bool enterCar(PlayerNS::Player* player, Level* level, Car* car)
{
    Printer* printer = level->printer;
    if (!car || car->playerAtWheel) return false;
    if (!car->standing()) {
        // nobody jumps into a moving car
        printer->showMessage(car->name() + " ajaa ohi. Ehdit vain koskettaa kylke\xC3\xA4.", level, false);
        return false;
    }
    if (car->armoured) {
        printer->showMessage("Vovan limusiinin ovet ovat panssaroidut. Kolkutat. Kukaan ei avaa.", level, false);
        return false;
    }

    string msg;
    if (car->driverInside && car->driverDead) {
        // whoever shot the driver did half the job
        msg = "Avaat oven ja revit kuskin ruumiin kadulle. ";
        Coord out;
        if (car->freeMode ? car->exitCell(out) : false) common::hitEvent(out, 3);
        else common::hitEvent(player->coord, 3);
        if (common::random(60)) {
            const uint32_t money = 50 + std::rand() % 400;
            player->stats.money += money;
            msg += "Taskusta l\xC3\xB6ytyy " + std::to_string(money) + " ruplaa. ";
        }
        msg += car->engineOn ? "Moottori k\xC3\xA4y yh\xC3\xA4. Istut l\xC3\xA4mpim\xC3\xA4lle penkille." : "Istut rattiin.";
    } else if (car->driverInside) {
        // a live driver of a stopped car: the key is in the ignition, so drag them out
        const uint32_t chance = 45 + std::min<uint32_t>(30, player->stats.health * 2);
        if (!common::random(chance)) {
            static const char* no[] = {
                "Kuski lukitsee ovet ja n\xC3\xA4ytt\xC3\xA4\xC3\xA4 keskisormea.",
                "Kuski l\xC3\xB6\xC3\xB6 sinua ovella naamaan!",
                "Rimpuilet ovenkahvaa. Kuski huutaa: \"Idi na hui!\"",
            };
            const int k = std::rand() % 3;
            if (k == 1) player->damage(1);
            printer->showMessage(no[k], level, false);
            return false;
        }
        msg = "Revit kuskin ulos autosta! ";
        // the driver lands on the street, furious
        car->makeFree();
        Coord out;
        if (car->exitCell(out)) {
            Person* driver = level->createPerson(out, car->police ? poliisi : car->model == "Ry\xC3\xB6st\xC3\xA4jien Lada" ? ryostaja : kansalainen);
            if (driver) {
                driver->say(car->police ? "Politsija! Seis!" : "Vor! Varas! Auto!");
                level->callForHelp(driver, player, true);
            }
        }
        level->alertCops(player, 15);
        msg += "Avaimet ovat virtalukossa.";
    } else {
        // nobody inside
        if (!car->keysIn && !car->engineOn) {
            printer->showMessage(car->name() + ": ovi aukeaa, mutta virtalukossa ei ole avaimia. Johdot ovat liian monimutkaisia t\xC3\xA4ss\xC3\xA4 kunnossa.", level, false);
            return false;
        }
        if (car->police) {
            msg = "Politsijan auto k\xC3\xA4y tyhj\xC3\xA4k\xC3\xA4ynnill\xC3\xA4, ovi auki. Hypp\xC3\xA4\xC3\xA4t rattiin!";
            level->alertCops(player, 20);
        } else if (car->engineOn) {
            msg = car->name() + " k\xC3\xA4y tyhj\xC3\xA4k\xC3\xA4ynnill\xC3\xA4. Hypp\xC3\xA4\xC3\xA4t rattiin!";
        } else {
            msg = "Avaimet roikkuvat virtalukossa! Istut rattiin. (W k\xC3\xA4ynnist\xC3\xA4\xC3\xA4)";
        }
    }

    sitDown(player, car);
    common::sound("door", 0.6f);
    printer->showMessage(msg + " [W kaasu, Shift kytkin, Space vaihde yl\xC3\xB6s, S+Space alas, A/D ratti, E ulos]", level, false);
    return true;
}

bool leaveCar(PlayerNS::Player* player, Level* level)
{
    Car* car = player->driving;
    if (!car) return false;
    Printer* printer = level->printer;
    Coord out;
    if (!car->exitCell(out)) {
        printer->showMessage("Ovi ei aukea: jotain on tiell\xC3\xA4.", level, false);
        return false;
    }
    const float speed = std::fabs(car->velocity);
    car->playerAtWheel = false;
    car->driverInside = false;
    car->throttlePos = 0;
    car->clutchDown = false;
    player->driving = nullptr;
    player->coord = out;
    common::sound("door", 0.6f);
    if (speed > 3.0f) {
        // out of a moving car: the street is hard
        const uint32_t dmg = 1 + (uint32_t)(speed / 5.0f);
        HurtBy who(level, "asfaltti (hypp\xC3\xA4sit liikkuvasta autosta)");
        player->damage(dmg);
        common::hitEvent(out, dmg + 1);
        printer->showMessage("Hypp\xC3\xA4\xC3\xA4t liikkuvasta autosta ja kieri\xC3\xA4t katua pitkin! Auto jatkaa matkaa ilman sinua.", level, false);
    } else {
        printer->showMessage(car->engineOn ? "Nouset autosta. Moottori j\xC3\xA4\xC3\xA4 k\xC3\xA4ym\xC3\xA4\xC3\xA4n." : "Nouset autosta.", level, false);
    }
    return true;
}

// --- where the drunk driver looks ---

void updateDriverGaze(PlayerNS::Player* player, uint32_t ms)
{
    Car* car = player->driving;
    if (!car) return;
    const float prom = (float)player->stats.promilles;
    player->gazeClockMs += ms;
    const float t = (float)player->gazeClockMs;

    // backing up you look over your shoulder
    float ahead = car->heading * STEP;
    if (car->velocity < -0.5f) ahead += PI;

    // the head sways all the time, more the more you have drunk
    const float amp = 0.08f + prom / 120.0f;
    const float sway = amp * (0.6f * std::sin(t / 700.0f) + 0.4f * std::sin(t / 310.0f + 1.7f));

    // now and then the eyes wander off: a kiosk, a babushka, the bottle on the seat
    if (player->glanceMs > ms) {
        player->glanceMs -= ms;
    } else if (player->glanceMs > 0) {
        player->glanceMs = 0;
        player->glanceTarget = 0;
    } else if ((float)(std::rand() % 10000) < (8.0f + prom * 1.5f)) {
        const float side = (std::rand() % 2) ? 1.0f : -1.0f;
        player->glanceTarget = side * (0.7f + (std::rand() % 100) / 100.0f * (0.6f + prom / 40.0f));
        player->glanceMs = 300 + std::rand() % 500 + (uint32_t)(prom * 25.0f);
        if (common::random(25) && car->level && car->level->printer) {
            static const char* why[] = {
                "Katse harhailee kioskin mainokseen...",
                "Huomaat penkill\xC3\xA4 kaljapullon. Kurkotat...",
                "Tuo babu\xC5\xA1ka n\xC3\xA4ytti tutulta...",
                "Kumarrut katsomaan, mik\xC3\xA4 kolisee lattialla...",
                "Peruutuspeili n\xC3\xA4ytt\xC3\xA4\xC3\xA4 kiinnostavalta...",
                "Radio rahisee. Etsit kanavaa...",
            };
            car->level->printer->showMessage(why[std::rand() % 6], car->level, false);
        }
    }
    // the head turns towards the glance quickly and back more slowly
    const float rate = player->glanceTarget != 0 ? 7.0f : 3.0f;
    player->gazeOffset += (player->glanceTarget - player->gazeOffset) * std::min(1.0f, rate * ms / 1000.0f);

    player->gaze = wrapAngle(ahead + sway + player->gazeOffset);

    // the camera runs ahead of the car where you look, the faster the further
    const float lead = std::min(1.0f, std::fabs(car->velocity) / 12.0f);
    const float tx = std::cos(player->gaze) * lead * 22.0f;
    const float ty = std::sin(player->gaze) * lead * 7.0f;
    const float k = std::min(1.0f, 2.5f * ms / 1000.0f);
    player->camLeadX += (tx - player->camLeadX) * k;
    player->camLeadY += (ty - player->camLeadY) * k;
}

bool gazeSees(const PlayerNS::Player* player, const Coord& from, const Coord& to, uint32_t range)
{
    const float dx = (float)to.x - (float)from.x;
    const float dy = ((float)to.y - (float)from.y) * 2.0f;
    const float d = std::sqrt(dx * dx + dy * dy);
    if (d < 3.0f) return true;                     // what is right around the car
    const float off = std::fabs(wrapAngle(std::atan2(dy, dx) - player->gaze));
    if (off < 0.55f) return d < range * 2.0f;      // straight where you look, far
    if (off < 1.1f) return d < range * 0.8f;       // the corner of the eye
    return false;
}

std::string drivingHud(const Car* car)
{
    string s = "AUTO ";
    for (int g = -1; g <= 4; ++g) {
        if (g == car->gear) s += string("\033[1;30;103m") + Car::gearName(g) + "\033[0m";
        else s += string("\033[2;37m") + Car::gearName(g) + "\033[0m";
    }
    // the rev counter: eight bars, red at the top
    const int bars = std::min(8, (int)std::lround(car->rpm / 825.0f));
    s += " \033[2;37mRPM\033[0m ";
    for (int k = 0; k < 8; ++k) {
        const char* col = k >= 7 ? "1;91" : k >= 5 ? "1;93" : "1;92";
        s += k < bars ? string("\033[") + col + "m\xE2\x96\xAE\033[0m" : string("\033[2;37m\xE2\x96\xAF\033[0m");
    }
    char buf[64];
    snprintf(buf, sizeof(buf), " %4d \033[1;97m%3d km/h\033[0m %03d\xC2\xB0 ", (int)car->rpm, (int)car->kmh(), car->bearing());
    s += buf;
    // the steering wheel: where the rim is turned
    const int pos = (int)std::lround(car->steerPos * 3.0f) + 3;
    s += "\033[2;37m\xE2\x97\x84\033[0m";
    for (int k = 0; k < 7; ++k) s += k == pos ? "\033[1;97m\xE2\x95\xAA\033[0m" : k == 3 ? "\033[2;37m\xC2\xB7\033[0m" : "\033[2;37m\xE2\x94\x80\033[0m";
    s += "\033[2;37m\xE2\x96\xBA\033[0m";
    if (car->clutchDown) s += " \033[1;96mKYTKIN\033[0m";
    if (!car->engineOn) s += " \033[1;91mSAMMUNUT\033[0m";
    if (car->grindMs) s += " \033[1;91mKRRRK\033[0m";
    return s;
}

std::string Car::glyphAt(uint32_t i) const
{
    if (freeMode) return i < freeMasks.size() ? quadrant(freeMasks[i]) : " ";
    // the limousine's long black body has a row of tinted windows
    if (limousine && i > 1 && i + 2 < length && i % 2 == 0) return "\xE2\x96\x92";
    return Vehicle::glyphAt(i);
}

const char* Car::colorAt(uint32_t i) const
{
    if (freeMode) {
        if (i < freeGlass.size() && freeGlass[i]) return "1;96";
        return paint;
    }
    if (limousine) return (i > 1 && i + 2 < length && i % 2 == 0) || i == 0 ? "1;38;5;250" : "38;5;16";
    // black, headlights at the front and the blue migalka on the roof
    if (fsbCar) return i == 0 ? "1;38;5;250" : i == 1 ? "1;94" : "38;5;16";
    return paint;
}
