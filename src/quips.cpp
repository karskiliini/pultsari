#include "quips.hpp"
#include <cstdlib>

std::string pickQuip(const std::vector<std::string>& lines)
{
    if (lines.empty()) return "";
    return lines[std::rand() % lines.size()];
}

const std::vector<std::string> BEER_LINES = {
    "Olutpullo meni kierteellä alas.",
    "Baltika katosi kuin rupla lomakassasta.",
};

const std::string VODKA_SIP_LINES[6] = {
    "Tyhjä. Pullo katsoo sinua syyttävästi.",
    "Viimeinen huikka odottaa.",
    "Pullo kevenee, mieli raskautuu.",
    "Puolikas jäljellä. Optimisti sanoisi puoliksi täysi.",
    "Kurkku polttaa. Se on hyvä merkki.",
    "Ensimmäinen on aina pyhä. Na zdorovje!",
};

const std::string HINT_HANDS_FULL = "Kädet ovat täynnä. Juo tai heitä jotain pois.";
const std::string HINT_AK_HANDS = "AK roikkuu selässä, mutta kädet ovat varattuina. Juo pullo tyhjäksi ja heitä se pois, tai paina inventaariossa E rynkyn kohdalla.";
const std::string HINT_PISTOL_BOTTLE = "Pistooli on vyöllä, mutta kädessä on pullo.";
const std::string HINT_PISTOL_HANDS = "Pistooli on vyöllä, mutta kädet ovat täynnä. Heitä jotain pois.";
const std::string HINT_AK_ON_BACK = "Selässä on jo rynkky.";
const std::string AK_ISSUED_MSG = "Kersantti työntää käsiisi Kalashnikovin ja täyden lippaan.";
const std::string KICK_BONA_MSG = "Kädet täynnä — potkaiset pullon rikki.";
