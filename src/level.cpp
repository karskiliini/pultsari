
#include "level.hpp"
#include "item.hpp"
#include "persons.hpp"
#include "player.hpp"
#include "building.hpp"
#include "printer.hpp"
#include "coord.hpp"
#include "common.hpp"
#include "animation.hpp"
#include "vehicle.hpp"
#include "events.hpp"
#include <algorithm>
#include <iostream>

using std::find;
using std::cout;
using std::endl;
using std::string;
using PlayerNS::Player;

Level::Level(uint32_t difficulty) : stage(difficulty)
{
}

void Level::updateCamera()
{
    auto p = getPlayer();
    if (!p) return;
    cameraX = (int32_t)p->coord.x - (int32_t)common::VIEWPORT_W / 2;
    cameraY = (int32_t)p->coord.y - (int32_t)common::VIEWPORT_H / 2;
    if (cameraX < 0) cameraX = 0;
    if (cameraY < 0) cameraY = 0;
    if (cameraX > (int32_t)(sizex - common::VIEWPORT_W))
        cameraX = (int32_t)(sizex - common::VIEWPORT_W);
    if (cameraY > (int32_t)(sizey - common::VIEWPORT_H))
        cameraY = (int32_t)(sizey - common::VIEWPORT_H);
}

Level::~Level() {
    for (auto& p : buildings)
    {
        delete p;
    }
    buildings.clear();

    for (auto& p : persons)
    {
        // player needs to remain from level to level
        if (p->type != PersonType::pelaaja) {
            delete p;

        }
    }
    persons.clear();

    for (auto& p : items)
    {
        delete p;
    }
    items.clear();

    for (auto& v : vehicles)
    {
        delete v;
    }
    vehicles.clear();

    if (events) {
        delete events;
        events = nullptr;
    }

    for (auto a : animations) delete a;
    animations.clear();
    delete thrownItem;
    thrownItem = nullptr;
}

void Level::buildingTurn()
{
    for (auto& b : buildings)
    {
        b->npcAct();
    }
}

void Level::npcTurn()
{
    for(auto& p : persons) {
        string msg = "";
        if (p->stats.health > 0)
        {
            p->npcAct();
        }
    }
}

void Level::actThrow(Person* source)
{
    bool hit = false;
    while (!hit) {
        thrownItem->actThrow(this);

        auto p = getPerson(thrownItem->coord);
        if (p) {
            string msg = "";
            hit = p->interactThrow(thrownItem, source, msg);
            printer->showMessage(msg, this, false);
        } else {
            if (hitBuilding(thrownItem->coord))
            {
                printer->showMessage("Osuit seinään.", this, false);
                hit = true;
            } else {
                Coord copy { thrownItem->coord.x, thrownItem->coord.y };
                if (common::checkBounds(copy)) {
                    printer->showMessage("Huti meni.", this, false);
                    hit = true;
                }
            }
        }

        if (hit)
        {
            Coord landed = thrownItem->coord;
            bool grenade = thrownItem->type == EGranata;
            delete thrownItem;
            thrownItem = nullptr;
            if (grenade) {
                printer->showMessage("KRANAATTI R\xC3\x84J\xC3\x84HT\xC3\x84\xC3\x84!!", this, false);
                explode(landed, 3, 10, source);
            }
        }

        printer->print(this);
        common::sleep(16);
    }
}

void Level::stepAnimations()
{
    for (auto it = animations.begin(); it != animations.end();) {
        if ((*it)->act()) {
            ++it;
        } else {
            delete *it;
            it = animations.erase(it);
        }
    }
}

void Level::playAnimation()
{
    // realtime advances animations once per frame in the main loop instead
    if (common::realtimeEnabled) return;

    while (!animations.empty()) {
        uint32_t delay = animations.front()->delay;
        stepAnimations();
        printer->print(this);
        common::sleep(delay);
    }
}

void Level::explode(const Coord& c, uint32_t radius, uint32_t maxDamage, Person* source)
{
    for (auto& p : persons) {
        if (p->stats.health > 0 && p->coord.distance(c) <= radius) {
            p->damage(std::rand() % maxDamage + 1);
        }
    }
    uint32_t x = c.x > 4 ? c.x - 4 : 0;
    addAnimation(new Animation(Coord{ x, c.y }, "***BOOM***", 8));
    audio.addSound(c, SoundType::Explosion,
        "PAUUUUM! Kranaatti r\xC3\xA4j\xC3\xA4ht\xC3\xA4\xC3\xA4!", "R\xC3\xA4j\xC3\xA4hdys!", "...r\xC3\xA4j\xC3\xA4hdys kaukaa...", 40);
    if (source) alertCops(source);
}

Person* Level::createPerson(const Coord& coord, PersonType type)
{
    Person* p = nullptr;

    if (!hit(coord))
    {
        switch(type) {
            case vanki:
                p = new Vanki(coord);
                break;
            case skinhead:
                p = new Skinhead(coord);
                break;
            case yka:
                p = new Yka(coord);
                break;
            case mummo:
                p = new Mummo(coord);
                break;
            case poliisi:
                p = new Cop(coord);
                break;
            case varas:
                p = new Varas(coord);
                break;
            case jengi:
                p = new Jengi(coord);
                break;
            case ryostaja:
                p = new Ryostaja(coord);
                break;
            case diileri:
                p = new Diileri(coord);
                break;
            case veteraani:
                p = new Veteraani(coord);
                break;
            case fsb:
                p = new FSBAgent(coord);
                break;
            case juoppo:
                p = new Juoppo(coord);
                break;
            case sotilas:
                p = new Sotilas(coord);
                break;
            case ilmatorjunta:
                p = new ITYksikko(coord);
                break;
            case vartija:
                p = new Vartija(coord);
                break;
            case putin:
                p = new Putin(coord);
                break;
            default:
                break;
        }
    }

    if (p) {
        p->pathFinding = pathFinding;
        p->printer = printer;
        p->setLevel(this);
        addPerson(p);
    }

    return p;
}

Person* Level::getPerson(const Coord& coord) const
{
    for (auto& p : persons) {
        if (p->coord == coord) {
            return p;
        }
    }
    return nullptr;
}

Player* Level::getPlayer() const
{
    for (const auto& p : persons) {
        if (p->type == pelaaja) return dynamic_cast<Player*>(p);
    }
    return nullptr;
}

bool Level::PersonExists(PersonType type) const
{
    for (auto& p : persons) {
        if (p->type == type) return true;
    }
    return false;
}

void Level::alertCops(Person* source)
{
    attack = source;
    for (auto& p : persons) {
        if (p->type == poliisi && p->stats.health > 0) {
            p->alerted = true;
            p->lastKnownPos = source->coord;
            p->alertCooldown = 20;
        }
    }
}

void Level::cleanDead()
{
    bool restart = true;
    while(restart) {
        restart = false;

        for (auto& p : persons) {
            if ((p->stats.health == 0) && (p->type != pelaaja)) {
                if (p->type == putin) {
                    putinDead = true;
                }
                delete p;
                p = nullptr;

                auto it = find(persons.begin(), persons.end(), p);
                persons.erase(it);

                restart = true;
                break;
            }
        }
    }
}

void Level::cleanDiscardedItems()
{
    bool restart = true;
    while(restart) {
        restart = false;

        for (auto& i : items) {
            if (i->discard) {
                delete i;
                i = nullptr;

                auto it = find(items.begin(), items.end(), i);
                items.erase(it);

                restart = true;
                break;
            }
        }
    }
}

bool Level::hitBuilding(const Coord& c) const
{
    for (const auto& b : buildings)
    {
        if (b->hitBuilding(c))
            return true;
    }
    return false;
}

bool Level::hitWindow(const Coord& c) const
{
    for (const auto& b : buildings)
    {
        if (b->hitWindow(c))
            return true;
    }
    return false;
}

bool Level::hitPerson(const Coord& c) const
{
    for (const auto& p : persons)
    {
        if ((p->coord == c) && (p->stats.health > 0))
            return true;
    }
    return false;
}

bool Level::hitItem(const Coord& c) const
{
    for (const auto& i : items)
    {
        if (i->coord == c)
            return true;
    }
    return false;
}

bool Level::hitLOSBlocker(const Coord& c) const
{
    for (const auto& i : items)
    {
        if (i->coord == c && i->blocksLOS)
            return true;
    }
    return false;
}

bool Level::hit(const Coord& c) const
{
    return hitBuilding(c) || hitPerson(c) || hitItem(c);
}

Item* Level::getItem(const Coord& c) const
{
    for (auto& i : items) {
        if (i->coord == c) return i;
    }
    return nullptr;
}

Coord Level::freePosition() const
{
    Coord coord;

    bool recheck;
    do {
        coord.x = std::rand() % sizex;
        coord.y = std::rand() % sizey;
        recheck = hit(coord);
    } while (recheck);

    return coord;
}

Person* Level::checkPerson(const Coord& c)
{
    for (auto& p : persons) {
        if (p->coord == c) {
            return p;
        }
    }
    return nullptr;
}

void Level::addBonas()
{
    for (uint32_t i = 0; i < stage; ++i) {
        Coord c = freePosition();
        auto b = new Bona(c);
        items.push_back(b);
    }
}

void Level::addItem(ItemType item)
{
    Coord c = freePosition();

    Item* i;
    switch(item)
    {
        default:
            printer->printErr("ERROR: addItem\n");
            i = nullptr;
            break;
        case EKalja:
            i = new Kalja(c);
            break;
        case ELonkka:
            i = new Lonkka(c);
            break;
        case ELenkki:
            i = new Lenkki(c);
            break;
        case EPamppu:
            i = new Pamppu(c);
            break;
        case EVeitsi:
            i = new Veitsi(c);
            break;
        case EKetjut:
            i = new Ketjut(c);
            break;
        case EKivi:
            i = new Kivi(c);
            break;
        case EBootsit:
            i = new Bootsit(c);
            break;
        case EKala:
            i = new Kala(c);
            break;
        case EOmppo:
            i = new Omppo(c);
            break;
        case EBanaani:
            i = new Banaani(c);
            break;
        case ERaha:
            i = new Raha(c);
            break;
        case EPaska:
            i = new Paska(c);
            break;
        case EViesti:
            i = new Viesti(c);
            break;
        case EPensas:
            i = new Pensas(c);
            break;
        case ERoskis:
            i = new Roskis(c);
            break;
        case EPistooli:
            i = new Pistooli(c);
            break;
        case EPropusk:
            i = new Propusk(c);
            break;
        case EGranata:
            i = new Granata(c);
            break;
        case EVodka:
            i = new Vodka(c);
            break;
    }
    if (i) {
        addItem(i);
    }
}

bool Level::addBuilding(Building* building)
{
    if (!building) return false;

    building->level = this;
    buildings.push_back(building);
    return true;
}

bool Level::addPerson(Person* person)
{
    for (const auto &p : persons)
    {
        if ((p->coord.x == person->coord.x) && (p->coord.y == person->coord.y))
            return false;
    }

    person->setLevel(this);
    persons.push_back(person);
    return true;
}

void Level::addItem(Item* item)
{
    items.push_back(item);
}

void Level::addThrownItem(Item* item)
{
    thrownItem = item;
}

void Level::addVehicle(Vehicle* v)
{
    v->level = this;
    vehicles.push_back(v);
}

void Level::updateVehicles()
{
    for (auto& v : vehicles)
    {
        v->update();
    }
}

Vehicle* Level::getVehicleAt(const Coord& c) const
{
    for (auto& v : vehicles)
    {
        if (v->hitVehicle(c)) return v;
    }
    return nullptr;
}

void Level::addAnimation(Animation* animation, bool play)
{
    if (!common::animsEnabled) {
        delete animation;
        return;
    }

    constexpr size_t MAX_ANIMATIONS = 24;
    if (animations.size() >= MAX_ANIMATIONS) {
        delete animations.front();
        animations.erase(animations.begin());
    }
    animations.push_back(animation);

    if (play) {
        playAnimation();
    }
}

void Level::removeItem(Item* item)
{
    delete item;
    auto it = find(items.begin(), items.end(), item);
    if (it != items.end())
    {
        items.erase(it);
    }
}

// walls block LOS, but windows and doors don't
bool Level::blocksLOS(const Coord& c) const
{
    if (hitLOSBlocker(c)) return true;

    for (const auto& b : buildings)
    {
        if (b->hitBuilding(c))
        {
            if (b->hitWall(c) && !b->hitWindow(c) && !b->hitDoor(c))
                return true;
        }
    }
    return false;
}

// return true if visibility ok
bool Level::raycast(Coord from, const Coord& to) const
{
    bool originBuilding = hitBuilding(to);

    uint32_t block = 0;
    uint32_t dx = std::max(from.x, to.x) - std::min(from.x, to.x);
    uint32_t dy = std::max(from.y, to.y) - std::min(from.y, to.y);
    float ang = (dy == 0) ? 10.f : (float)((float)dx / (float)dy);

    if (ang < 1.f) {
        bool ret = (from == to);
        float k = (ang == 0) ? 100 : 1.f / ang;

        while(from != to) {
            uint32_t dx = std::max(from.x, to.x) - std::min(from.x, to.x);
            uint32_t dy = std::max(from.y, to.y) - std::min(from.y, to.y);
            float ang = (dy == 0) ? 999.f : (float)((float)dx / (float)dy);
            k = (ang == 0) ? (float)dy / 2.f : 1.f / ang;
            k = ((int)k == 0) ? 1 : k;

            for (int i = (int)k; i > 0; --i)
            {
                if (from.y < to.y) ++from.y;
                else if (from.y > to.y) --from.y;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
                if (block > 0) {
                    break;
                }
            }
            if (block <= 1)
            {
                if (from.x < to.x) ++from.x;
                else if (from.x > to.x) --from.x;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
            }

            if (from == to)
            {
                ret = true;
                break;
            }
            if ((block <= 1) && hitPerson(from) && (to == from)) {
                ret = true;
                break;
            } else if (block > 0) {
                ret = false;
                break;
            }
        }

        // always show building internals
        if (originBuilding) ret = true;

        return ret;

    } else if (ang >= 1.f) {
        bool ret = (from == to);
        float k = ang;

        while(from != to) {

            uint32_t dx = std::max(from.x, to.x) - std::min(from.x, to.x);
            uint32_t dy = std::max(from.y, to.y) - std::min(from.y, to.y);
            float ang = (dy == 0) ? 100 : (float)((float)dx / (float)dy);
            k = ang;

            for (int i = (int)k; i > 0; --i)
            {
                if (from.x < to.x) ++from.x;
                else if (from.x > to.x) --from.x;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
                if (block > 0) {
                    break;
                }
            }
            if (block <= 1)
            {
                if (from.y < to.y) ++from.y;
                else if (from.y > to.y) --from.y;
                block += (blocksLOS(from) || hitPerson(from)) ? 1 : 0;
            }

            if (from == to)
            {
                ret = true;
                break;
            }
            if ((block <= 1) && hitPerson(from) && (to == from)) {
                ret = true;
                break;
            } else if (block > 0) {
                ret = false;
                break;
            }
        }
        // always show building internals
        if (originBuilding) ret = true;

        return ret;
    }

    return true;
}

Person* Level::raycast(const Coordinate<int>& from, const Coordinate<int>& vector) const
{
    if (vector == Coordinate<int>(0, 0)) return nullptr;

    Coordinate<int> pos = from;
    pos.x += vector.x;
    pos.y += vector.y;

    while (true) {
        if (pos.x < 0 || pos.y < 0 ||
            pos.x >= (int)sizex || pos.y >= (int)sizey) {
            return nullptr;
        }

        Coord c { (uint32_t)pos.x, (uint32_t)pos.y };

        if (hitBuilding(c) || hitLOSBlocker(c)) return nullptr;

        Person* p = getPerson(c);
        if (p) return p;

        pos.x += vector.x;
        pos.y += vector.y;
    }
}


Building* Level::raycastBuilding(const Coordinate<int>& from, const Coordinate<int>& vector) const
{
    return nullptr;
}
