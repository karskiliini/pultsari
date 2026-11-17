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

bool isFood(Thing t)
{
    return t == Thing::Lenkki || t == Thing::Kala || t == Thing::Omppo || t == Thing::Bansku;
}

const char* thingName(Thing t)
{
    switch (t) {
        case Thing::Kalja: return "PIVO";
        case Thing::Lonkka: return "JAGUAR";
        case Thing::Vodka: return "VODKA";
        case Thing::Tyhja: return "TYHJ\xC3\x84 PULLO";
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

Thing throwNumToThing(uint32_t n)
{
    switch (n) {
        case 1: return Thing::Kalja;
        case 2: return Thing::Lonkka;
        case 3: return Thing::Lenkki;
        case 4: return Thing::Ketjut;
        case 5: return Thing::Veitsi;
        case 6: return Thing::Kivi;
        case 7: return Thing::Pamppu;
        case 8: return Thing::Bootsit;
        case 9: return Thing::Kala;
        case 10: return Thing::Omppo;
        case 11: return Thing::Bansku;
        case 13: return Thing::Vodka;
        case 14: return Thing::Granata;
        case 15: return Thing::Tyhja;
        default: return Thing::None;
    }
}

uint32_t thingToThrowNum(Thing t)
{
    for (uint32_t n = 1; n <= 15; ++n)
        if (t != Thing::None && throwNumToThing(n) == t) return n;
    return 0;
}

char coatColour(Coat c)
{
    switch (c) {
        case Coat::Poliisi: return 'B';
        case Coat::Sotilas: return 'O';
        case Coat::Musta: return 'D';
        case Coat::Siviili: return 'N';
        default: return 'J';
    }
}

const char* coatName(Coat c)
{
    switch (c) {
        case Coat::Poliisi: return "poliisin takki";
        case Coat::Sotilas: return "sotilaan takki";
        case Coat::Musta: return "musta nahkatakki";
        case Coat::Siviili: return "takki";
        default: return "oma takki";
    }
}
