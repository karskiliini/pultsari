#include "subtitles.hpp"
#include "level.hpp"
#include "mask.hpp"
#include "persons.hpp"
#include "persontype.hpp"
#include "player.hpp"
#include <cstdlib>
#include <deque>
#include <map>

namespace Subtitles {

namespace {
std::string text;
uint32_t left = 0;
uint64_t clockMs = 0;
std::map<std::string, uint64_t> lastSaid;   // kind -> when it was last shown
std::vector<std::string> said;
std::deque<std::pair<std::string, uint32_t>> pending;

void show(const std::string& line, uint32_t showMs)
{
    text = line;
    left = showMs;
    said.push_back(line);
    if (said.size() > 200) said.erase(said.begin());
}
}

bool say(const std::string& line, const std::string& kind, uint32_t cooldownMs, uint32_t showMs)
{
    if (line.empty()) return false;
    if (!kind.empty()) {
        auto it = lastSaid.find(kind);
        if (it != lastSaid.end() && clockMs - it->second < cooldownMs) return false;
        lastSaid[kind] = clockMs;
    }
    show(line, showMs);
    return true;
}

void queue(const std::string& line, uint32_t showMs)
{
    if (line.empty()) return;
    if (left == 0 && pending.empty()) show(line, showMs);
    else pending.emplace_back(line, showMs);
}

const std::string& current()
{
    static const std::string none;
    return left > 0 ? text : none;
}

uint32_t leftMs() { return left; }

void tick(uint32_t ms)
{
    clockMs += ms;
    left = left > ms ? left - ms : 0;
    if (left == 0 && !pending.empty()) {
        show(pending.front().first, pending.front().second);
        pending.pop_front();
    }
}

void reset()
{
    text.clear();
    left = 0;
    clockMs = 0;
    lastSaid.clear();
    said.clear();
    pending.clear();
}

void toTheFront(WarRoute how)
{
    switch (how) {
        case WarRoute::Draft: say("Kutsunta: l\xC3\xA4\xC3\xA4k\xC3\xA4ri totesi sinut terveeksi. Rintamalle.", "", 0, 4000); break;
        case WarRoute::Court: say("Oikeuden tuomio: rintamalle. Valitusaika umpeutui jo.", "", 0, 4000); break;
        case WarRoute::Fsb:   say("FSB-v\xC3\xA4rv\xC3\xA4ys: vapaaehtoinen, koska et kielt\xC3\xA4ytynyt.", "", 0, 4000); break;
    }
}

std::vector<std::string> handsTutorial(char introKey)
{
    const char key[2] = { (char)(introKey >= 'a' && introKey <= 'z' ? introKey - 32 : introKey), 0 };
    return {
        "Kartan sivulla n\xC3\xA4kyv\xC3\xA4t k\xC3\xA4tesi. Ne ovat koko omaisuutesi.",
        "Vasen ja oikea k\xC3\xA4si kantavat kumpikin yhden tavaran.",
        "Lis\xC3\xA4ksi tasku, vy\xC3\xB6 ja sel\xC3\xA4ss\xC3\xA4 AK. Ei muuta. Neuvostoliitossa riitti v\xC3\xA4hempikin.",
        "Hihan kunto on terveytesi: mit\xC3\xA4 repaleisempi hiha, sit\xC3\xA4 l\xC3\xA4hemp\xC3\xA4n\xC3\xA4 loppu.",
        std::string("Paina ") + key + ": k\xC3\xA4sien esittely. Ohjeet luetaan t\xC3\xA4\xC3\xA4ll\xC3\xA4 vasta j\xC3\xA4lkik\xC3\xA4teen.",
    };
}

const std::vector<std::string>& history() { return said; }

std::string meetingLine(int personType)
{
    switch (personType) {
        case poliisi:      return "Miliisi. Hymyile: olet jo syyllinen.";
        case sotilas:      return "Sotilas. Erikoisoperaation sankari, kuulemma.";
        case fsb:          return "FSB-mies. H\xC3\xA4n tiet\xC3\xA4\xC3\xA4 jo, mit\xC3\xA4 aiot tehd\xC3\xA4.";
        case varas:        return "Taskuvaras. Pid\xC3\xA4 kiinni pulloistasi.";
        case myyja:        return "Myyj\xC3\xA4. H\xC3\xA4n muistaa kasvosi.";
        case mummo:        return "Babu\xC5\xA1ka. Vaarallisempi kuin n\xC3\xA4ytt\xC3\xA4\xC3\xA4.";
        case skinhead:     return "Skinhead. Is\xC3\xA4nmaan yst\xC3\xA4v\xC3\xA4, ei sinun.";
        case jengi:        return "Gopnik. Kyykyss\xC3\xA4 ja vihainen.";
        case diileri:      return "Diileri. Tarjous, josta ei kielt\xC3\xA4ydyt\xC3\xA4.";
        case veteraani:    return "Veteraani. Mitalit kilisev\xC3\xA4t, el\xC3\xA4ke ei.";
        case ryostaja:
        case pankkirosvo:  return "Ry\xC3\xB6st\xC3\xA4j\xC3\xA4. Rahat tai henki, kumpikin kelpaa.";
        case vartija:      return "Vartija. Palkka pieni, pamppu iso.";
        case ilmatorjunta: return "Ilmatorjunta. Ampuu kaikkea, mik\xC3\xA4 surisee.";
        case korealainen:  return "Pohjoiskorealainen. H\xC3\xA4nelle sanottiin, ett\xC3\xA4 t\xC3\xA4m\xC3\xA4 on harjoitus.";
        case rekryytti:    return "Afrikasta v\xC3\xA4rv\xC3\xA4tty. H\xC3\xA4nelle luvattiin siviility\xC3\xB6.";
        case juoppo:       return "Toinen juoppo. Kilpailija pullonpalautuksessa.";
        case putin:        return "Vova. Pienempi kuin televisiossa.";
        default:           return "";
    }
}

void noticePeople(Level& level, const PlayerNS::Player& player, const VisionNS::Mask* mask)
{
    // an action caption on screen is more important than a face in the crowd
    if (left > 0 || level.playerDead) return;
    constexpr int NEAR = 6;
    const Person* nearest = nullptr;
    int best = NEAR * NEAR + 1;
    for (const Person* p : level.persons) {
        if (!p || p == &player || p->stats.health == 0 || meetingLine(p->type).empty()) continue;
        const int dx = (int)p->coord.x - (int)player.coord.x, dy = (int)p->coord.y - (int)player.coord.y;
        const int d = dx * dx + dy * dy;
        if (d > NEAR * NEAR || d >= best) continue;
        if (mask && mask->enabled && !mask->visible(p->coord)) continue;
        // each sort of person only once a minute
        auto it = lastSaid.find("meet:" + std::to_string(p->type));
        if (it != lastSaid.end() && clockMs - it->second < 60000) continue;
        nearest = p;
        best = d;
    }
    if (nearest) say(meetingLine(nearest->type), "meet:" + std::to_string(nearest->type), 60000);
}

}
