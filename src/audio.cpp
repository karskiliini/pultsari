
#include "audio.hpp"
#include "level.hpp"
#include "player.hpp"
#include "common.hpp"
#include <algorithm>
#include <cmath>

void AudioSystem::clear()
{
    events.clear();
}

void AudioSystem::addSound(const AudioEvent& event)
{
    events.push_back(event);
    cue(event);
    if (event.type == SoundType::Vomit) common::vomitEvent(event.position, 0, 0);
}

// sound effect for an event, picked by type and, for guns, by the weapon's text
static const char* effectFor(const AudioEvent& e)
{
    switch (e.type) {
        case SoundType::Gunshot:
            if (e.textClose.rfind("PAM", 0) == 0) return "pistol";
            if (e.textClose.rfind("PAKPAK", 0) == 0) return "mg";
            if (e.textClose.rfind("BUUM", 0) == 0) return "aa";
            return "rifle";
        case SoundType::Explosion:    return "explosion";
        case SoundType::Siren:        return "siren";
        case SoundType::WalkieTalkie:
        case SoundType::Radio:        return "radio";
        case SoundType::Vomit:        return "vomit";
        case SoundType::Fire:         return "fire";
        case SoundType::Drone:        return "drone";
        case SoundType::Glass:        return "glass";
        default:                      return nullptr;
    }
}

void AudioSystem::cue(const AudioEvent& e) const
{
    const char* name = effectFor(e);
    if (!name || !level) return;
    const PlayerNS::Player* p = level->getPlayer();
    if (!p) return;
    const std::vector<AudioEvent> single { e };
    AudioSystem probe;
    probe.level = level;
    probe.events = single;
    const auto heard = probe.getHeardSounds(p->coord);
    if (heard.empty()) return;
    const uint32_t range = std::max<uint32_t>(1, e.range);
    const float vol = 1.0f - 0.8f * std::min(1.0f, (float)heard.front().distance / (float)range);
    common::sound(name, vol);
}

void AudioSystem::addSound(const Coord& pos, SoundType type,
                           const std::string& close,
                           const std::string& medium,
                           const std::string& far,
                           uint32_t range)
{
    AudioEvent e;
    e.position = pos;
    e.type = type;
    e.textClose = close;
    e.textMedium = medium;
    e.textFar = far;
    e.range = range;
    events.push_back(e);
    cue(e);
    if (type == SoundType::Vomit) common::vomitEvent(pos, 0, 0);
}

std::vector<HeardSound> AudioSystem::getHeardSounds(const Coord& listenerPos) const
{
    std::vector<HeardSound> heard;

    for (const auto& e : events) {
        uint32_t dist = listenerPos.distance(e.position);
        uint32_t effectiveRange = e.range * e.volume / 100;

        // speech and quiet sounds stop at walls; loud ones are muffled to a third per wall
        const uint32_t walls = level ? level->wallsBetween(listenerPos, e.position) : 0;
        const bool quiet = e.type == SoundType::Voice || e.type == SoundType::Radio ||
                           e.type == SoundType::WalkieTalkie || e.type == SoundType::Footsteps ||
                           e.type == SoundType::Vomit;
        if (walls > 0 && quiet) continue;
        for (uint32_t w = 0; w < walls; ++w) effectiveRange /= 3;

        if (dist > effectiveRange) continue;

        HeardSound s;
        s.distance = dist;
        s.type = e.type;

        if (dist <= 5) {
            s.text = e.textClose;
        } else if (dist <= 12) {
            s.text = e.textMedium;
        } else {
            s.text = e.textFar;
        }

        heard.push_back(s);
    }

    // sort by distance
    std::sort(heard.begin(), heard.end(),
        [](const HeardSound& a, const HeardSound& b) {
            return a.distance < b.distance;
        });

    return heard;
}

std::string AudioSystem::formatSoundLine(const Coord& listenerPos, uint32_t maxLen) const
{
    auto heard = getHeardSounds(listenerPos);
    if (heard.empty()) return "";

    std::string line = "KUULET: ";
    uint32_t count = 0;
    constexpr uint32_t MAX_SOUNDS = 4;

    for (const auto& s : heard) {
        if (count >= MAX_SOUNDS) break;
        if (s.text.empty()) continue;

        if (count > 0) line += " | ";
        line += s.text;
        ++count;

        if (line.length() > maxLen - 5) break;
    }

    if (line.length() > maxLen) {
        size_t cut = maxLen - 3;
        while (cut > 0 && ((unsigned char)line[cut] & 0xC0) == 0x80) --cut;
        line = line.substr(0, cut) + "...";
    }

    return line;
}
