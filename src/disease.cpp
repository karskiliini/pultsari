#include "disease.hpp"
#include <algorithm>

namespace Disease {

bool Hiv::needleStick(uint32_t turn, uint32_t roll)
{
    ++sticks;
    if (infected || roll >= INFECTION_CHANCE) return false;
    infected = true;
    infectedTurn = turn;
    return true;
}

uint32_t Hiv::stageStart(uint32_t s) const
{
    if (s == 0) return infectedTurn;
    return infectedTurn + INCUBATION_TURNS + (s - 1) * STAGE_TURNS;
}

uint32_t Hiv::stage(uint32_t turn) const
{
    if (!infected || turn < stageStart(1)) return 0;
    const uint32_t s = 1 + (turn - stageStart(1)) / STAGE_TURNS;
    return std::min(s, AIDS_STAGE);
}

uint32_t Hiv::wasted(uint32_t turn) const
{
    if (stage(turn) < AIDS_STAGE) return 0;
    return (turn - stageStart(AIDS_STAGE)) / WASTING_TURNS;
}

static const uint32_t HEALTH_CAP[] = { 0, 8, 7, 5, 4, 3 };
static const uint32_t VISION_PCT[] = { 100, 90, 75, 60, 45, 35 };
static const uint32_t HEARING_PCT[] = { 100, 85, 70, 55, 40, 25 };

static uint32_t clampStage(uint32_t s) { return std::min(s, AIDS_STAGE); }

uint32_t maxHealth(uint32_t stage) { return HEALTH_CAP[clampStage(stage)]; }

uint32_t visionDistance(uint32_t base, uint32_t stage)
{
    return std::max<uint32_t>(2, base * VISION_PCT[clampStage(stage)] / 100);
}

uint32_t hearingPercent(uint32_t stage) { return HEARING_PCT[clampStage(stage)]; }

const char* hudLabel(uint32_t stage)
{
    if (stage == 0) return "";
    return stage >= AIDS_STAGE ? "AIDS" : "HIV+";
}

const char* stageMessage(uint32_t stage)
{
    switch (clampStage(stage)) {
        case 1: return "Imusolmukkeet turvoksissa, y\xC3\xB6hiki ja voimat hupenevat. Se neula ei ollutkaan vain neula: HIV. "
                       "Virallisesti Ven\xC3\xA4j\xC3\xA4ll\xC3\xA4 ei ole epidemiaa, joten et ole sairas.";
        case 2: return "Kuume palaa ja ysk\xC3\xA4 ei ota loppuakseen. N\xC3\xA4k\xC3\xB6kentt\xC3\xA4 kapenee.";
        case 3: return "Korvissa humisee. Kaupungin \xC3\xA4\xC3\xA4net tulevat kuin villan l\xC3\xA4pi. Ruoka ei pysy sis\xC3\xA4ll\xC3\xA4.";
        case 4: return "Painoa l\xC3\xA4htee viikko viikolta. Silmiss\xC3\xA4 sumenee, ja jalat painavat kuin valurauta.";
        case 5: return "AIDS. Elimist\xC3\xB6 ei en\xC3\xA4\xC3\xA4 puolustaudu. Poliklinikan jonoon olisi ollut kolme kuukautta.";
        default: return "";
    }
}

}
