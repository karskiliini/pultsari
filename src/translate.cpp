#include "translate.hpp"
#include <cctype>
#include <map>
#include <vector>

using std::string;

// Every bit of Russian in the game, in Finnish. Keys are in capitals; lookups
// ignore case and trailing punctuation, and longer lines fall back to their sentences.
static const std::map<string, string>& dictionary()
{
    static const std::map<string, string> d = {
        // the metro
        { "СТАНЦИЯ", "asema" },
        { "ВЫХОД", "uloskäynti" },
        { "ОСТОРОЖНО, ДВЕРИ ЗАКРЫВАЮТСЯ", "varokaa, ovet sulkeutuvat" },
        { "ТЫ ЧЁ, ОХРЕНЕЛ?! ПАЦАНЫ", "oletko sekaisin?! Pojat" },
        { "НУ ВСЁ, ТЕБЕ КОНЕЦ", "no niin, nyt sinä kuolet" },
        { "ОЙ! ХУЛИГАН! МИЛИЦИЯ", "voi! Huligaani! Miliisi" },
        { "БРАТАН, ЕСТЬ СОТКА", "veli, onko satasta" },
        { "ЕСТЬ КОЕ-ЧТО. ХОЧЕШЬ", "minulla on jotain. Haluatko" },
        { "БУДЕШЬ", "otatko" },
        { "БИЛЕТ", "lippu" },
        { "МУЖИК, ХОЧЕШЬ ЗАРАБОТАТЬ? КОНТРАКТ", "hei mies, haluatko tienata? Sopimus" },
        { "ПОДПИШИ. СЕЙЧАС", "allekirjoita. Nyt" },
        { "НОРМАЛЬНО", "ihan ok" },
        { "НУ ЛАДНО", "no okei" },
        { "ВРЁШЬ", "valehtelet" },
        { "КАК ХОЧЕШЬ", "kuten haluat" },
        { "БЕЗ ДЕНЕГ - БЕЗ ТОВАРА", "ei rahaa, ei kamaa" },
        { "ТЫ МЕНЯ НЕ УВАЖАЕШЬ", "etkö sinä kunnioita minua" },
        { "ПРОХОДИ", "jatka matkaa" },
        { "МОЛОДЕЦ", "hyvä poika" },
        { "ПОДУМАЙ", "mieti sitä" },
        { "ЧЁ УСТАВИЛСЯ", "mitä tuijotat" },
        { "ТЫ ЧЁ ТОЛКАЕШЬСЯ", "mitä sinä tönit" },
        // signs and buildings
        { "ХРУЩЁВКА", "hruštšovka, 60-luvun elementtikerrostalo" },
        { "ПАНЕЛЬКА", "elementtitalo" },
        { "ОБЩЕЖИТИЕ", "työläisasuntola" },
        { "ПРОДУКТЫ", "elintarvikkeet" },
        { "КИОСК", "kioski" },
        { "МАГАЗИН", "kauppa" },
        { "ОРУЖИЕ", "aseet" },
        { "БУТЫРКА", "Butyrkan vankila" },
        { "ГИПЕРМАРКЕТ", "hypermarket" },
        { "ГОРЯЩИЙ ГИПЕРМАРКЕТ", "palava hypermarket" },
        { "ПРИЁМ СТЕКЛОТАРЫ", "pullonpalautus" },
        { "М МЕТРО", "metro" },
        { "МЕТРО", "metro" },
        { "АЗС", "huoltoasema" },
        { "НПЗ", "öljynjalostamo" },
        { "Н  П  З", "öljynjalostamo" },
        { "СБЕРБАНК", "Sberbank, valtion pankki" },
        { "ВОДКА", "vodka" },
        { "ВОЕНКОМАТ", "kutsuntatoimisto" },
        { "МИЛИЦИЯ", "miliisi" },
        { "ФСБ", "FSB, turvallisuuspalvelu" },
        { "МОСКВА", "Moskova" },
        { "ТОВАРЫ", "tavarat" },
        { "ПРОТОКОЛ", "pöytäkirja" },
        { "НЕ КУРИТЬ", "tupakointi kielletty" },
        { "БЕЛАЯ ГОРЯЧКА", "juoppohulluus" },
        { "РУССКИЙ МИР", "venäläinen maailma" },
        { "БАЛАЛАЙКА", "balalaikka" },
        { "КАЛИНКА", "Kalinka" },
        { "СЕМЕЧКИ", "auringonkukansiemenet" },
        { "ГРУЗ", "lasti" },
        { "ГРУЗ 200", "lasti 200, eli kaatuneen ruumis" },
        { "ИМПОРТОЗАМЕЩЕНИЕ", "tuonnin korvaaminen" },
        { "Ж", "Ž" },

        // things people say
        { "ВСЁ ИДЁТ ПО ПЛАНУ", "kaikki menee suunnitelman mukaan" },
        { "ЭТО ДРУГОЕ", "tämä on eri asia" },
        { "ГРАНИЦЫ РОССИИ НИГДЕ НЕ ЗАКАНЧИВАЮТСЯ", "Venäjän rajat eivät lopu missään" },
        { "МЫ ВЕРНЁМСЯ", "me palaamme" },
        { "СПАСИБО", "kiitos" },
        { "СПАСИБО, БРАТ", "kiitos, veli" },
        { "ПОКА", "heippa" },
        { "ПОШЁЛ ВОН", "painu helvettiin" },
        { "ПОШЁЛ", "häivy" },
        { "ПОМОГИТЕ", "apua" },
        { "СВОБОДА", "vapaus" },
        { "СМЕРТЬ", "kuolema" },
        { "СТОЙ", "seis" },
        { "ДЕНЬГИ ДАВАЙ", "rahat tänne" },
        { "ДОКУМЕНТЫ", "paperit" },
        { "ИДИ СЮДА", "tule tänne" },
        { "ПРОПУСК", "kulkulupa" },
        { "ПОКАЖИ ПРОПУСК", "näytä kulkulupa" },
        { "ЕСТЬ ЧЁ", "onko mitään" },
        { "СЕМКИ ЕСТЬ", "onko siemeniä" },
        { "ЗА ВОВУ", "Vovan puolesta" },
        { "ЗА РОДИНУ", "isänmaan puolesta" },
        { "ЗА ПРЕЗИДЕНТА", "presidentin puolesta" },
        { "ПРЕДАТЕЛЬ", "petturi" },
        { "УБИТЬ ЕГО", "tappakaa hänet" },
        { "УБИЙЦА", "murhaaja" },
        { "НАЙТИ УБИЙЦУ", "etsikää murhaaja" },
        { "ТРЕВОГА", "hälytys" },
        { "ОГОНЬ", "tulta" },
        { "ОГОНЬ ПО ГОТОВНОСТИ", "tulta kun valmis" },
        { "СТРЕЛЯТЬ", "ampukaa" },
        { "ЧТО ПРОИСХОДИТ", "mitä tapahtuu" },
        { "ЭВАКУАЦИЯ", "evakuointi" },
        { "ПРЕЗИДЕНТ... НЕТ", "presidentti... ei" },
        { "ОХРАНА", "vartijat" },
        { "ШОЙГУ", "Šoigu" },
        { "ПОЗВОНИТЕ ШОЙГУ", "soittakaa Šoigulle" },
        { "ШОЙГУ! ГЕРАСИМОВ! СЮДА", "Šoigu! Gerasimov! Tänne" },
        { "ШОЙГУ! ГЕРАСИМОВ! КОГО-НИБУДЬ", "Šoigu! Gerasimov! Joku" },
        { "НА КОЛЕНИ, СВИНЬЯ", "polvillesi, sika" },
        { "Я ДЗЮДОИСТ", "olen judoka" },
        { "Я ТЕБЯ СЛОМАЮ", "murskaan sinut" },
        { "Я ВСЕМОГУЩИЙ", "olen kaikkivaltias" },
        { "Я ВСЕМОГУЩИЙ... ВСЕМОГУЩИЙ", "olen kaikkivaltias... kaikkivaltias" },
        { "ГДЕ МОЙ ЧАЙ", "missä on teeni" },
        { "УКРАИНА БУДЕТ НАША", "Ukraina on vielä meidän" },
        { "ЗАПАД НАС БОИТСЯ", "länsi pelkää meitä" },
        { "НАДО ПОЗВОНИТЬ КИМ ЧЕН ЫНУ", "pitää soittaa Kim Jong-unille" },
        { "СТОЛИК НАКРЫТ", "onko pöytä katettu" },
        { "ЭТО... КОНЕЦ... ДЗЮДОИСТ... НЕ МОЖЕТ... ПРОИГРАТЬ",
          "tämä... on loppu... judoka... ei voi... hävitä" },
    };
    return d;
}

static bool isCyrillic(const string& s, size_t i)
{
    const unsigned char b = s[i];
    return (b == 0xD0 || b == 0xD1) && i + 1 < s.size();
}

static bool isLowerCyrillic(const string& s, size_t i)
{
    const unsigned char b0 = s[i], b1 = s[i + 1];
    return (b0 == 0xD0 && b1 >= 0xB0) || (b0 == 0xD1 && b1 >= 0x80 && b1 <= 0x9F);
}

// Cyrillic and ASCII to capitals, for dictionary keys
static string upper(const string& in)
{
    string out;
    for (size_t i = 0; i < in.size(); ++i) {
        const unsigned char b0 = in[i];
        if (isCyrillic(in, i)) {
            const unsigned char b1 = in[i + 1];
            if (b0 == 0xD0 && b1 >= 0xB0 && b1 <= 0xBF) { out += (char)0xD0; out += (char)(b1 - 0x20); }
            else if (b0 == 0xD1 && b1 >= 0x80 && b1 <= 0x8F) { out += (char)0xD0; out += (char)(b1 + 0x20); }
            else if (b0 == 0xD1 && b1 == 0x91) { out += (char)0xD0; out += (char)0x81; }
            else { out += in[i]; out += in[i + 1]; }
            ++i;
        } else if (b0 < 0x80) {
            out += (char)std::toupper(b0);
        } else {
            out += in[i];
        }
    }
    return out;
}

// Finnish to capitals, for translating a shout: ä ö å š ž too
static string upperFinnish(const string& in)
{
    string out;
    for (size_t i = 0; i < in.size(); ++i) {
        const unsigned char b0 = in[i];
        const unsigned char b1 = i + 1 < in.size() ? (unsigned char)in[i + 1] : 0;
        if (b0 == 0xC3 && (b1 == 0xA4 || b1 == 0xB6 || b1 == 0xA5)) { out += in[i]; out += (char)(b1 - 0x20); ++i; }
        else if (b0 == 0xC5 && (b1 == 0xA1 || b1 == 0xBE)) { out += in[i]; out += (char)(b1 - 1); ++i; }
        else if (b0 < 0x80) out += (char)std::toupper(b0);
        else out += in[i];
    }
    return out;
}

static bool isEndPunct(char c) { return c == '.' || c == '!' || c == '?' || c == ' ' || c == ','; }

// one piece: whole, or with its trailing punctuation set aside
static bool lookup(const string& piece, string& out)
{
    const auto& d = dictionary();
    auto it = d.find(upper(piece));
    if (it != d.end()) { out = it->second; return true; }
    size_t end = piece.size();
    while (end > 0 && isEndPunct(piece[end - 1])) --end;
    if (end == piece.size() || end == 0) return false;
    it = d.find(upper(piece.substr(0, end)));
    if (it == d.end()) return false;
    string tail = piece.substr(end);
    while (!tail.empty() && (tail[0] == ' ' || tail[0] == ',')) tail.erase(0, 1);
    out = it->second + tail;
    return true;
}

// the translation in the same case as the Russian: shouted in capitals, sentences capitalised
static string caseLike(const string& russian, string t)
{
    bool hasLower = false, letters = false;
    for (size_t i = 0; i < russian.size(); ++i) {
        if (isCyrillic(russian, i)) { letters = true; if (isLowerCyrillic(russian, i)) hasLower = true; ++i; }
    }
    if (letters && !hasLower && russian.size() > 2) return upperFinnish(t);
    if (!t.empty() && t[0] >= 'a' && t[0] <= 'z' && isCyrillic(russian, 0) && !isLowerCyrillic(russian, 0))
        t[0] = (char)std::toupper(t[0]);
    return t;
}

string translateRussian(const string& run)
{
    string out;
    if (lookup(run, out)) return caseLike(run, out);

    // sentence by sentence: "ОХРАНА! ОХРАНА!!" -> "VARTIJAT! VARTIJAT!!"
    std::vector<string> pieces;
    size_t start = 0;
    for (size_t i = 0; i < run.size(); ++i) {
        const char c = run[i];
        if ((c == '!' || c == '?' || c == '.') && i + 1 < run.size() && run[i + 1] == ' ') {
            pieces.push_back(run.substr(start, i + 1 - start));
            start = i + 2;
            ++i;
        } else if ((c == '!' || c == '?' || c == '.') && i + 1 < run.size() && (run[i + 1] == '!' || run[i + 1] == '?' || run[i + 1] == '.')) {
            continue;
        }
    }
    if (start < run.size()) pieces.push_back(run.substr(start));
    if (pieces.size() < 2) return "";

    out.clear();
    for (const auto& p : pieces) {
        string t;
        t = lookup(p, t) ? caseLike(p, t) : latinize(p);
        if (!out.empty()) out += " ";
        out += t;
    }
    return out;
}

string withTranslation(const string& text)
{
    string out;
    size_t i = 0;
    while (i < text.size()) {
        if (!isCyrillic(text, i)) { out += text[i++]; continue; }

        // a run of Russian: Cyrillic words with spaces and punctuation between them
        const size_t start = i;
        size_t cyrEnd = i;
        size_t j = i;
        while (j < text.size()) {
            if (isCyrillic(text, j)) { j += 2; cyrEnd = j; }
            else if (text[j] >= '0' && text[j] <= '9' && j > start) { ++j; cyrEnd = j; } // "Груз 200"
            else if (text[j] == ' ' || text[j] == ',' || text[j] == '.' || text[j] == '!' || text[j] == '?' || text[j] == '-') ++j;
            else break;
        }
        size_t end = cyrEnd;
        while (end < text.size() && (text[end] == '!' || text[end] == '?')) ++end;
        const string run = text.substr(start, end - start);
        out += run;
        i = end;

        // already translated in the text itself
        size_t k = end;
        while (k < text.size() && text[k] == ' ') ++k;
        if (k < text.size() && text[k] == '(') continue;

        string t = translateRussian(run);
        if (t.empty()) t = latinize(run);
        out += " (" + t + ")";
    }
    return out;
}

// Russian is romanised the Finnish way.
string latinize(const string& in)
{
    static const char* lower[] = {
        "a", "b", "v", "g", "d", "e", "\xC5\xBE", "z", "i", "i", "k", "l", "m", "n", "o", "p",
        "r", "s", "t", "u", "f", "h", "ts", "t\xC5\xA1", "\xC5\xA1", "\xC5\xA1t\xC5\xA1", "", "y", "", "e", "ju", "ja",
    };
    static const char* upper[] = {
        "A", "B", "V", "G", "D", "E", "\xC5\xBD", "Z", "I", "I", "K", "L", "M", "N", "O", "P",
        "R", "S", "T", "U", "F", "H", "Ts", "T\xC5\xA1", "\xC5\xA0", "\xC5\xA0t\xC5\xA1", "", "Y", "", "E", "Ju", "Ja",
    };

    string out;
    for (size_t i = 0; i < in.size(); ++i) {
        const unsigned char b0 = in[i];
        const unsigned char b1 = i + 1 < in.size() ? (unsigned char)in[i + 1] : 0;
        uint32_t cp = 0;
        if (b0 == 0xD0 || b0 == 0xD1) cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        if (cp >= 0x410 && cp <= 0x42F)      { out += upper[cp - 0x410]; ++i; }
        else if (cp >= 0x430 && cp <= 0x44F) { out += lower[cp - 0x430]; ++i; }
        else if (cp == 0x401)                { out += "Jo"; ++i; }
        else if (cp == 0x451)                { out += "jo"; ++i; }
        else out += in[i];
    }
    return out;
}

