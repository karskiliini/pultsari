#include "hands.hpp"

bool isSmall(Thing t)
{
    return t == Thing::Lipas || t == Thing::Granata || t == Thing::Veitsi;
}

bool isDrink(Thing t)
{
    return t == Thing::Kalja || t == Thing::Lonkka || t == Thing::Vodka;
}

bool isWeapon(Thing t)
{
    return t == Thing::Pistooli || t == Thing::Rynkky;
}

const char* thingName(Thing t)
{
    switch (t) {
        case Thing::Kalja: return "PIVO";
        case Thing::Lonkka: return "JAGUAR";
        case Thing::Vodka: return "VODKA";
        case Thing::Tyhja: return "TYH\xC3\x84 PULLO";
        case Thing::Lenkki: return "MAKKARA";
        case Thing::Kala: return "VOBLA";
        case Thing::Omppo: return "ANTONOVKA";
        case Thing::Bansku: return "BANAANI";
        case Thing::Ketjut: return "KETJUT";
        case Thing::Veitsi: return "VEITSI";
        case Thing::Kivi: return "KIVI";
        case Thing::Pamppu: return "PAMPPU";
        case Thing::Bootsit: return "VERKKARIT";
        case Thing::Granata: return "KRANAATTI";
        case Thing::Pistooli: return "MAKAROV";
        case Thing::Rynkky: return "RYNKKY";
        case Thing::Lipas: return "LIPAS";
        case Thing::None: break;
    }
    return "";
}
