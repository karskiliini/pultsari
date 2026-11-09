
#include "audio.hpp"
#include "level.hpp"
#include <algorithm>
#include <cmath>

void AudioSystem::clear()
{
    events.clear();
}

void AudioSystem::addSound(const AudioEvent& event)
{
    events.push_back(event);
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
