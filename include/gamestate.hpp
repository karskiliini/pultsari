#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

#include <cstdint>
#include <string>

enum class GamePhase {
    Moscow,
    War,
    ReturnMoscow,
    Kremlin,
    Victory
};

enum class MoscowLandmark {
    GorkyPark,
    RedSquare,
    Arbat,
    MoskvaRiver,
    Metro,
    GUM,
    Bolshoi,
    KremlinApproach,
    None
};

struct GameState {
    GamePhase phase = GamePhase::Moscow;
    uint32_t moscowStage = 1;
    uint32_t warStage = 0;
    bool hasBeenToWar = false;
    bool hasKremlinPass = false;
    uint32_t kremlinPassParts = 0;

    uint32_t warWeapons = 0;

    std::string getLevelName() const;
    MoscowLandmark getLandmark() const;
    uint32_t getEffectiveStage() const;

    void advanceLevel();
    void sendToWar();
    void returnFromWar();
    void enterKremlin();
};

#endif
