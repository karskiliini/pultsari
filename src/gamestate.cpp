#include "gamestate.hpp"

using std::string;

static const char* moscowNames[] = {
    "Gorkin Puisto",
    "Punainen Tori",
    "Arbat",
    "Moskva-joki",
    "Metro",
    "GUM",
    "Bolshoi-teatteri",
    "Kremlin portit"
};

static const char* warNames[] = {
    "Juoksuhauta",
    "Ei-kenenkään-maa",
    "Kylän rauniot"
};

static const MoscowLandmark landmarks[] = {
    MoscowLandmark::GorkyPark,
    MoscowLandmark::RedSquare,
    MoscowLandmark::Arbat,
    MoscowLandmark::MoskvaRiver,
    MoscowLandmark::Metro,
    MoscowLandmark::GUM,
    MoscowLandmark::Bolshoi,
    MoscowLandmark::KremlinApproach
};

string GameState::getLevelName() const
{
    switch (phase) {
        case GamePhase::Moscow: {
            uint32_t idx = (moscowStage - 1) % 8;
            return moscowNames[idx];
        }
        case GamePhase::War: {
            if (warStage >= 1 && warStage <= 3)
                return warNames[warStage - 1];
            return "Ukraina";
        }
        case GamePhase::ReturnMoscow: {
            uint32_t idx = (moscowStage - 1) % 8;
            return string(moscowNames[idx]) + " (paluu)";
        }
        case GamePhase::Kremlin:
            return "Kreml";
        case GamePhase::Victory:
            return "Vovan viinakellarissa";
    }
    return "???";
}

MoscowLandmark GameState::getLandmark() const
{
    if (phase == GamePhase::Moscow || phase == GamePhase::ReturnMoscow) {
        uint32_t idx = (moscowStage - 1) % 8;
        return landmarks[idx];
    }
    return MoscowLandmark::None;
}

uint32_t GameState::getEffectiveStage() const
{
    switch (phase) {
        case GamePhase::Moscow:
            return moscowStage;
        case GamePhase::War:
            return 8 + warStage;
        case GamePhase::ReturnMoscow:
            return 11 + moscowStage;
        case GamePhase::Kremlin:
            return 20;
        case GamePhase::Victory:
            return 0;
    }
    return 1;
}

void GameState::advanceLevel()
{
    switch (phase) {
        case GamePhase::Moscow:
            ++moscowStage;
            if (moscowStage > 8) {
                moscowStage = 8;
            }
            break;
        case GamePhase::War:
            ++warStage;
            if (warStage > 3) {
                returnFromWar();
            }
            break;
        case GamePhase::ReturnMoscow:
            ++moscowStage;
            if (moscowStage > 8) {
                if (kremlinPassParts >= 3) {
                    enterKremlin();
                } else {
                    moscowStage = 1;
                }
            }
            break;
        case GamePhase::Kremlin:
            phase = GamePhase::Victory;
            break;
        case GamePhase::Victory:
            break;
    }
}

void GameState::sendToWar()
{
    phase = GamePhase::War;
    warStage = 1;
}

void GameState::returnFromWar()
{
    phase = GamePhase::ReturnMoscow;
    hasBeenToWar = true;
    moscowStage = 1;
}

void GameState::enterKremlin()
{
    phase = GamePhase::Kremlin;
    hasKremlinPass = true;
}
