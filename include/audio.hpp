#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "coord.hpp"
#include <string>
#include <vector>
#include <cstdint>

enum class SoundType {
    Voice,
    Footsteps,
    Engine,
    Siren,
    Gunshot,
    Explosion,
    Fire,
    Radio,
    WalkieTalkie,
    Drone,
    Music,
    Vomit,
    Fight,
    Glass,
    Door,
    Ambient
};

struct AudioEvent {
    Coord position;
    SoundType type;
    std::string textClose;    // 1-5 tiles
    std::string textMedium;   // 6-12 tiles
    std::string textFar;      // 13-25 tiles
    uint32_t range = 20;
    uint32_t volume = 100;    // 0-100, affects effective range
};

struct HeardSound {
    std::string text;
    uint32_t distance;
    SoundType type;
};

class Level;

class AudioSystem {
public:
    AudioSystem() = default;

    void clear();
    void addSound(const AudioEvent& event);
    void addSound(const Coord& pos, SoundType type,
                  const std::string& close,
                  const std::string& medium,
                  const std::string& far,
                  uint32_t range = 20);

    // hearing: percent of normal range the listener still hears (HIV wears it down)
    std::vector<HeardSound> getHeardSounds(const Coord& listenerPos, uint32_t hearing = 100) const;
    std::string formatSoundLine(const Coord& listenerPos, uint32_t maxLen = 78, uint32_t hearing = 100) const;

    void setLevel(const Level* l) { level = l; }

private:
    void cue(const AudioEvent& e) const;
    std::vector<AudioEvent> events;
    const Level* level = nullptr;
};

#endif
