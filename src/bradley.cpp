#include "bradley.hpp"
#include "animation.hpp"
#include "common.hpp"
#include "fpv.hpp"
#include "level.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "printer.hpp"
#include "trenches.hpp"
#include <algorithm>
#include <cstdlib>

Bradley::Bradley(const Coord& front, uint32_t stop) : Vehicle(front, front.y), stopX(stop)
{
    length = 6;
    horizontal = true;
    direction = -1;
}

// three cells long, two rows deep; the front (index 0) is the top-left, where the gun is
std::vector<Coord> Bradley::cells() const
{
    std::vector<Coord> out;
    for (uint32_t dy = 0; dy < 2; ++dy)
        for (uint32_t dx = 0; dx < 3; ++dx) out.push_back(Coord{ coord.x + dx, coord.y + dy });
    return out;
}

std::string Bradley::glyphAt(uint32_t i) const
{
    if (wrecked) return "\xE2\x96\x93";
    if (i == 0) return "=";                     // the cannon's barrel
    if (i == 1) return "\xE2\x96\x88";          // the turret
    return i >= 3 ? "\xE2\x96\x80" : "\xE2\x96\x93";    // hull, tracks underneath
}

const char* Bradley::colorAt(uint32_t i) const
{
    if (wrecked) return "38;5;240";
    return i == 0 ? "1;38;5;250" : "38;5;180";      // desert tan
}

uint32_t bradleyStopX(const Level& level, uint32_t y)
{
    const int lip = ownTrenchEastX(level, y);
    return (uint32_t)((lip < 0 ? (int)TrenchNS::OWN_X : lip) + 6 + std::rand() % 5);
}

void Bradley::update()
{
    if (wrecked || !level) return;
    if (coord.x > stopX) {
        // tracks over no man's land, two cells a tick; it halts short of a gun position in its way
        for (int step = 0; step < 2 && coord.x > stopX && coord.x > 0; ++step) {
            bool clear = true;
            for (uint32_t dy = 0; dy < 2 && clear; ++dy) {
                const Vehicle* v = level->getVehicleAt(Coord{ coord.x - 1, coord.y + dy });
                clear = !v || v == this;
            }
            if (!clear) { stopX = coord.x; break; }
            --coord.x;
        }
        level->audio.addSound(coord, SoundType::Engine,
            "Bradleyn tela-ketjut kolisevat l\xC3\xA4hell\xC3\xA4!", "Panssariajoneuvon jyrin\xC3\xA4\xC3\xA4", "...telaketjujen kolinaa...", 45);
        return;
    }
    if (!unloaded) {
        // the ramp at the back drops and the squad pours out
        unloaded = true;
        const uint32_t squad = 4 + std::rand() % 3;
        uint32_t out = 0;
        for (int tries = 0; tries < 60 && out < squad; ++tries) {
            const Coord c { coord.x + 3 + std::rand() % 3, coord.y - 1 + std::rand() % 4 };
            if (level->hit(c) || hitVehicle(c)) continue;
            Ukrainalainen* u = new Ukrainalainen(c, Ukrainalainen::Mode::Assault);
            u->printer = level->printer;
            u->pathFinding = level->pathFinding;
            if (level->addPerson(u)) ++out;
            else delete u;
        }
        if (level->printer) level->printer->showMessage("Bradleyn takaluukku aukeaa! Jalkav\xC3\xA4ki rynnist\xC3\xA4\xC3\xA4 ulos!", level, false);
        return;
    }
    if (gunCooldown && --gunCooldown) return;
    gunCooldown = 3;
    // the cannon: the nearest Russian it can see, the new man above all
    Person* target = nullptr;
    uint32_t best = 31;
    for (Person* p : level->persons) {
        if (!isRussianSoldier(*level, p)) continue;
        uint32_t d = coord.distance(p->coord);
        if (p->type == pelaaja && d > 3) d -= 3;
        if (d < best) { best = d; target = p; }
    }
    if (!target) return;
    level->addAnimation(new Animation(coord.x > 2 ? Coord{ coord.x - 2, coord.y } : coord, "==*", 3));
    level->audio.addSound(coord, SoundType::Gunshot,
        "TUK-TUK-TUK! Bradleyn konetykki!", "Konetykin tulta!", "...konetykin tykytyst\xC3\xA4...", 60);
    // three rounds of 25 mm, the burst comes down where the last one lands
    for (uint32_t k = 0; k < 3; ++k) level->addTracer(coord, target->coord, true, TracerKind::Cannon, k * 120);
    level->shellBurst(target->coord, 1, 5, "Bradleyn konetykki");
}
