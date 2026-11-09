#include "printerweb.hpp"
#include "level.hpp"
#include "player.hpp"
#include "scoreboard.hpp"
#include "inventory.hpp"
#include "common.hpp"
#include <string>

using std::string;
using std::to_string;
using common::jsonString;
using common::webSend;

void PrinterWeb::clear()
{
    webSend("{\"t\":\"clear\"}");
}

void PrinterWeb::printChar(string c)
{
    webSend("{\"t\":\"text\",\"s\":" + jsonString(c) + "}");
}

void PrinterWeb::printScore(const ScoreBoard* scoreBoard)
{
    const uint32_t total = scoreBoard->mummot + scoreBoard->kaikki;
    webSend("{\"t\":\"score\",\"mummot\":" + to_string(scoreBoard->mummot) +
            ",\"kaikki\":" + to_string(scoreBoard->kaikki) + ",\"total\":" + to_string(total) + "}");
}

void PrinterWeb::printHelp()
{
    printChar("PПLTSAЯI 2 · Русский Мир\n");
}

void PrinterWeb::print(Level* l)
{
    string j = "{\"t\":\"frame\",\"msg\":" + jsonString(msg) +
               ",\"lvl\":" + jsonString(levelName) +
               ",\"cam\":[" + to_string(l->cameraX) + "," + to_string(l->cameraY) + "]";

    if (player) {
        const auto& s = player->stats;
        j += ",\"hud\":[" + to_string(s.money) + "," + to_string(s.health) + "," +
             to_string(s.promilles) + "," + to_string(s.hunger) + "]";
        j += ",\"pos\":[" + to_string(player->coord.x) + "," + to_string(player->coord.y) + "]";

        j += ",\"snd\":[";
        bool first = true;
        for (const auto& h : l->audio.getHeardSounds(player->coord)) {
            if (h.text.empty()) continue;
            const bool loud = h.type == SoundType::Gunshot || h.type == SoundType::Explosion || h.type == SoundType::Siren;
            j += string(first ? "" : ",") + "[" + jsonString(h.text) + "," + (loud ? "1" : "0") + "]";
            first = false;
        }
        j += "]";

        j += ",\"inv\":[";
        first = true;
        for (const auto& item : overlayItems(&player->inventory)) {
            if (item.count == 0) continue;
            j += string(first ? "" : ",") + "[" + jsonString(item.name) + "," + to_string(item.count) + "]";
            first = false;
        }
        if (player->inventory.propusk) j += string(first ? "" : ",") + "[\"PROPUSK\"," + to_string(player->inventory.propusk) + "]";
        j += "]";
    }

    // map rows as runs of glyphs sharing an SGR colour
    j += ",\"rows\":[";
    for (uint32_t y = 0; y < common::VIEWPORT_H; ++y) {
        j += y ? ",[" : "[";
        string text;
        string color;
        bool firstRun = true;
        auto flush = [&]() {
            if (text.empty()) return;
            j += string(firstRun ? "" : ",") + "[" + jsonString(text) + "," + jsonString(color) + "]";
            firstRun = false;
            text.clear();
        };
        for (const auto& cell : rowCells(l, y)) {
            if (cell.second != color) { flush(); color = cell.second; }
            text += cell.first;
        }
        flush();
        j += "]";
    }
    j += "]}";
    webSend(j);
}

void PrinterWeb::printInventoryOverlay(Inventory* inventory, int selectedRow)
{
    string j = "{\"t\":\"invmenu\",\"sel\":" + to_string(selectedRow) + ",\"items\":[";
    bool first = true;
    for (const auto& item : overlayItems(inventory)) {
        string use = item.useLabel;
        while (!use.empty() && use.back() == ' ') use.pop_back();
        j += string(first ? "" : ",") + "[" + jsonString(item.name) + "," + to_string(item.count) + "," +
             jsonString(use) + "," + (item.canThrow ? "1" : "0") + "]";
        first = false;
    }
    webSend(j + "]}");
}

void PrinterWeb::printAimLine(Level* level, uint32_t px, uint32_t py, float angle)
{
    string j = "{\"t\":\"aim\",\"cells\":[";
    bool first = true;
    for (const auto& c : aimCells(level, px, py, angle)) {
        j += string(first ? "" : ",") + "[" + to_string(c.first) + "," + to_string(c.second) + "]";
        first = false;
    }
    webSend(j + "]}");
}
