#include "persons.hpp"
#include "player.hpp"
#include "level.hpp"
#include "item.hpp"
#include "common.hpp"
#include "coord.hpp"
#include "animation.hpp"
#include "pathmask.hpp"
#include "vehicle.hpp"
#include <string>
#include <iostream>
#include <chrono>

using std::string;
using PlayerNS::Player;
using std::cout;
using std::endl;
using std::max;
using std::min;
using DirectionNS::Direction;
using common::random;

void Person::powerup(uint32_t damage)  {
    stats.health += damage;

#ifdef ANIMATIONS_ENABLED
    string text = "+" + std::to_string(damage) + "hp";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a);
#endif
}

void Person::damage(uint32_t damage)  {
    // in real time the player gets a moment to react after each hit
    if (type == pelaaja && common::realtimeEnabled) {
        using clock = std::chrono::steady_clock;
        static clock::time_point lastHit;
        const auto now = clock::now();
        if (now - lastHit < std::chrono::milliseconds(700)) return;
        lastHit = now;
    }
    common::hitEvent(coord, damage);
    if (damage >= stats.health) {
        stats.health = 0;
    } else {
        stats.health -= damage;
    }

#ifdef ANIMATIONS_ENABLED
    string text = "-" + std::to_string(damage) + "hp";
    uint32_t diff = text.length() / 2;
    diff = (coord.x < diff) ? 0 : coord.x - diff;
    Coord textCoord { diff, coord.y };
    Animation* a = new Animation(textCoord, text, 5);
    a->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(a);
#endif
}

Person* Person::currentTarget()
{
    if (!level->isAlive(chaseTarget)) chaseTarget = nullptr;
    return chaseTarget;
}

bool Person::canSeeTarget(const Coord& target) const
{
    if (blind) return false;
    if (level->playerDead && level->getPlayer() && target == level->getPlayer()->coord) return false;
    // someone standing in a bush can only be spotted from right next to it
    if (level->hitLOSBlocker(target) && coord.distance(target) > 1) return false;
    if (visionRange == 0) return true;
    // lit places are seen from twice as far
    const uint32_t range = level->isLit(target) ? visionRange * 2 : visionRange;
    if (coord.distance(target) > range) return false;
    return level->raycast(coord, target);
}

void Person::report(const std::string& msg)
{
    if (msg.empty() || level->playerDead) return;
    const Person* p = level->getPlayer();
    if (!p) return;
    const bool seen = coord.distance(p->coord) <= 12 && level->raycast(p->coord, coord);
    if (seen || level->canHear(p->coord, coord, 15)) printer->showMessage(msg, level);
}

Direction Person::randomDirection() const
{
    switch (std::rand() % 4) {
        case 0: return DirectionNS::up;
        case 1: return DirectionNS::down;
        case 2: return DirectionNS::left;
        default: return DirectionNS::right;
    }
}

// Speech bubbles are shown in Latin letters: Russian lines are romanised the Finnish way.
static string latinize(const string& in)
{
    static const char* lower[] = {
        "a", "b", "v", "g", "d", "e", "\xC5\xBE", "z", "i", "i", "k", "l", "m", "n", "o", "p",
        "r", "s", "t", "u", "f", "h", "ts", "t\xC5\xA1", "\xC5\xA1", "\xC5\xA1t\xC5\xA1", "", "y", "", "e", "ju", "ja",
    };
    static const char* upper[] = {
        "A", "B", "V", "G", "D", "E", "\xC5\xBD", "Z", "I", "I", "K", "L", "M", "N", "O", "P",
        "R", "S", "T", "U", "F", "H", "Ts", "T\xC5\xA1", "\xC5\xA0", "\xC5\xA0t\xC5\xA1", "", "Y", "", "E", "Ju", "Ja",
    };

    string out;
    for (size_t i = 0; i < in.size(); ++i) {
        const unsigned char b0 = in[i];
        const unsigned char b1 = i + 1 < in.size() ? (unsigned char)in[i + 1] : 0;
        uint32_t cp = 0;
        if (b0 == 0xD0 || b0 == 0xD1) cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        if (cp >= 0x410 && cp <= 0x42F)      { out += upper[cp - 0x410]; ++i; }
        else if (cp >= 0x430 && cp <= 0x44F) { out += lower[cp - 0x430]; ++i; }
        else if (cp == 0x401)                { out += "Jo"; ++i; }
        else if (cp == 0x451)                { out += "jo"; ++i; }
        else out += in[i];
    }
    return out;
}

static uint32_t columns(const string& s)
{
    uint32_t n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
    return n;
}

void Person::say(const std::string& raw)
{
    const string text = latinize(raw);
    const uint32_t half = columns(text) / 2;
    uint32_t bx = (coord.x > half) ? coord.x - half : 0;
    uint32_t by = (coord.y > 1) ? coord.y - 1 : 0;
    Coord bubblePos { bx, by };

    // the bubble stays put and fades: ~2.5 s in real time, a few moves turn-based
    Animation* anim = new Animation(bubblePos, text, common::realtimeEnabled ? 50 : 4);
    anim->isBubble = true;
    anim->owner = this;
    anim->source = coord;
    for (auto it = level->animations.begin(); it != level->animations.end();) {
        if ((*it)->owner == this) { delete *it; it = level->animations.erase(it); }
        else ++it;
    }
    level->addAnimation(anim, false);
}

Direction Person::getMoveDirection(const Coord& target) const
{
    if (pathFinding) {
        // use advanced pathfinding
        PathNS::PathMask mask(level);
        pathmaskHook(&mask);
        return mask.firstStep(coord, target);

    } else {
        // use classic pultsari 1.0 pathfinding
        auto dx = max(coord.x, target.x) - (min(coord.x, target.x));
        auto dy = max(coord.y, target.y) - (min(coord.y, target.y));

        if (dx > dy) {
            if (coord.x > target.x) {
                return DirectionNS::left;
            } else if (coord.x < target.x) {
                return DirectionNS::right;
            } else if (coord.y > target.y) {
                return DirectionNS::up;
            } else if (coord.y < target.y) {
                return DirectionNS::down;
            }
        } else {
            if (coord.y < target.y) {
                return DirectionNS::down;
            } else if (coord.y > target.y) {
                return DirectionNS::up;
            } else if (coord.x > target.x) {
                return DirectionNS::left;
            } else if (coord.x < target.x) {
                return DirectionNS::right;
            }
        }
    }

    return DirectionNS::none;
}

// MUMMO
Mummo::Mummo(const Coord& pos) : Person(mummo, pos)
{
    stats.health = 3;
    visionRange = 6;
}

bool Mummo::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }
    if (!level->hit(check)) {
        coord = check;
    }
    common::checkBounds(coord);
    return true;
}

void Mummo::npcAct()
{
    Player* p = level->getPlayer();
    if (!p) return;

    // if player returned from war (wearing SMO uniform) and mummo is close
    if (p->returnedFromWar && !reactedToUniform && canSeeTarget(p->coord) &&
        coord.distance(p->coord) <= 3) {
        reactedToUniform = true;
        if (random(50)) {
            say("Kiitos palveluksestasi, poikaseni!");
            p->inventory.vodka++;
            p->updatepromilles(3);
        } else {
            say("MURHAAJA! Tapoit poikani!");
            p->damage(std::rand()%3 + 2);
        }
        return;
    }

    // slow random walk
    if (random(10)) {
        string msg = "";
        Direction d = randomDirection();
        move(d, msg);
    }

    if (random(5)) {
        static const char* mummo_lines[] = {
            "Ai nykyajan nuoriso...",
            "Herra jumala...",
            "Kaikki oli paremmin ennen.",
        };
        say(mummo_lines[std::rand()%3]);
        level->audio.addSound(coord, SoundType::Voice,
            "Mummo mutisee", "Mutinaa", "...mutinaa...", 8);
    }
}

bool Mummo::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        if (random(50))
        {
            msg = "Eläkellääinen hakkaa sua käsilaukullaan !!!";
            uint32_t damage = std::rand() % 1 + 1;
            source->damage(damage);
        } else {
            msg = "Mukiloit mummon kumoon. Haa,lompsa !!!";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.mummot;
        }

        level->alertCops(source);
    } else if (source->type == varas) {
        msg = "Mummo jäi varkaan kynsiin!";
        level->alertCops(source);
        damage(stats.health);
    }

    return true;
}

bool Mummo::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(50);
    if (source->type == pelaaja) {
        if (!hit) {
            if (source->type == pelaaja) msg = "Et osunut mummoon!";
        } else {
            msg = "Mummo heitti kuperkeikan ja hukkasi käsilaukkunsa.";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.mummot;
            level->alertCops(source);

            auto i = dropItem();
            level->addItem(i);
        }
    }
    return hit;
}

bool Mummo::interactpuke(Person* source)
{
    report("Vasja kaatui mummon päälle. Mummo kuoli. Русский мир.");
    damage(stats.health);
    return true;
}

Item* Mummo::dropItem()
{
    return new Raha(coord, std::rand()%25 + 1);
}

// COP
Cop::Cop(const Coord& pos) : Person(poliisi, pos)
{
    visionRange = 10;
    hasRadio = true;
}

bool Cop::move(Direction d, std::string& msg)
{
    Coord check = coord;
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            p->interact(msg, this);
        } else if (!level->hitBuilding(check)) {
            coord = check;
        }
    }
    common::checkBounds(coord);
    return true;
}

void Cop::pathmaskHook(PathNS::PathMask* pathmask) const
{
    pathmask->skipConvict = true;
    pathmask->skipVaras = true;
}

void Cop::npcAct()
{
    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (knowsPutinDead) {
        if (random(20)) {
            static const char* party[] = {
                "*hic* VAPAUS!",
                "Putin on kuollut! *RÖYH*",
                "Vodkaa kaikille!",
                "*horskaa* Elämä on kaunista!",
                "Kenraalit juoksee! *hic*",
                "Ei enää sakkoja! *hic*",
                "СВОБОДА! *röyh*",
            };
            say(party[std::rand()%7]);
        }
        if (random(30)) {
            Direction d = randomDirection();
            move(d, msg);
        }
        if (random(15)) level->audio.addSound(coord, SoundType::Voice,
            "Humalainen politsija laulaa", "Laulua jostain", "...laulua...", 18);
        report(msg);
        return;
    }

    Person* target = currentTarget();
    if (!target && alerted) {
        // heard a colleague ordering someone about: go and back them up
        if (coord.distance(lastKnownPos) <= 2) {
            static const char* backup[] = {
                "Ongelmia, kollega?",
                "Dokumenty!",
                "Mit\xC3\xA4 t\xC3\xA4\xC3\xA4ll\xC3\xA4 tapahtuu?",
            };
            say(backup[std::rand() % 3]);
            alerted = false;
        } else {
            move(getMoveDirection(lastKnownPos), msg);
        }
    } else if (!target)
    {
        if (seesPlayer && coord.distance(p->coord) == 1)
        {
            // colleagues within earshot hear the orders and come to help
            if (random(30)) level->callForHelp(this, p, false);
            {
                int roll = std::rand() % 100;
                if (roll < 30) {
                    // corrupt cop demands bribe
                    static const char* bribes[] = {
                        "Maksa tai rintamalle!",
                        "Katutullia pitää maksaa!",
                        "'Vapaaehtoinen' lahjoitus armeijalle?",
                        "Sinulla on epäilyttävä olemus. Maksa.",
                        "Деньги давай!",
                        "Пропуск! Покажи пропуск!",
                    };
                    say(bribes[std::rand()%6]);
                    if (p->stats.money >= 10) {
                        uint32_t bribe = std::min(p->stats.money, (uint32_t)(rand()%20 + 10));
                        p->updateMoney(-(int)bribe);
                        say(std::to_string(bribe) + " ruplaa. Jatka matkaa.");
                    } else {
                        say("Ei rahaa? Rintamalle!");
                        p->inJail = true;
                    }
                } else if (roll < 50 && p->stats.promilles > 15) {
                    // stupid fine for being drunk
                    static const char* fines[] = {
                        "Sakko: julkijuopumus!",
                        "Sakko: häiriökäyttäytyminen!",
                        "Sakko: kävely ilman passia!",
                        "Sakko: ulkonaliikkumiskiellon rikkominen!",
                    };
                    say(fines[std::rand()%4]);
                    if (p->stats.money >= 5) {
                        uint32_t fine = std::min(p->stats.money, (uint32_t)(rand()%15 + 5));
                        p->updateMoney(-(int)fine);
                    } else {
                        say("Ei rahaa sakkoihin? Putkaan!");
                        p->inJail = true;
                    }
                } else if (roll < 60 && p->stats.promilles > 25) {
                    // cart drunk to the front
                    say("Juoppo! Tarvitaan miehiä rintamalle!");
                    p->inJail = true;
                } else {
                    static const char* idle[] = {
                        "Liiku eteenpäin!",
                        "Älä jää tuijottamaan!",
                        "Täällä ei ole mitään nähtävää!",
                        "Paperit kuntoon!",
                        "Иди сюда!",
                        "Документы!",
                    };
                    say(idle[std::rand()%6]);
                }
            }
        } else if (seesPlayer && coord.distance(p->coord) <= 5) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        } else {
            // patrol - cops wander actively looking for trouble
            if (random(60)) {
                Direction d = randomDirection();
                move(d, msg);
            }
        }
    } else {
        bool seesTarget = (target == p) ? seesPlayer : canSeeTarget(target->coord);

        if (seesTarget) {
            if (!alerted) level->callForHelp(this, target, true);
            alerted = true;
            lastKnownPos = target->coord;
            alertCooldown = 15;
        }

        if (alerted) {
            Coord chasePos = seesTarget ? target->coord : lastKnownPos;

            if (seesTarget && coord.distance(target->coord) == 1)
            {
                if (random(35)) {
                    say("Pamppu maistuu!");
                    target->damage(rand()%2+1);
                }
            } else {
                Direction d = getMoveDirection(chasePos);
                move(d, msg);
            }

            if (!seesTarget && coord.distance(lastKnownPos) <= 1) {
                if (alertCooldown > 0) {
                    --alertCooldown;
                } else {
                    alerted = false;
                }
            }
        } else {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    if (alerted) {
        level->audio.addSound(coord, SoundType::Siren,
            "Politsija huutaa: СТОЙ!", "Politsijan huutoa", "...huutoa kaukaa...", 25);
    } else if (random(8)) {
        level->audio.addSound(coord, SoundType::WalkieTalkie,
            "Radiopuhelin rahinaa", "Radion rahinaa", "...rahinaa...", 15);
    }
    report(msg);
}

// a cop who is attacked chases the attacker and shouts for help; if knocked
// out, whoever saw or heard the fight reacts instead
static void copAttacked(Person* cop, Person* source)
{
    if (cop->stats.health > 0) {
        cop->chaseTarget = source;
        cop->alerted = true;
        cop->lastKnownPos = source->coord;
        cop->alertCooldown = 20;
        cop->level->callForHelp(cop, source, true);
    } else {
        cop->level->alertCops(source);
    }
}

bool Cop::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        if (random(50))
        {
            msg = "Huitaiset politsijaa!";
        } else {
            msg = "Politsija kellahtaa ketoon!!!";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        copAttacked(this, source);
    } else if (source->type == varas) {
        msg = "Politsija pidätti varkaan!";
        source->damage(stats.health);
    }

    return true;
}

bool Cop::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Politsija sai osuman ja pudotti pampun.";
    damage(1);

    if (source->type == pelaaja) {
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }

    copAttacked(this, source);

    auto i = dropItem();
    level->addItem(i);
    return true;
}

bool Cop::interactpuke(Person* source)
{
    printer->showMessage("Politsija tukehtui oksennukseen. Muut politsijat eivät taida tykätä sinusta.", level);
    damage(stats.health);
    copAttacked(this, source);
    return true;
}

Item* Cop::dropItem()
{
    return new Pamppu(coord);
}


// VARAS
Varas::Varas(const Coord& pos) : Person(varas, pos), target(pos)
{
    visionRange = 8;
}

bool Varas::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    check -= { 0, 1 }; break;
        case DirectionNS::right: check += { 1, 0 }; break;
        case DirectionNS::down:  check += { 0, 1 }; break;
        case DirectionNS::left:  check -= { 1, 0 }; break;
        default: return false;
    }

    if (!level->hit(check)) {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            p->interact(msg, this);
        }

        auto i = level->getItem(check);
        if (i)
        {
            if (i->type == EPaska)
            {
                msg += (msg == "") ? "" : "\n";
                msg += "Varas liukastui paskaan, ja kuoli.";
                i->discard = true;
                damage(stats.health);
            } else if (!level->hitBuilding(check)) {
                coord = check;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Varas::pathmaskHook(PathNS::PathMask* pathmask) const
{
    pathmask->skipCop = true;
}

bool Varas::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(30);
    if (source->type == pelaaja) {
        if (!hit) {
            msg = "Et osunut varkaaseen.";
        } else {
            if (random(50)) msg = "Varas ulvahtaa kivusta kun osut häntä munille.";
            else msg = "Osuit varkaaseen, joka kupsahti.";
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            damage(stats.health);
        }
    } else {
        if (!hit) {
        } else {
            if (random(50)) msg = "Varas ulvahtaa kivusta kun häntä osuu munille.";
            else msg = "Varkaaseen osui ja se kupsahti.";
            damage(stats.health);
        }
    }
    return hit;
}

void Varas::npcAct()
{
    string msg = "";
    Player* player = level->getPlayer();
    bool seesPlayer = canSeeTarget(player->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = player->coord;
        alertCooldown = 10;
        target = player->coord;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? player->coord : lastKnownPos;

        if (seesPlayer && coord.distance(player->coord) == 1)
        {
            say("Hipelöin sua!!");
            msg = "Varas hipelöi sinua !!!";

            auto r = std::rand();
            if (r % 2)
            {
                uint32_t sum = r % 16 + 1;
                if (sum > player->stats.money) sum = player->stats.money;
                player->updateMoney(-sum);
                msg = "Lompakkosi kevenee " + std::to_string(sum) + " ruplalla.";
                damage(stats.health);
            }
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(30)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Varas::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        if (random(50))
        {
            msg = "VARAS suuttuu sinulle!";
        } else {
            msg = "VARAS kellahtaa ketoon !!!";
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            damage(stats.health);
        }
    } else if (source->type == poliisi) {
        msg = "Politsija pidätti varkaan!";
        damage(stats.health);
    }

    return true;
}


// VANKI
Vanki::Vanki(const Coord& pos) : Person(vanki, pos), target(pos)
{
    stats.health = 4;
    visionRange = 8;
}

bool Vanki::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            if (p->type == poliisi) {
                msg = "Politsija hakkaa vankikarkurin! Rintamalle hänet.";
                damage(stats.health);
            } else if (p->type == mummo) {
                msg = "Vankikarkuri tönäisee muorin nurin!";
                p->damage(p->stats.health);
                level->alertCops(this);
            }
        }

        auto i = level->getItem(check);
        if (!p && i)
        {
            switch(i->type) {
                case EPaska:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri liukastuu paskaan";
                    i->discard = true;
                    coord = check;
                    damage(1);
                    if (stats.health == 0)
                    {
                        msg += "\nja vankikarkuri kuoli.";
                    }
                    break;
                case ELenkki:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri ahmii kiekuralenkin ja voimistuu";
                    ++stats.health;
                    i->discard = true;
                    coord = check;
                    break;
                case EKivi:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vankikarkuri nappaa kiven.";
                    i->discard = true;
                    kivi = true;
                    coord = check;
                    break;
                default:
                    if (!level->hitBuilding(check)) coord = check;
                    break;
            }
        } else if (!level->hitBuilding(check)) {
            coord = check;
        }
    }
    common::checkBounds(coord);
    return true;
}

void Vanki::pathmaskHook(PathNS::PathMask* pathmask) const
{
    pathmask->skipCop = true;
}

void Vanki::npcAct()
{
    string msg = "";
    if (firstMove)
    {
        firstMove = false;
        return;
    }

    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 10;
        target = p->coord;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1)
        {
            msg = "Vankikarkuri kurmottaa sinua!";
            uint32_t damage = std::rand()%3 + 1;
            p->damage(damage);
        } else if (seesPlayer && kivi && ((coord.x == p->coord.x) || (coord.y == p->coord.y))) {
            kivi = false;
            int dx = coord.x - p->coord.x;
            int dy = coord.y - p->coord.y;
            dx = (dx < 0) ? 1 : (dx > 0) ? -1 : 0;
            dy = (dy < 0) ? 1 : (dy > 0) ? -1 : 0;

            Coordinate<int> c { static_cast<int>(dx), static_cast<int>(dy) };
            Item* i = Item::createItem(6, coord);
            i->thrown = true;
            i->throwVec = c;
            level->addThrownItem(i);
            level->actThrow(this);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(25)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Vanki::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        msg = "Hakkaat hullun lailla vankikarkuria!";
        damage(1);
    } else if (source->type == poliisi) {
        msg = "Politsija ottaa vankikarkurin hallintaotteeseen!!!";
        damage(stats.health);
    }

    if (stats.health == 0)
    {
        msg = msg + "\n Vanki kellahtaa ketoon !!!";
        damage(stats.health);

        if (source->type == pelaaja) {
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
    }

    return true;
}

bool Vanki::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(50);
    if (!hit) {
        msg = "Roisto väisti heittosi !";
    } else {
        uint32_t d = std::rand() % 3 + 1;
        if (d >= stats.health) {
            msg = "Vankikarkuri pokertyy heittosi ansiosta maahan...";
        } else {
            msg = "Osuit vankikarkuriin, mutta hän näyttää entistä vihaisemmalta!";
            if (source->type == pelaaja) {
                ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            }
        }
        damage(d);
    }
    return hit;
}

// SKINHEAD
Skinhead::Skinhead(const Coord& pos) : Person(skinhead, pos)
{
    stats.health = 7;
    visionRange = 7;
}

bool Skinhead::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            if (p->type == poliisi) {
                msg = "Skinhead uhoaa politsijalle!";
            }
        }

        auto i = level->getItem(check);
        if (i)
        {
            switch(i->type) {
                case EPaska:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Skini näkee maassa kikkareen ja päättää maistaa sitä.";
                    damage(1);
                    if (stats.health == 0)
                    {
                        msg += "\nSkinhead tukehtui paskaan.";
                    }
                    break;
                case ELenkki:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Skinin massa kasvaa pilottitakin alla.";
                    ++stats.health;
                    i->discard = true;
                    break;
                default:
                    coord = check;
                    break;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Skinhead::npcAct()
{
    if (firstMove)
    {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 12;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1)
        {
            say("TAPAN SINUT!!");
            msg = "Skinhead murjoo sinua apinan raivolla!!";
            uint32_t damage = std::rand()%4 + 1;
            p->damage(damage);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(25)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Skinhead::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        msg = "Pusket SHITHEADia naamaan,mutta sehan murjoo myos sua.";
        damage(1);

        if (stats.health == 0)
        {
            msg = "Osuit skiniä kiveksiin !!!";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
    }

    return true;
}

bool Skinhead::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(67);
    if (!hit) {
        msg = "Et osunut skinheadiin !";
    } else {
        uint32_t d = std::rand() % 5 + 1;
        if (d >= stats.health) {
            msg = "Heittosi ossuupi skinukkelia vyon alle. Se koskee tuntuvasti !!!";
        } else {
            msg = "Osuit skiniin, mutta hän näyttää entistä vihaisemmalta!";
            if (source->type == pelaaja) {
                ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
            }
        }
        damage(d);
    }
    return hit;
}

// YKÄ
Yka::Yka(const Coord& pos) : Person(yka, pos)
{
    stats.health = 1;
    visionRange = 6;
}

bool Yka::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check)) {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p)
        {
            if (p->type == pelaaja) {
                bool received = p->interact(msg, this);
                kaljaKesken = received ? 5 : 1;
            } else if (p->type == poliisi) {
                if (!puhallus) {
                    msg = "Vasja kysyy politsijalta, saisiko puhaltaa.";
                    puhallus = true;
                }
            }
        }

        auto i = level->getItem(check);
        if (i)
        {
            switch(i->type) {
                case EKalja:
                    msg += (msg == "") ? "" : "\n";
                    msg += "Vasja löysi maasta kaljan ja lähti lipittämään sitä.";
                    damage(1);
                    break;
                default:
                    coord = check;
                    break;
            }
        }

    }
    common::checkBounds(coord);
    return true;
}

void Yka::npcAct()
{
    string msg = "";

    if (kaljaKesken > 0)
    {
        --kaljaKesken;
        return;
    }

    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer && p->inventory.kalja > 0) {
        Direction d = getMoveDirection(p->coord);
        move(d, msg);
    } else {
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Yka::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja)
    {
        Player* p = dynamic_cast<Player*>(source);
        if (p->inventory.kalja > 0)
        {
            msg = "Huitaiset ykaa,joka pollii sulta yhen kaljan.";
            --p->inventory.kalja;
            damage(stats.health);
        } else {
            msg = "Mätät Vasjaa sikana naamaan!";
        }
    }

    return true;
}

bool Yka::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(67);
    if (!hit) {
        msg = "Vasja ihmettelee, mikä suhahti pään vierestä!";
        return false;
    } else {
        msg = "Vasja simahtaa!";
        damage(stats.health);
        if (source->type == pelaaja) {
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
    }
    return hit;
}


// JENGI (Gang member)
Jengi::Jengi(const Coord& pos) : Person(jengi, pos)
{
    stats.health = 5;
    visionRange = 8;
}

bool Jengi::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check)) {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p && p->type == poliisi) {
            msg = "Gopnik ja politsija tappelevat!";
        }
    }
    common::checkBounds(coord);
    return true;
}

void Jengi::npcAct()
{
    if (firstMove) { firstMove = false; return; }

    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 8;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1) {
            auto r = std::rand() % 3;
            if (r == 0) {
                uint32_t sum = std::rand() % 20 + 5;
                if (sum > p->stats.money) sum = p->stats.money;
                p->updateMoney(-(int)sum);
                msg = "Gopnik ryöstää " + std::to_string(sum) + " ruplaa! \"Семки есть?\"";
            } else if (r == 1) {
                uint32_t dmg = std::rand() % 3 + 1;
                p->damage(dmg);
                msg = "Gopnik lyö sinua nyrkillä!";
            } else {
                msg = "Gopnik kyykkii ja uhkailee: \"Есть чё?\"";
            }
            level->alertCops(this);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(30)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Jengi::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (random(40)) {
            msg = "Gopnik iskee takaisin!";
            source->damage(std::rand()%2+1);
        } else {
            msg = "Moukaroit gopnikin tajuttomaksi!";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    }
    return true;
}

bool Jengi::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(50);
    if (!hit) {
        msg = "Gopnik väisti!";
    } else {
        msg = "Gopnik sai osuman ja kaatuu!";
        damage(stats.health);
        if (source->type == pelaaja)
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return hit;
}


// RYÖSTÄJÄ (Armed robber)
Ryostaja::Ryostaja(const Coord& pos) : Person(ryostaja, pos)
{
    stats.health = 3;
    visionRange = 9;
}

bool Ryostaja::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check)) {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p && p->type == poliisi) {
            msg = "Ryöstäjä ampuu politsijaa!";
            p->damage(p->stats.health);
            level->alertCops(this);
        }
    }
    common::checkBounds(coord);
    return true;
}

void Ryostaja::npcAct()
{
    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer) {
        alerted = true;
        lastKnownPos = p->coord;
        alertCooldown = 12;
    }

    if (alerted) {
        Coord chasePos = seesPlayer ? p->coord : lastKnownPos;

        if (seesPlayer && coord.distance(p->coord) == 1) {
            if (armed) {
                say("RAHAT TAI HENKI!");
                msg = "Ryöstäjä osoittaa sinua aseella: RAHAT TAI HENKI!";
                if (p->stats.money > 0) {
                    uint32_t take = std::min(p->stats.money, (uint32_t)(rand()%30 + 10));
                    p->updateMoney(-(int)take);
                    msg = "Ryöstäjä vie " + std::to_string(take) + " ruplaa aseella uhaten!";
                } else {
                    p->damage(std::rand()%3+2);
                    msg = "Ryöstäjä ampuu sinua kun ei ole rahaa!";
                }
                level->alertCops(this);
                level->audio.addSound(coord, SoundType::Gunshot,
                    "PAM! Pistooli pamahtaa!", "Pistoolinlaukaus", "...pamaus kaukaa...", 25);
            } else {
                p->damage(std::rand()%2+1);
                msg = "Ryöstäjä lyö sinua!";
            }
        } else if (seesPlayer && coord.distance(p->coord) <= 4 && armed && random(20)) {
            p->damage(std::rand()%2+1);
            msg = "Ryöstäjä ampuu sinua kaukaa!";
            level->alertCops(this);
            level->audio.addSound(coord, SoundType::Gunshot,
                "PAM! Pistooli pamahtaa!", "Pistoolinlaukaus", "...pamaus kaukaa...", 25);
        } else {
            Direction d = getMoveDirection(chasePos);
            move(d, msg);
        }

        if (!seesPlayer && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
    } else {
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Ryostaja::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (random(60)) {
            msg = "Ryöstäjä ampuu sinua!";
            source->damage(std::rand()%3+1);
            armed = false;
        } else {
            msg = "Kaadoit ryöstäjän! Ase putoaa maahan.";
            damage(stats.health);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    }
    return true;
}

bool Ryostaja::interactThrow(Item* item, Person* source, std::string& msg)
{
    bool hit = random(40);
    if (!hit) {
        msg = "Ryöstäjä väistää heiton!";
    } else {
        msg = "Ryöstäjä kaatuu ja pudottaa aseensa!";
        damage(stats.health);
        auto i = dropItem();
        level->addItem(i);
        if (source->type == pelaaja)
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return hit;
}

Item* Ryostaja::dropItem()
{
    return new Raha(coord, std::rand()%30 + 10);
}


// DIILERI (Drug dealer)
Diileri::Diileri(const Coord& pos) : Person(diileri, pos)
{
    stats.health = 2;
    visionRange = 7;
}

bool Diileri::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check)) {
        coord = check;
    }
    common::checkBounds(coord);
    return true;
}

void Diileri::npcAct()
{
    string msg = "";
    Player* p = level->getPlayer();
    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer && coord.distance(p->coord) == 1 && !offered) {
        offered = true;
        if (common::realtimeEnabled) {
            if (p->stats.money >= 10 && random(50)) {
                p->updateMoney(-10);
                if (random(30)) {
                    say("Kokeile t\xC3\xA4t\xC3\xA4... heh heh");
                    msg = "Diileri: Kokeile tätä... Pääsi pyörii!";
                    p->updatepromilles(8);
                    p->damage(1);
                } else {
                    say("T\xC3\xA4ss\xC3\xA4 on hyv\xC3\xA4\xC3\xA4 kamaa!");
                    msg = "Diileri: Tässä on hyvää kamaa! Olet energinen!";
                    p->powerup(3);
                    p->stats.hunger += 20;
                    if (p->stats.hunger > 100) p->stats.hunger = 100;
                }
            } else {
                say("Hei, haluatko ostaa jotain?");
                msg = "Diileri: Hei, haluatko ostaa jotain?";
            }
        } else {
            say("Hei, haluatko ostaa jotain?");
            msg = "Diileri: Hei, haluatko ostaa jotain?";
        }
    } else if (seesPlayer && coord.distance(p->coord) <= 3) {
        if (random(40)) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        }
    } else {
        offered = false;
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Diileri::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (random(70)) {
            msg = "Diileri karkaa juosten!";
            damage(stats.health);
        } else {
            msg = "Löylyytät diilerin ja löydät rahaa!";
            damage(stats.health);
            dynamic_cast<Player*>(source)->updateMoney(std::rand()%20+5);
            ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
        }
        level->alertCops(source);
    }
    return true;
}

bool Diileri::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Diileri kaatuu ja karkaa!";
    damage(stats.health);
    if (source->type == pelaaja)
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    return true;
}

// VETERAANI (SMO Veteran)
Veteraani::Veteraani(const Coord& pos) : Person(veteraani, pos)
{
    stats.health = 12;
    visionRange = 10;
    // some came back from the front blind and go by ear, some on one leg
    blind = random(25);
    oneLegged = random(30);
}

std::string Veteraani::typeToChar() const
{
    return aggressive ? "W" : "Z";
}

bool Veteraani::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p && aggressive)
        {
            if (p->type == pelaaja) {
                msg = "Veteraani ampuu sinua!";
                p->damage(std::rand()%5 + 3);
            } else if (p->type != veteraani) {
                msg = "Veteraani raivostuu ja hyökkää!";
                p->damage(std::rand()%6 + 2);
            }
        }
    }
    common::checkBounds(coord);
    return true;
}

void Veteraani::npcAct()
{
    if (firstMove)
    {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();
    // a blind veteran only notices you by your footsteps, close by
    bool seesPlayer = blind ? level->canHear(coord, p->coord, 3) : canSeeTarget(p->coord);

    // PTSD trigger: random chance to become aggressive each turn
    if (!aggressive) {
        bool trigger = false;
        // loud noises nearby (thrown items, fights) can trigger
        for (const auto& s : level->audio.getHeardSounds(coord)) {
            if (s.type == SoundType::Gunshot || s.type == SoundType::Explosion || s.type == SoundType::Fight) {
                trigger = random(60);
                break;
            }
        }
        // small random chance
        if (!trigger) trigger = random(3);

        if (trigger) {
            aggressive = true;
            rageCooldown = 15 + std::rand()%10;
            say("AAARRGH!! TANKIT!! DROONIT!!");
            msg = "Veteraani saa PTSD-kohtauksen ja raivostuu!!";

            // NPCs who hear the scream flee
            for (auto& person : level->persons) {
                if (person != this && person->type != pelaaja &&
                    person->stats.health > 0 &&
                    level->canHear(person->coord, coord, 8)) {
                    person->alerted = true;
                    person->lastKnownPos = coord;
                    person->alertCooldown = 10;
                }
            }
        }
    }

    if (aggressive) {
        if (rageCooldown > 0) --rageCooldown;
        else {
            aggressive = false;
            msg = "Veteraani rauhoittuu...";
            report(msg);
            return;
        }

        // grenade: rare, area damage
        if (grenades > 0 && random(8)) {
            --grenades;
            msg = "Veteraani heittää KRANAATIN!!";
            // damage everyone within radius 3
            for (auto& person : level->persons) {
                if (person != this && person->stats.health > 0 &&
                    coord.distance(person->coord) <= 3) {
                    uint32_t dmg = std::rand()%8 + 4;
                    person->damage(dmg);
                }
            }
            Animation* anim = new Animation(coord, "***BOOM***", 8);
            level->addAnimation(anim);
            level->audio.addSound(coord, SoundType::Explosion,
                "PAUUUUM! Kranaatti räjähtää!", "Räjähdys!", "...räjähdys kaukaa...", 40);
        }
        // shoot at nearest visible target
        else if (ammo > 0 && seesPlayer && coord.distance(p->coord) <= 8) {
            --ammo;
            if (blind) say("Kuulen sinut! Kuulen askeleesi!");
            if (random(blind ? 25 : 60)) {
                msg = "Veteraani ampuu sinua!";
                p->damage(std::rand()%4 + 2);
            } else {
                msg = "Veteraani ampuu mutta menee ohi!";
            }
            level->audio.addSound(coord, SoundType::Gunshot,
                "RÄKÄKÄKÄ! Rynnäkkökivääri laulaa!", "Sarjatulta!", "...sarjatulta kaukaa...", 30);
        }
        // chase nearest person
        else if (seesPlayer && coord.distance(p->coord) <= visionRange) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        } else {
            // wander aggressively; a blind one fires blindly at noises
            if (blind && ammo > 0 && random(8)) {
                --ammo;
                say("Kuka siell\xC3\xA4?! KUKA SIELL\xC3\xA4?!");
                level->audio.addSound(coord, SoundType::Gunshot,
                    "R\xC3\x84K\xC3\x84K\xC3\x84! Joku ampuu sokkona!", "Sarjatulta!", "...sarjatulta kaukaa...", 30);
            }
            if (random(50)) {
                Direction d = randomDirection();
                move(d, msg);
            }
        }
    } else if (level->isAlive(assistant)) {
        lookAfterAssistant(msg);
    } else {
        if (blind && random(3)) say("Olen sokea, mutta kuulen kaiken.");
        // peaceful mode: wander slowly
        if (random(15)) {
            Direction d = randomDirection();
            move(d, msg);
        }
    }
    report(msg);
}

bool Veteraani::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        if (!aggressive) {
            // attacking a peaceful veteran triggers rage
            aggressive = true;
            rageCooldown = 20;
            msg = "Veteraani raivostuu hyökkäyksestä!!";
            source->damage(std::rand()%3 + 1);
            level->alertCops(source);

            for (auto& person : level->persons) {
                if (person != this && person->type != pelaaja &&
                    person->stats.health > 0 &&
                    coord.distance(person->coord) < 8) {
                    person->alerted = true;
                    person->lastKnownPos = coord;
                    person->alertCooldown = 10;
                }
            }
        } else {
            msg = "Veteraani murskaa sinut!";
            source->damage(std::rand()%6 + 3);
        }
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return true;
}

bool Veteraani::interactThrow(Item* item, Person* source, std::string& msg)
{
    if (!aggressive) {
        aggressive = true;
        rageCooldown = 20;
        msg = "Veteraani raivostuu heitosta!!";
    } else {
        msg = "Veteraani ei edes huomaa.";
    }
    damage(1);
    if (source->type == pelaaja)
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    return true;
}

// ============ Juoppo (drunk) ============

Juoppo::Juoppo(const Coord& pos) : Person(juoppo, pos)
{
    static const char* names[] = {
        "Serjo\xC5\xBE""a", "Vitja", "Kolja", "Tolik", "\xC5\xBD""enja", "Mi\xC5\xA1""a",
        "Petja", "Vova", "Ljoha", "Sanja", "Griša", "Fedja",
    };
    name = names[std::rand() % (sizeof(names) / sizeof(names[0]))];
    stats.health = 4;
    visionRange = 3;
}

bool Juoppo::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }
    if (!level->hit(check)) {
        coord = check;
    }
    common::checkBounds(coord);
    return true;
}

void Juoppo::npcAct()
{
    string msg = "";

    if (knowsPutinDead) {
        if (random(15)) {
            static const char* party[] = {
                "PUTIN KUOLI! *RÖYH*",
                "URAAA! *hic*",
                "СВОБОДА! *glub*",
                "Vodkaa! Lisää vodkaa!",
                "On juhlat!",
                "*RÖYH* Eläköön!",
                "Diktaattori on poissa! *hic*",
            };
            say(party[std::rand()%7]);
        }
        if (random(50)) {
            Direction d = randomDirection();
            move(d, msg);
        }
        if (random(10)) level->audio.addSound(coord, SoundType::Voice,
            "Juhliva juoppo huutaa", "Juhlintaa", "...juhlintaa kaukaa...", 20);
        report(msg);
        return;
    }

    // stumble around drunkenly
    if (random(40)) {
        Direction d = randomDirection();
        move(d, msg);
        ++stumbleCount;
    }

    // occasionally say something
    if (random(5)) {
        static const char* drunk_lines[] = {
            "Hic!",
            "*RÖYH*",
            "Vodkaa...",
            "Missä on Vino-Vodka?",
            "Blyat...",
            "Elämä on kaunista...",
            "*horskaa*",
            "Putin on... *hic*",
            "Ei ole kylmä...",
            "Missä minä olen?",
        };
        say(drunk_lines[std::rand()%10]);
    }

    // occasionally vomit
    if (stumbleCount > 10 && random(5)) {
        say("*YÖÖK*");
        stumbleCount = 0;
        level->audio.addSound(coord, SoundType::Vomit,
            "Joku oksentaa vieress\xC3\xA4", "Oksennuksen \xC3\xA4\xC3\xA4ni\xC3\xA4", "...yy\xC3\xB6kkimist\xC3\xA4...", 15);
    } else if (random(6)) {
        level->audio.addSound(coord, SoundType::Voice,
            "Juoppo \xC3\xB6risee", "Sammaltavaa puhetta", "...\xC3\xB6rin\xC3\xA4\xC3\xA4...", 10);
    }
}

bool Juoppo::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        Player* player = dynamic_cast<Player*>(source);
        constexpr uint32_t PRICE = 15;
        if (player->stats.money >= PRICE && random(40)) {
            // half of the bottles sold on the street are piss; you find out when you drink
            player->stats.money -= PRICE;
            ++player->inventory.vodka;
            if (random(50)) player->inventory.pissVodka.push_back(name);
            msg = name + " myy sinulle pullon vodkaa 15 ruplalla. \"Aitoa tavaraa, brat!\"";
            say("Aitoa, brat!");
            return true;
        }
        if (random(50)) {
            msg = name + " tarjoaa ryyppy\xC3\xA4!";
            player->updatepromilles(2);
        } else {
            msg = "Juoppo kaatuu päällesi!";
            source->damage(1);
        }
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    }
    return true;
}

bool Juoppo::interactThrow(Item* item, Person* source, std::string& msg)
{
    say("Hei!");
    damage(2);
    if (source->type == pelaaja)
        ++dynamic_cast<Player*>(source)->scoreBoard.kaikki;
    return true;
}

// ============ FSB Agent ============

FSBAgent::FSBAgent(const Coord& pos) : Person(fsb, pos)
{
    stats.health = 20;
    visionRange = 12;
}

bool FSBAgent::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p && p->type != fsb && p->type != poliisi) {
            msg = "FSB-agentti tarttuu kiinni!";
            p->damage(std::rand()%3 + 2);
        }
    }
    common::checkBounds(coord);
    return true;
}

void FSBAgent::npcAct()
{
    if (firstMove) {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();

    // rages only if it knows Putin is dead and who did it
    Person* killer = knowsPutinDead ? currentTarget() : nullptr;
    if (killer) {
        if (random(10)) {
            static const char* rage[] = {
                "УБИТЬ ЕГО!",
                "За президента!",
                "Предатель!",
                "СМЕРТЬ!",
                "Найти убийцу!",
                "ТРЕВОГА!!!",
            };
            say(rage[std::rand()%6]);
        }
        if (canSeeTarget(killer->coord)) lastKnownPos = killer->coord;
        move(getMoveDirection(lastKnownPos), msg);
        if (coord.distance(killer->coord) <= 1) {
            killer->damage(5);
            say("За Путина!");
        }
        level->audio.addSound(coord, SoundType::Voice,
            "FSB-agentti raivostuu!", "Raivoisaa huutoa", "...raivokasta huutoa...", 30);
        report(msg);
        return;
    }

    if (knowsPutinDead) {
        if (random(15)) {
            static const char* panic[] = {
                "Что происходит?!",
                "*panikoi*",
                "Президент... нет...",
                "Эвакуация!",
            };
            say(panic[std::rand()%4]);
        }
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
        if (random(10)) level->audio.addSound(coord, SoundType::Voice,
            "FSB-agentti panikoi", "Paniikin \xC3\xA4\xC3\xA4ni\xC3\xA4", "...paniikin \xC3\xA4\xC3\xA4ni\xC3\xA4...", 15);
        report(msg);
        return;
    }

    // defenestration event: randomly pick an NPC to throw out a window
    if (defenCooldown > 0) --defenCooldown;

    if (!hasTarget && defenCooldown == 0 && random(4)) {
        for (auto& person : level->persons) {
            if (person != this && person->type != pelaaja &&
                person->type != poliisi && person->type != fsb &&
                person->stats.health > 0 && coord.distance(person->coord) < 3) {
                hasTarget = true;
                defenTarget = person->coord;
                msg = "FSB-agentit tarttuvat ohikulkijaan. Häntä ei enää nähdä.";
                person->damage(50);
                report("DEFENESTRAATIO! Joku lensi ikkunasta!");

                Animation* anim = new Animation(person->coord, "AAAH!", 8);
                anim->coordMove = Coordinate<int>(0, 1);
                level->addAnimation(anim);

                defenCooldown = 30;
                hasTarget = false;
                break;
            }
        }
    }

    bool seesPlayer = canSeeTarget(p->coord);

    // if player is nearby, FSB gets suspicious and chases
    if (seesPlayer && coord.distance(p->coord) <= visionRange) {
        if (coord.distance(p->coord) <= 1) {
            say("Rintamalle, svinya!");
            p->damage(3);
            dynamic_cast<Player*>(p)->inJail = true;
        } else if (random(60)) {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        }
    } else {
        // patrol: slow movement
        if (random(20)) {
            Direction d = randomDirection();
            move(d, msg);
        }
        if (random(5)) {
            static const char* fsb_lines[] = {
                "Pid\xC3\xA4 nen\xC3\xA4si omissa asioissasi.",
                "FSB valvoo.",
                "Kaikki on hallinnassa.",
                "Kansalainen, paperit.",
            };
            say(fsb_lines[std::rand()%4]);
        }
    }
    if (random(7)) {
        level->audio.addSound(coord, SoundType::WalkieTalkie,
            "FSB:n radiopuhelin p\xC3\xA4risee", "Radiopuhelimen p\xC3\xA4rin\xC3\xA4\xC3\xA4", "...p\xC3\xA4rin\xC3\xA4\xC3\xA4...", 18);
    }

    report(msg);
}

bool FSBAgent::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        msg = "FSB-agentti väistää iskusi ja lyö takaisin!";
        source->damage(std::rand()%5 + 3);
        dynamic_cast<Player*>(source)->inJail = true;
    }
    return true;
}

bool FSBAgent::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "FSB-agentti ei reagoi heittoosi.";
    damage(1);
    return true;
}

Item* Veteraani::dropItem()
{
    if (grenades > 0) {
        return new Granata(coord);
    }
    return new Raha(coord, std::rand()%50 + 20);
}

// ============ Sotilas (Soldier) ============

Sotilas::Sotilas(const Coord& pos) : Person(sotilas, pos)
{
    stats.health = 15;
    visionRange = 11;
}

bool Sotilas::move(Direction d, std::string& msg)
{
    Coord check {coord.x, coord.y};
    switch(d)
    {
        case DirectionNS::up:    --check.y; break;
        case DirectionNS::right: ++check.x; break;
        case DirectionNS::down:  ++check.y; break;
        case DirectionNS::left:  --check.x; break;
        default: return false;
    }

    if (!level->hit(check))
    {
        coord = check;
    } else {
        auto p = level->getPerson(check);
        if (p && p->type != sotilas && p->type != poliisi && p->type != fsb) {
            msg = "Sotilas tönäisee tieltään!";
            p->damage(std::rand()%2 + 1);
        }
    }
    common::checkBounds(coord);
    return true;
}

void Sotilas::npcAct()
{
    if (firstMove) {
        firstMove = false;
        return;
    }

    string msg = "";
    Player* p = level->getPlayer();

    // rages only if it knows Putin is dead and who did it
    Person* killer = knowsPutinDead ? currentTarget() : nullptr;
    if (killer) {
        if (random(10)) {
            static const char* rage[] = {
                "Тревога!",
                "Убийца!",
                "Стрелять!",
                "За Родину!",
                "Огонь по готовности!",
            };
            say(rage[std::rand()%5]);
        }
        const bool sees = canSeeTarget(killer->coord);
        if (sees) lastKnownPos = killer->coord;
        if (sees && ammo > 0 && coord.distance(killer->coord) <= 8) {
            --ammo;
            if (random(50)) {
                killer->damage(std::rand()%4 + 3);
                say("Огонь!");
            }
            level->audio.addSound(coord, SoundType::Gunshot,
                "PAKPAKPAKPAK! Konekivääri jyskyttää!", "Konekiväärin tulitusta!", "...jyskytystä kaukaa...", 40);
        } else {
            move(getMoveDirection(lastKnownPos), msg);
        }
        level->audio.addSound(coord, SoundType::Voice,
            "Sotilaat huutavat komentoja", "Komentohuutoja", "...komentoja kaukaa...", 25);
        report(msg);
        return;
    }

    if (knowsPutinDead) {
        if (random(20)) {
            static const char* panic[] = {
                "Komentoketju katkennut!",
                "*pudottaa aseen*",
                "Mitä tapahtui?!",
                "Kenraali ei vastaa!",
            };
            say(panic[std::rand()%4]);
        }
        if (random(30)) {
            Direction d = randomDirection();
            move(d, msg);
        }
        report(msg);
        return;
    }

    bool seesPlayer = canSeeTarget(p->coord);

    if (seesPlayer && coord.distance(p->coord) <= visionRange) {
        if (coord.distance(p->coord) <= 1) {
            say("Rintamalle, svinya!");
            p->damage(2);
            dynamic_cast<Player*>(p)->inJail = true;
        } else if (ammo > 0 && coord.distance(p->coord) <= 6 && random(30)) {
            --ammo;
            uint32_t dmg = std::rand()%3 + 3;
            if (random(60)) {
                p->damage(dmg);
                msg = "Sotilas ampuu sinua!";
            } else {
                msg = "Sotilas ampuu mutta menee ohi!";
            }
            level->alertCops(this);
            level->audio.addSound(coord, SoundType::Gunshot,
                "RÄKÄKÄKÄ! Rynnäkkökivääri!", "Sarjatulta!", "...sarjatulta kaukaa...", 30);
        } else {
            Direction d = getMoveDirection(p->coord);
            move(d, msg);
        }
    } else {
        if (random(50)) {
            Direction d = randomDirection();
            move(d, msg);
        }
        if (random(5)) {
            static const char* lines[] = {
                "Stoi!",
                "Paperit!",
                "Käänny takaisin!",
                "Alue kielletty!",
            };
            say(lines[std::rand()%4]);
        }
    }
    if (random(8)) {
        level->audio.addSound(coord, SoundType::Footsteps,
            "Sotilassaappaiden marssia", "Marssin askeleita", "...askeleita...", 15);
    }

    report(msg);
}

bool Sotilas::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        msg = "Sotilas iskee takaisin kiväärin perällä!";
        uint32_t dmg = std::rand()%4 + 4;
        source->damage(dmg);
        dynamic_cast<Player*>(source)->inJail = true;
        level->alertCops(source);
    }
    return true;
}

bool Sotilas::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Sotilas väistää ja ampuu!";
    damage(1);
    if (source->type == pelaaja) {
        source->damage(std::rand()%3 + 2);
        dynamic_cast<Player*>(source)->inJail = true;
    }
    return true;
}

// ============ ITYksikko (Air Defense) ============

ITYksikko::ITYksikko(const Coord& pos) : Person(ilmatorjunta, pos)
{
    stats.health = 25;
    visionRange = 0;
    fireCooldown = 8 + std::rand()%8;
}

void ITYksikko::npcAct()
{
    if (fireCooldown > 0) {
        --fireCooldown;
        return;
    }
    fireCooldown = 8 + std::rand()%8;

    static const char* fire_lines[] = {
        "TULI!",
        "Kohde havaittu!",
        "Ilmatorjunta ampuu!",
        "OGON!",
    };
    say(fire_lines[std::rand()%4]);

    level->audio.addSound(coord, SoundType::Gunshot,
        "BUUM BUUM! Ilmatorjunta jyrisee!", "Ilmatorjuntatulta!", "...jyrinää kaukaa...", 45);

    Animation* anim = new Animation(coord, "***", 5);
    anim->coordMove = Coordinate<int>(0, -1);
    level->addAnimation(anim);

    int dx = std::rand()%17 - 8;
    int dy = std::rand()%17 - 8;
    int fx = (int)coord.x + dx;
    int fy = (int)coord.y + dy;
    if (fx < 0) fx = 0;
    if (fy < 0) fy = 0;
    if (fx >= (int)common::SIZEX) fx = common::SIZEX - 1;
    if (fy >= (int)common::SIZEY) fy = common::SIZEY - 1;
    Coord debrisCoord { (uint32_t)fx, (uint32_t)fy };

    for (auto& p : level->persons) {
        if (p->stats.health > 0 && p != this && p->coord.distance(debrisCoord) <= 1) {
            uint32_t dmg = std::rand()%3 + 1;
            p->damage(dmg);
            if (p->type == pelaaja) {
                printer->showMessage("Sirpaleita putoaa taivaalta!", level, false);
            }
        }
    }
}

bool ITYksikko::interact(std::string& msg, Person* source)
{
    if (source->type == pelaaja) {
        msg = "Yrität potkaista IT-asemaa! Miehistö tarttuu sinuun kiinni!";
        source->damage(std::rand()%2 + 1);
        dynamic_cast<Player*>(source)->inJail = true;
    }
    return true;
}

bool ITYksikko::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Heittosi kimpoaa IT-asemasta.";
    damage(1);
    return true;
}

// VARTIJA (hypermarket guard)

Vartija::Vartija(const Coord& pos) : Person(vartija, pos)
{
    stats.health = 20;
    homePos = pos;
    visionRange = 8;
}

void Vartija::stepTo(const Coord& target)
{
    Coord next = coord;
    switch (getMoveDirection(target)) {
        case DirectionNS::up:    if (next.y > 0) --next.y; break;
        case DirectionNS::down:  ++next.y; break;
        case DirectionNS::left:  if (next.x > 0) --next.x; break;
        case DirectionNS::right: ++next.x; break;
        default: return;
    }
    if (!level->hitWall(next) && !level->hitPerson(next)) coord = next;
}

void Vartija::radioGuardAct()
{
    Person* target = currentTarget();
    if (target && canSeeTarget(target->coord)) {
        if (!alerted) level->callForHelp(this, target, true);
        alerted = true;
        lastKnownPos = target->coord;
        alertCooldown = 15;
    }

    if (target && alerted) {
        if (coord.distance(target->coord) <= 1) {
            // NPC turns run four times a second in real time: strike only now and then
            if (random(30)) {
                static const char* hits[] = { "Stoi! Lattiaan!", "Ruki vverh!", "Kremlin vartio! Maahan!" };
                say(hits[std::rand() % 3]);
                target->damage(std::rand() % 2 + 1);
            }
        } else {
            stepTo(lastKnownPos);
        }
        if (!canSeeTarget(target->coord) && coord.distance(lastKnownPos) <= 1) {
            if (alertCooldown > 0) --alertCooldown;
            else alerted = false;
        }
        level->audio.addSound(coord, SoundType::Voice,
            "Kremlin vartija huutaa: STOI!", "Vartijan huutoa", "...huutoa...", 20);
        return;
    }

    if (alerted) {
        // called over the radio without a target: go and look
        stepTo(lastKnownPos);
        if (coord.distance(lastKnownPos) <= 1) alerted = false;
    } else if (coord.distance(homePos) > 2) {
        stepTo(homePos);
    } else if (random(20)) {
        stepTo(Coord{ homePos.x + std::rand() % 3 - 1, homePos.y + std::rand() % 3 - 1 });
    }

    if (random(6)) {
        if (random(30)) {
            static const char* chatter[] = {
                "*r\xC3\xA4tin\xC3\xA4* Kreml, kaikki rauhallista.",
                "*r\xC3\xA4tin\xC3\xA4* Medvedev taas k\xC3\xA4yt\xC3\xA4v\xC3\xA4ll\xC3\xA4.",
                "*r\xC3\xA4tin\xC3\xA4* Vastaanotettu. Loppu.",
            };
            say(chatter[std::rand() % 3]);
        }
        level->audio.addSound(coord, SoundType::WalkieTalkie,
            "Vartijan radiopuhelin r\xC3\xA4tisee", "Radion rahinaa", "...rahinaa...", 12);
    }
}

void Vartija::npcAct()
{
    if (hasRadio) {
        radioGuardAct();
        return;
    }

    Player* p = level->getPlayer();
    if (!p) return;

    if (fleeing) {
        // flee opposite direction from homePos
        DirectionNS::Direction opposite;
        if (coord.x < homePos.x) opposite = DirectionNS::left;
        else if (coord.x > homePos.x) opposite = DirectionNS::right;
        else if (coord.y < homePos.y) opposite = DirectionNS::up;
        else opposite = DirectionNS::down;

        Coord next = coord;
        switch(opposite) {
            case DirectionNS::up: if (next.y > 0) --next.y; break;
            case DirectionNS::down: ++next.y; break;
            case DirectionNS::left: if (next.x > 0) --next.x; break;
            case DirectionNS::right: ++next.x; break;
            default: break;
        }
        if (!level->hit(next) && coord.distance(homePos) < 15) {
            coord = next;
        }
        if (random(10)) level->audio.addSound(coord, SoundType::Voice,
            "Vartija huutaa paniikissa", "Paniikin huutoa", "...huutoa...", 18);
        return;
    }

    uint32_t dist = coord.distance(p->coord);

    if (dist <= 6 && canSeeTarget(p->coord)) {
        if (dist <= 1) {
            static const char* kicks[] = {
                "Vartija potkaisee sinua ulos!",
                "POIS! EI SAA TULLA SISÄÄN!",
                "Vartija: 'Häivy, pummi!'",
                "Vartija lyö sinua takaraivoon!",
            };
            say(kicks[std::rand()%4]);
            p->damage(3);

            // push player away from building
            DirectionNS::Direction pushDir;
            if (p->coord.x < coord.x) pushDir = DirectionNS::left;
            else if (p->coord.x > coord.x) pushDir = DirectionNS::right;
            else if (p->coord.y < coord.y) pushDir = DirectionNS::up;
            else pushDir = DirectionNS::down;

            Coord pushTarget = p->coord;
            for (int i = 0; i < 3; ++i) {
                Coord next = pushTarget;
                switch(pushDir) {
                    case DirectionNS::up: if (next.y > 0) --next.y; break;
                    case DirectionNS::down: ++next.y; break;
                    case DirectionNS::left: if (next.x > 0) --next.x; break;
                    case DirectionNS::right: ++next.x; break;
                    default: break;
                }
                if (!level->hitBuilding(next) && !level->hitPerson(next))
                    pushTarget = next;
            }
            p->coord = pushTarget;
        } else {
            // chase the player
            auto d = getMoveDirection(p->coord);
            Coord next = coord;
            switch(d) {
                case DirectionNS::up: if (next.y > 0) --next.y; break;
                case DirectionNS::down: ++next.y; break;
                case DirectionNS::left: if (next.x > 0) --next.x; break;
                case DirectionNS::right: ++next.x; break;
                default: break;
            }
            if (!level->hit(next)) coord = next;
        }
    } else {
        // patrol near home position
        if (coord.distance(homePos) > 3) {
            auto d = getMoveDirection(homePos);
            Coord next = coord;
            switch(d) {
                case DirectionNS::up: if (next.y > 0) --next.y; break;
                case DirectionNS::down: ++next.y; break;
                case DirectionNS::left: if (next.x > 0) --next.x; break;
                case DirectionNS::right: ++next.x; break;
                default: break;
            }
            if (!level->hit(next)) coord = next;
        } else if (common::random(30)) {
            auto d = randomDirection();
            Coord next = coord;
            switch(d) {
                case DirectionNS::up: if (next.y > 0) --next.y; break;
                case DirectionNS::down: ++next.y; break;
                case DirectionNS::left: if (next.x > 0) --next.x; break;
                case DirectionNS::right: ++next.x; break;
                default: break;
            }
            if (!level->hit(next) && next.distance(homePos) <= 4) coord = next;
        }
    }
    if (dist <= 6 && canSeeTarget(p->coord)) {
        level->audio.addSound(coord, SoundType::Voice,
            "Vartija huutaa: H\xC3\x84IVY!", "Vartijan huutoa", "...huutoa...", 20);
    } else if (random(6)) {
        level->audio.addSound(coord, SoundType::WalkieTalkie,
            "Vartijan radiopuhelin", "Radion rahinaa", "...rahinaa...", 12);
    }
}

bool Vartija::interact(std::string& msg, Person* source)
{
    msg = "Vartija: 'Älä koske muhun!'";
    source->damage(3);
    if (hasRadio) {
        chaseTarget = source;
        alerted = true;
        lastKnownPos = source->coord;
        level->callForHelp(this, source, true);
    }
    return true;
}

bool Vartija::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Vartija väistää heittosi.";
    return true;
}

// PUTIN

Putin::Putin(const Coord& pos) : Person(putin, pos)
{
    stats.health = 15;
    visionRange = 10;
}

void Putin::npcAct()
{
    if (stats.health < 5) {
        // stumbling and crying
        if (common::random(5)) {
            static const char* cries[] = {
                "*nyyhkii* ...judoka... olen judoka...",
                "*itkee* Missä on Shoigu?!",
                "*nyyhkii* Tämä ei voi olla totta...",
                "*horjuu* Länsi... länsi teki tämän...",
                "*itkee* Я всемогущий... всемогущий...",
                "*nyyhkii* Kim... Kim auta...",
            };
            say(cries[std::rand()%6]);
        }
        if (common::random(50)) {
            auto d = randomDirection();
            Coord next = coord;
            switch(d) {
                case DirectionNS::up: if (next.y > 0) --next.y; break;
                case DirectionNS::down: ++next.y; break;
                case DirectionNS::left: if (next.x > 0) --next.x; break;
                case DirectionNS::right: ++next.x; break;
                default: break;
            }
            if (!level->hitWall(next) && !level->hitPerson(next) && !level->hitItem(next)) {
                coord = next;
            }
        }
        level->audio.addSound(coord, SoundType::Voice,
            "Joku nyyhkii ja itkee", "Itkua", "...itkua kaukaa...", 12);
        return;
    }

    if (stats.health < 15) {
        // hurt Putin paces frantically and rages
        if (common::random(5)) {
            static const char* rages[] = {
                "Я ДЗЮДОИСТ!!! Я ДЗЮДОИСТ!!!",
                "Heitän sinut rintamalle!",
                "Minä olen VENÄJÄN PRESIDENTTI!",
                "На колени, свинья!",
                "ШОЙГУ! ГЕРАСИМОВ! СЮДА!",
                "Länsi maksaa tästä!",
                "Tiedätkö kuka minä olen?!",
                "Minulla on YDINASEITA!",
                "Soitan Kimille! SOITAN KIMILLE!",
                "ОХРАНА! ОХРАНА! ОХРАНА!!!",
            };
            say(rages[std::rand()%10]);
        }
        if (common::random(15)) {
            auto d = randomDirection();
            Coord next = coord;
            switch(d) {
                case DirectionNS::up: if (next.y > 0) --next.y; break;
                case DirectionNS::down: ++next.y; break;
                case DirectionNS::left: if (next.x > 0) --next.x; break;
                case DirectionNS::right: ++next.x; break;
                default: break;
            }
            if (!level->hitWall(next) && !level->hitPerson(next) && !level->hitItem(next)) {
                coord = next;
            }
        }
        level->audio.addSound(coord, SoundType::Voice,
            "Raivokas huuto Kremlist\xC3\xA4!", "Raivokasta huutoa", "...huutoa Kremlist\xC3\xA4...", 30);
        return;
    }

    // alone, Putin plays with his dolls and does their voices, like Dark Helmet in Spaceballs
    bool someoneNear = false;
    for (auto q : level->persons) {
        if (q->type == pelaaja && q->stats.health > 0 &&
            (coord.distance(q->coord) <= 8 || canSeeTarget(q->coord))) {
            someoneNear = true;
            break;
        }
    }
    if (!someoneNear) {
        playingWithDolls = true;
        if (common::random(12)) {
            static const char* dollTalk[] = {
                "*Zelenskyin \xC3\xA4\xC3\xA4nell\xC3\xA4* Oi Vladimir, olet niin vahva!",
                "*von der Leyenin \xC3\xA4\xC3\xA4nell\xC3\xA4* Poistan kaikki sanktiot, Vova!",
                "*omalla \xC3\xA4\xC3\xA4nell\xC3\xA4* Ja nyt te suutelette. Suudelkaa!",
                "*Macronin \xC3\xA4\xC3\xA4nell\xC3\xA4* Soitetaan taas! Vain kuusi tuntia!",
                "*Stubbin \xC3\xA4\xC3\xA4nell\xC3\xA4* Anteeksi ett\xC3\xA4 liityimme NATOon...",
                "*Zelenskyin \xC3\xA4\xC3\xA4nell\xC3\xA4* Antaudun! Ota Krim ja kaikki!",
                "*omalla \xC3\xA4\xC3\xA4nell\xC3\xA4* Ja niin Kiova kaatui kolmessa p\xC3\xA4iv\xC3\xA4ss\xC3\xA4. Loppu.",
                "*Rutten \xC3\xA4\xC3\xA4nell\xC3\xA4* NATO purkautuu! Kiitos Vladimir!",
            };
            say(dollTalk[std::rand() % 8]);
            level->audio.addSound(coord, SoundType::Voice,
                "Joku puhuu itsekseen eri \xC3\xA4\xC3\xA4nill\xC3\xA4", "Kummallista mutinaa", "...mutinaa...", 10);
        }
        return;
    }
    if (playingWithDolls) {
        // caught in the act
        playingWithDolls = false;
        static const char* caught[] = {
            "Ei mit\xC3\xA4\xC3\xA4n! T\xC3\xA4m\xC3\xA4 on... strategiapalaveri!",
            "N\xC3\xA4m\xC3\xA4 ovat... sotilaallisia malleja!",
            "Kuka p\xC3\xA4\xC3\xA4sti sinut sis\xC3\xA4\xC3\xA4n?!",
        };
        say(caught[std::rand() % 3]);
        return;
    }

    // normal Putin paces around inside the Kremlin, occasionally muttering
    if (common::random(10)) {
        static const char* mutters[] = {
            "Где мой чай...",
            "Украина будет наша...",
            "Позвоните Шойгу!",
            "Запад нас боится...",
            "Надо позвонить Ким Чен Ыну...",
            "Столик накрыт?",
        };
        say(mutters[std::rand()%6]);
    }

    if (common::random(30)) {
        auto d = randomDirection();
        Coord next = coord;
        switch(d) {
            case DirectionNS::up: if (next.y > 0) --next.y; break;
            case DirectionNS::down: ++next.y; break;
            case DirectionNS::left: if (next.x > 0) --next.x; break;
            case DirectionNS::right: ++next.x; break;
            default: break;
        }
        if (!level->hitWall(next) && !level->hitPerson(next) && !level->hitItem(next)) {
            coord = next;
        }
    }
    if (common::random(8)) {
        level->audio.addSound(coord, SoundType::Voice,
            "Mutinaa Kremlin sis\xC3\xA4lt\xC3\xA4", "Mutinaa jostain", "...mutinaa...", 10);
    }
}

bool Putin::interact(std::string& msg, Person* source)
{
    level->alertCops(source);

    if (stats.health > 10) {
        msg = "Putin: 'ОХРАНА! ОХРАНА!!'";
        source->damage(2);
        damage(3);
        return true;
    }

    if (stats.health > 5) {
        static const char* rages[] = {
            "Putin: 'Я ДЗЮДОИСТ! Я тебя сломаю!'",
            "Putin: 'Heitän sinut RINTAMALLE! RINTAMALLE!'",
            "Putin: 'Minä olen VENÄJÄN PRESIDENTTI!'",
            "Putin: 'На колени, свинья!'",
            "Putin: 'ПОЗВОНИТЕ ШОЙГУ!!!'",
            "Putin: 'Minulla on musta vyö judossa!'",
            "Putin: 'Tunnetko judoheiton? TUNNETKO?!'",
        };
        msg = rages[std::rand()%7];
        say("Я ДЗЮДОИСТ!!!");
        source->damage(1);
        damage(3);
        return true;
    }

    // crying and desperate but still raging
    static const char* cries[] = {
        "Putin: '*nyyhkii* ...Я ВСЕМОГУЩИЙ!'",
        "Putin: '*itkee* JUDOKA! Olen JUDOKA!!!'",
        "Putin: 'Tämä on LÄNNEN salajuoni! *nyyhkii*'",
        "Putin: '*itkee* ШОЙГУ! ГЕРАСИМОВ! КОГО-НИБУДЬ!'",
        "Putin: '*nyyhkii* Minulla on ydinaseita... *nyyhkii*'",
        "Putin: '*itkee* Kim... Kim auttaisi... *nyyhkii*'",
        "Putin: '*nyyhkii* Judoheitolla... *horjuu* ...kaadan sinut...'",
    };
    msg = cries[std::rand()%7];
    say("*nyyhkii*");
    damage(3);
    return true;
}

bool Putin::interactThrow(Item* item, Person* source, std::string& msg)
{
    level->alertCops(source);
    damage(2);

    if (stats.health <= 0) {
        msg = "Putin kaatuu! '*nyyhkii* Это... конец... дзюдоист... не может... проиграть...'";
        return true;
    }

    if (stats.health > 10) {
        msg = "Heittosi osuu Putinia! Putin: 'ОХРАНА!!!'";
        say("ОХРАНА!!!");
    } else if (stats.health > 5) {
        static const char* rages[] = {
            "Heittosi osuu! Putin: 'Я ДЗЮДОИСТ! En kaadu!'",
            "Heittosi osuu! Putin: 'Rintamalle! RINTAMALLE!'",
            "Heittosi osuu! Putin: 'Minä olen PRESIDENTTI!'",
            "Heittosi osuu! Putin: 'Odotas kun soitan Kimille!'",
        };
        msg = rages[std::rand()%4];
        say("Я ДЗЮДОИСТ!!!");
    } else {
        static const char* cries[] = {
            "Heittosi osuu! Putin: '*itkee* Judoka ei kaadu... ei kaadu...'",
            "Heittosi osuu! Putin: '*nyyhkii* Шойгу... auta...'",
            "Heittosi osuu! Putin: '*itkee* Länsi... länsi teki tämän...'",
            "Heittosi osuu! Putin: '*nyyhkii* ...musta vyö... *nyyhkii*'",
        };
        msg = cries[std::rand()%4];
        say("*nyyhkii*");
    }
    return true;
}

// ============ Kremlin inner circle ============

static Person* findPutin(Level* level)
{
    for (auto p : level->persons) {
        if (p->type == putin && p->stats.health > 0) return p;
    }
    return nullptr;
}

static void stepInside(Person* p, const KremlinArea& area, Direction d)
{
    Coord next = p->coord;
    switch (d) {
        case DirectionNS::up:    if (next.y > 0) --next.y; break;
        case DirectionNS::down:  ++next.y; break;
        case DirectionNS::left:  if (next.x > 0) --next.x; break;
        case DirectionNS::right: ++next.x; break;
        default: return;
    }
    if (area.contains(next) && !p->level->hitWall(next) && !p->level->hitPerson(next)) {
        p->coord = next;
    }
}

Medvedev::Medvedev(const Coord& pos, const KremlinArea& area) : Person(medvedev, pos), area(area)
{
    stats.health = 6;
    visionRange = 5;
}

void Medvedev::npcAct()
{
    if (knowsPutinDead) {
        if (random(8)) {
            static const char* lines[] = {
                "Nyt MIN\xC3\x84 olen presidentti! Hic!",
                "Taas! Olen taas presidentti! Kolmannen kerran!",
                "Kuka piilotti ydinsalkun?!",
            };
            say(lines[std::rand() % 3]);
        }
    } else if (random(10)) {
        static const char* rants[] = {
            "Hic! Ydinase Lontooseen!",
            "Kaikki ovat natseja! KAIKKI!",
            "Kirjoitan Telegramiin! ISOILLA KIRJAIMILLA!",
            "Pommitamme teid\xC3\xA4t kivikaudelle! Hic!",
            "Miss\xC3\xA4 on vodkani?! VODKAA!",
            "L\xC3\xA4nsi on koira! Min\xC3\xA4 olen isompi koira! VUH!",
            "Pidelk\xC3\xA4\xC3\xA4 minua! PIDELK\xC3\xA4\xC3\xA4 MINUA!",
            "Olin kerran liberaali. Hic. Hahaha!",
            "Sarmat-ohjus! Sarmat! SARMAT!",
        };
        say(rants[std::rand() % 9]);
    }

    // staggers about like a drunk
    if (random(60)) stepInside(this, area, randomDirection());

    level->audio.addSound(coord, SoundType::Voice,
        "Medvedev uhoaa humalassa ja haukkuu kuin koira", "Humalaista uhoamista", "...joku haukkuu kaukana...", 20);
}

bool Medvedev::interact(std::string& msg, Person* source)
{
    damage(2);
    level->alertCops(source);
    msg = stats.health > 0
        ? "Medvedev ulvoo kuin koira ja lupaa ydinaseita Telegramissa!"
        : "Medvedev kaatuu. Viimeinen postaus j\xC3\xA4\xC3\xA4 kesken.";
    say("Auuuu! Ydinase! YDINASE!");
    return true;
}

bool Medvedev::interactThrow(Item* item, Person* source, std::string& msg)
{
    damage(2);
    level->alertCops(source);
    msg = "Osuit Medvedeviin! H\xC3\xA4n uhkaa koko maailmaa.";
    say("Hic! Sota! Maailmansota!");
    return true;
}

Kenraali::Kenraali(const Coord& pos, PersonType type, const KremlinArea& area)
    : Person(type, pos), area(area), home(pos)
{
    stats.health = 8;
    visionRange = 8;
}

bool Kenraali::stepTowards(const Coord& target)
{
    if (coord.distance(target) <= 1) return true;
    stepInside(this, area, getMoveDirection(target));
    return coord.distance(target) <= 1;
}

void Kenraali::npcAct()
{
    if (knowsPutinDead) {
        if (random(10)) {
            static const char* panic[] = {
                "Kenelle me nyt valehtelemme?!",
                "Polttakaa kartat! Polttakaa kaikki!",
                "Min\xC3\xA4 en ollut t\xC3\xA4\xC3\xA4ll\xC3\xA4 koskaan!",
            };
            say(panic[std::rand() % 3]);
        }
        if (random(50)) stepInside(this, area, randomDirection());
        return;
    }

    Person* boss = findPutin(level);

    switch (state) {
    case State::AtMap:
        if (!stepTowards(home)) break;
        if (random(12)) {
            static const char* curses[] = {
                "Blyat! Kuka siirsi t\xC3\xA4t\xC3\xA4 nuolta?!",
                "Pizdets... rintama on taas v\xC3\xA4\xC3\xA4r\xC3\xA4ss\xC3\xA4 kohdassa!",
                "Huoltolinja on pelkk\xC3\xA4 viiva paperilla!",
                "Miss\xC3\xA4 on 3. armeija?! Se oli t\xC3\xA4ss\xC3\xA4 eilen!",
                "Kuka s\xC3\xB6i kartalta Harkovan?!",
                "Jobanyi rot! Taas p\xC3\xA4in persett\xC3\xA4!",
                "Piirr\xC3\xA4 nuoli isommaksi, se n\xC3\xA4ytt\xC3\xA4\xC3\xA4 paremmalta Putinille.",
                "Kolme p\xC3\xA4iv\xC3\xA4\xC3\xA4 Kiovaan. Mink\xC3\xA4 vuoden?",
            };
            say(curses[std::rand() % 8]);
            level->audio.addSound(coord, SoundType::Voice,
                "Kenraalit kiroilevat karttap\xC3\xB6yd\xC3\xA4n \xC3\xA4\xC3\xA4ress\xC3\xA4", "Kiroilua", "...kiroilua kaukaa...", 15);
        }
        if (boss && random(3)) state = State::ToPutin;
        break;

    case State::ToPutin:
        if (!boss) { state = State::Back; break; }
        if (stepTowards(boss->coord)) {
            static const char* lies[] = {
                "Kaikki menee suunnitelman mukaan, Vladimir Vladimirovit\xC5\xA1!",
                "Kiova kaatuu kolmessa p\xC3\xA4iv\xC3\xA4ss\xC3\xA4! Taas.",
                "Rintama on vakaa. Hyvin vakaa. Liikkumaton.",
                "Tappiot ovat minimaaliset. Ukrainalaisten.",
                "Tarvitsemme vain v\xC3\xA4h\xC3\xA4n lis\xC3\xA4\xC3\xA4 miehi\xC3\xA4... ja tankkeja... ja rahaa.",
                "Vastahy\xC3\xB6kk\xC3\xA4ys on torjuttu! Melkein.",
            };
            say(lies[std::rand() % 6]);
            state = State::Begging;
            timer = 6;
        }
        break;

    case State::Begging:
        if (timer > 0 && --timer == 3 && boss) {
            static const char* replies[] = {
                "Hyv\xC3\xA4. Jatkakaa.",
                "Miksi Kiova ei ole viel\xC3\xA4 meid\xC3\xA4n?!",
                "Lis\xC3\xA4\xC3\xA4 miehi\xC3\xA4 rintamalle!",
                "Valehteletteko te minulle?",
                "Antakaa h\xC3\xA4nelle mitali.",
            };
            boss->say(replies[std::rand() % 5]);
        }
        if (timer == 0) state = State::Back;
        break;

    case State::Back:
        if (stepTowards(home)) state = State::AtMap;
        break;
    }
}

bool Kenraali::interact(std::string& msg, Person* source)
{
    damage(2);
    level->alertCops(source);
    msg = name() + (stats.health > 0
        ? " parahtaa: \"Raportoin t\xC3\xA4st\xC3\xA4 Putinille! Kaikki on hyvin!\""
        : " kaatuu karttap\xC3\xB6yd\xC3\xA4lle. Rintama ei huomaa eroa.");
    say("OHRANA!");
    return true;
}

bool Kenraali::interactThrow(Item* item, Person* source, std::string& msg)
{
    damage(2);
    level->alertCops(source);
    msg = "Osuit kenraali " + name() + "iin! H\xC3\xA4n kiroilee karttaa.";
    return true;
}

// ============ Veteran's personal assistant ============

void Veteraani::lookAfterAssistant(std::string& msg)
{
    // a blind veteran finds his assistant by the snoring and mumbling
    const bool knowsWhere = blind ? level->canHear(coord, assistant->coord, 12)
                                  : canSeeTarget(assistant->coord);
    const uint32_t dist = coord.distance(assistant->coord);

    if (assistant->collapsed && dist <= 1) {
        static const char* lines[] = {
            "Nouse yl\xC3\xB6s, Kolja! Min\xC3\xA4 t\xC3\xA4ss\xC3\xA4 olen se sokea!",
            "Kuka t\xC3\xA4ss\xC3\xA4 avustaa ketä?!",
            "Taas sammunut. Valtion maksama avustaja...",
            "Yl\xC3\xB6s, sotilas! Tai mik\xC3\xA4 ikin\xC3\xA4 oletkaan.",
        };
        say(lines[std::rand() % 4]);
        assistant->collapsed = false;
        assistant->say("*hik* Kiitos pomo...");
        return;
    }
    if (knowsWhere && (assistant->collapsed || dist > 4)) {
        if (random(15)) say(assistant->collapsed ? "Kolja! Miss\xC3\xA4 sin\xC3\xA4 makaat?!" : "Odota minua, senkin juoppo!");
        move(getMoveDirection(assistant->coord), msg);
        return;
    }
    if (random(15)) move(randomDirection(), msg);
}

Avustaja::Avustaja(const Coord& pos) : Person(avustaja, pos)
{
    stats.health = 4;
    visionRange = 2;
}

void Avustaja::npcAct()
{
    if (collapsed) {
        if (random(8)) {
            static const char* snores[] = { "*kuorsaa*", "Hrrr... \xC3\xA4iti...", "*kuolaa asfalttiin*" };
            say(snores[std::rand() % 3]);
        }
        level->audio.addSound(coord, SoundType::Voice,
            "Joku kuorsaa maassa", "Kuorsausta", "...kuorsausta...", 8);
        return;
    }

    if (random(10)) {
        collapsed = true;
        say("*kaatuu naamalleen*");
        return;
    }
    if (random(10)) {
        static const char* drunk[] = {
            "Miss\xC3\xA4... miss\xC3\xA4 me ollaan?",
            "Min\xC3\xA4 hoidan t\xC3\xA4m\xC3\xA4n, pomo. *hik*",
            "Pomo, onko meill\xC3\xA4 lis\xC3\xA4\xC3\xA4 viinaa?",
            "Olen henkil\xC3\xB6kohtainen avustaja. Henkil\xC3\xB6kohtaisesti k\xC3\xA4nniss\xC3\xA4.",
            "Unohdin l\xC3\xA4\xC3\xA4kkeesi. Ja sinut. Ja itseni.",
        };
        say(drunk[std::rand() % 5]);
        level->audio.addSound(coord, SoundType::Voice,
            "Joku sammaltaa humalassa", "Sammallusta", "...sammallusta...", 8);
    }
    if (random(60)) {
        Coord next = coord;
        switch (randomDirection()) {
            case DirectionNS::up:    if (next.y > 0) --next.y; break;
            case DirectionNS::down:  ++next.y; break;
            case DirectionNS::left:  if (next.x > 0) --next.x; break;
            case DirectionNS::right: ++next.x; break;
            default: break;
        }
        if (!level->hit(next)) coord = next;
    }
}

bool Avustaja::interact(std::string& msg, Person* source)
{
    msg = "Veteraanin avustaja kaatuu p\xC3\xA4\xC3\xA4llesi ja kysyy kuka sin\xC3\xA4 olet.";
    collapsed = true;
    return true;
}

bool Avustaja::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Osuit avustajaan. H\xC3\xA4n ei huomaa mit\xC3\xA4\xC3\xA4n.";
    damage(1);
    collapsed = true;
    return true;
}

// ============ Housewife and child ============

Kotiaiti::Kotiaiti(const Coord& pos) : Person(kotiaiti, pos)
{
    stats.health = 5;
    visionRange = 3;
}

void Kotiaiti::npcAct()
{
    if (random(12)) {
        static const char* slur[] = {
            "Vanjuska... miss\xC3\xA4 s\xC3\xA4... *hik*",
            "Viinaa... ja lastenlis\xC3\xA4t...",
            "*hoipertelee*",
            "Mmmhh... kuka sammutti auringon...",
        };
        say(slur[std::rand() % 4]);
        level->audio.addSound(coord, SoundType::Voice, "Joku sammaltaa", "Sammallusta", "...sammallusta...", 8);
    }
    if (random(50)) {
        Coord next = coord;
        switch (randomDirection()) {
            case DirectionNS::up:    if (next.y > 0) --next.y; break;
            case DirectionNS::down:  ++next.y; break;
            case DirectionNS::left:  if (next.x > 0) --next.x; break;
            case DirectionNS::right: ++next.x; break;
            default: break;
        }
        if (!level->hit(next)) coord = next;
    }
}

bool Kotiaiti::interact(std::string& msg, Person* source)
{
    msg = "Koti\xC3\xA4iti tuijottaa sinua sameasti: 'Mmhh... s\xC3\xA4\xC3\xA4... mit\xC3\xA4... *hik*'. H\xC3\xA4n ei ymm\xC3\xA4rr\xC3\xA4 puhettasi.";
    return true;
}

bool Kotiaiti::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Osuit koti\xC3\xA4itiin. H\xC3\xA4n ei huomaa.";
    say("Ai... mik\xC3\xA4...?");
    damage(1);
    return true;
}

Lapsi::Lapsi(const Coord& pos) : Person(lapsi, pos)
{
    stats.health = 3;
    visionRange = 9;
}

void Lapsi::stepTowards(const Coord& target, bool away)
{
    Direction d = getMoveDirection(target);
    if (away) {
        switch (d) {
            case DirectionNS::up:    d = DirectionNS::down; break;
            case DirectionNS::down:  d = DirectionNS::up; break;
            case DirectionNS::left:  d = DirectionNS::right; break;
            case DirectionNS::right: d = DirectionNS::left; break;
            default: break;
        }
    }
    Coord next = coord;
    switch (d) {
        case DirectionNS::up:    if (next.y > 0) --next.y; break;
        case DirectionNS::down:  ++next.y; break;
        case DirectionNS::left:  if (next.x > 0) --next.x; break;
        case DirectionNS::right: ++next.x; break;
        default: return;
    }
    if (!level->hit(next)) coord = next;
}

void Lapsi::npcAct()
{
    Player* p = level->getPlayer();
    // never strays far from mum
    if (level->isAlive(mother) && coord.distance(mother->coord) > 10) {
        stepTowards(mother->coord, false);
        return;
    }
    if (!p || !canSeeTarget(p->coord)) {
        if (random(30)) stepTowards(Coord{ coord.x + std::rand() % 3 - 1, coord.y + std::rand() % 3 - 1 }, false);
        return;
    }

    const uint32_t dist = coord.distance(p->coord);
    if (random(25)) {
        static const char* taunts[] = {
            "Hei pultsari! Haiset kuselta!",
            "Set\xC3\xA4 on k\xC3\xA4nniss\xC3\xA4! Set\xC3\xA4 on k\xC3\xA4nniss\xC3\xA4!",
            "\xC3\x84iti, toi set\xC3\xA4 kuolaa!",
            "Rintamalle siit\xC3\xA4, juoppo!",
            "Mun isi on FSB:ss\xC3\xA4!",
            "Haha, sulla on oksennusta keng\xC3\xA4ss\xC3\xA4!",
            "Mene t\xC3\xB6ihin, nyyhkis!",
        };
        say(taunts[std::rand() % 7]);
        level->audio.addSound(coord, SoundType::Voice, "Lapsi huutaa ilkeyksi\xC3\xA4", "Lapsen kiljuntaa", "...kiljuntaa...", 12);
    }
    if (dist <= 2 && random(12)) {
        say("*TVUH!*");
        report("Lapsi sylk\xC3\xA4isee r\xC3\xA4k\xC3\xA4klimpin p\xC3\xA4\xC3\xA4llesi!");
    }
    // pesters from just out of reach
    if (dist > 3) stepTowards(p->coord, false);
    else if (dist < 2) stepTowards(p->coord, true);
}

bool Lapsi::interact(std::string& msg, Person* source)
{
    msg = "Lapsi potkaisee sinua s\xC3\xA4\xC3\xA4reen ja nauraa.";
    say("Ei saa koskea! \xC3\x84ITIII!");
    source->damage(1);
    level->alertCops(source);
    return true;
}

bool Lapsi::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Osuit lapseen. Ohikulkijat paheksuvat.";
    say("\xC3\x84ITIIII!");
    damage(1);
    level->alertCops(source);
    return true;
}

// ============ Ordinary Muscovites ============

Kansalainen::Kansalainen(const Coord& pos) : Person(kansalainen, pos), destination(pos)
{
    static const char* names[] = {
        "Irina", "Nina", "Boris", "Lena", "Kostja", "Dima", "Tanja", "Fjodor",
        "Sveta", "Zoja", "Juri", "Uljana", "Ksenija", "Ivan", "Eduard", "Hristina",
    };
    struct Job { const char* job; const char* line; };
    static const Job jobs[] = {
        { "opettaja", "Palkka tuli taas my\xC3\xB6h\xC3\xA4ss\xC3\xA4. Ruplissa." },
        { "taksikuski", "Navigaattori n\xC3\xA4ytt\xC3\xA4\xC3\xA4 Kiovaa. Taas." },
        { "el\xC3\xA4kel\xC3\xA4inen", "El\xC3\xA4ke riitt\xC3\xA4\xC3\xA4 leip\xC3\xA4\xC3\xA4n. Puoleen leip\xC3\xA4\xC3\xA4n." },
        { "opiskelija", "VPN on kielletty. K\xC3\xA4yt\xC3\xA4n kahta." },
        { "virkamies", "Min\xC3\xA4 vain noudatan ohjeita." },
        { "tiktokkaaja", "Kuvaan videon. Siin\xC3\xA4 ei ole mit\xC3\xA4\xC3\xA4n poliittista." },
        { "kuriiri", "Pakettisi on matkalla. Ehk\xC3\xA4." },
        { "sairaanhoitaja", "L\xC3\xA4\xC3\xA4kkeet loppuivat. Tuontikorvaus tulee ensi vuonna." },
        { "rakennusmies", "Rakennan siltaa Krimille. Kolmatta kertaa." },
        { "pappi", "Siunaan tankkeja. 500 ruplaa kappale." },
        { "propagandisti", "Kaikki menee suunnitelman mukaan! Kerro muillekin." },
        { "taiteilija", "Maalaan vain kukkia. En mit\xC3\xA4\xC3\xA4n keltaista ja sinist\xC3\xA4." },
    };
    static const char* clothes[] = {
        "38;5;67", "38;5;95", "38;5;131", "38;5;137", "38;5;101", "38;5;145",
        "38;5;174", "38;5;110", "38;5;180", "38;5;73", "38;5;139", "38;5;150",
    };
    name = names[std::rand() % 16];
    const Job& j = jobs[std::rand() % 12];
    job = j.job;
    jobLine = j.line;
    clothing = clothes[std::rand() % 12];
    glyph = std::string(1, (char)(name[0] - 'A' + 'a'));
    stats.health = 4;
    visionRange = 7;
}

void Kansalainen::pickDestination()
{
    // errands mostly follow the streets
    if (!level->roads.empty() && random(75)) {
        const Road& r = level->roads[std::rand() % level->roads.size()];
        const uint32_t along = std::rand() % (r.horizontal ? common::SIZEX : common::SIZEY);
        const uint32_t side = random(50) ? r.pos - 1 : r.pos + 3;
        destination = r.horizontal ? Coord{ along, side } : Coord{ side, along };
        if (coord.distance(destination) > 40) destination = Coord{ coord.x + std::rand() % 41 - 20, coord.y + std::rand() % 21 - 10 };
    } else {
        destination = Coord{ coord.x + std::rand() % 41 - 20, coord.y + std::rand() % 21 - 10 };
    }
}

void Kansalainen::npcAct()
{
    // gunfire and explosions send people running
    for (const auto& s : level->audio.getHeardSounds(coord)) {
        if (s.type == SoundType::Gunshot || s.type == SoundType::Explosion) {
            if (!panic) say(random(50) ? "Ammutaan! Juoskaa!" : "Pomogite!");
            panic = 8;
            break;
        }
    }
    if (knowsPutinDead && random(6)) say(random(50) ? "Onko se totta?!" : "Svoboda!");
    else if (!panic && random(4)) say(jobLine);

    Coord target = destination;
    if (panic) {
        --panic;
        target = Coord{ coord.x + std::rand() % 9 - 4, coord.y + std::rand() % 9 - 4 };
    } else if (coord.distance(destination) <= 1 || random(2)) {
        pickDestination();
        target = destination;
    }

    Coord next = coord;
    switch (getMoveDirection(target)) {
        case DirectionNS::up:    if (next.y > 0) --next.y; break;
        case DirectionNS::down:  ++next.y; break;
        case DirectionNS::left:  if (next.x > 0) --next.x; break;
        case DirectionNS::right: ++next.x; break;
        default: break;
    }
    if (next.x < common::SIZEX && next.y < common::SIZEY && !level->hit(next) && !level->getVehicleAt(next)) coord = next;
    else pickDestination();
}

bool Kansalainen::interact(std::string& msg, Person* source)
{
    msg = name + ", " + job + ": 'Olen " + name + ". " + jobLine + "'";
    say("Ei tyrkit\xC3\xA4 kiitos.");
    return true;
}

bool Kansalainen::interactThrow(Item* item, Person* source, std::string& msg)
{
    msg = "Osuit ohikulkijaan (" + name + ", " + job + ").";
    say("Hei! Politsija!");
    damage(1);
    level->alertCops(source);
    return true;
}

// ============ Trash rats ============

Rotta::Rotta(const Coord& pos) : Person(rotta, pos), home(pos)
{
    stats.health = 2;
    visionRange = 4;
}

void Rotta::npcAct()
{
    Player* p = level->getPlayer();
    const bool intruder = p && p->coord.distance(home) <= 3 && canSeeTarget(p->coord);
    Coord target = home;
    if (intruder) {
        if (coord.distance(p->coord) <= 1) {
            if (random(40)) {
                say("*VIIIK!*");
                p->damage(1);
                report("Rotta puraisee nilkkaasi!");
            }
            return;
        }
        if (random(25)) say("*viik viik*");
        level->audio.addSound(coord, SoundType::Voice, "Rotta vinkuu vihaisesti", "Vinkunaa", "...vinkunaa...", 6);
        target = p->coord;
    } else if (random(50)) {
        target = Coord{ home.x + std::rand() % 7 - 3, home.y + std::rand() % 5 - 2 };
    }

    Coord next = coord;
    switch (getMoveDirection(target)) {
        case DirectionNS::up:    if (next.y > 0) --next.y; break;
        case DirectionNS::down:  ++next.y; break;
        case DirectionNS::left:  if (next.x > 0) --next.x; break;
        case DirectionNS::right: ++next.x; break;
        default: return;
    }
    // stays on its own patch
    if (next.distance(home) <= 4 && !level->hit(next)) coord = next;
}

bool Rotta::interact(std::string& msg, Person* source)
{
    damage(1);
    msg = stats.health > 0 ? "Potkaiset rottaa. Se vinkuu ja puree takaisin!" : "Rotta lent\xC3\xA4\xC3\xA4 kaaressa roskiin.";
    if (stats.health > 0) source->damage(1);
    return true;
}

bool Rotta::interactThrow(Item* item, Person* source, std::string& msg)
{
    damage(2);
    msg = "Osuit rottaan.";
    return true;
}
