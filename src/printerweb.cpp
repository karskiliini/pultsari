#include "printerweb.hpp"
#include "mask.hpp"
#include "animation.hpp"
#include "level.hpp"
#include "building.hpp"
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
    printChar("GRAND TYR MOSCOW · Русский Мир\n");
}

// buildings and streets for the overview map (Tab in the browser)
void PrinterWeb::sendCityMap(Level* l)
{
    string j = "{\"t\":\"map\",\"w\":" + to_string(common::SIZEX) + ",\"h\":" + to_string(common::SIZEY) + ",\"b\":[";
    bool first = true;
    for (auto b : l->buildings) {
        j += string(first ? "" : ",") + "[" + to_string(b->wallLeft) + "," + to_string(b->wallTop) + "," +
             to_string(b->wallRight) + "," + to_string(b->wallBot) + "," +
             jsonString(b->type == EFlats ? string() : b->getName()) + "," + // blocks of flats stay unnamed on the map
             to_string((int)b->type) + "]";
        first = false;
    }
    j += "],\"r\":[";
    first = true;
    for (const auto& r : l->roads) {
        j += string(first ? "" : ",") + "[" + (r.horizontal ? "1" : "0") + "," + to_string(r.pos) + "," + jsonString(r.name) + "]";
        first = false;
    }
    webSend(j + "]}");
}

void PrinterWeb::print(Level* l)
{
    if (++framesSinceMap > 40) { framesSinceMap = 0; sendCityMap(l); }
    string j = "{\"t\":\"frame\",\"msg\":" + jsonString(msg) +
               ",\"lvl\":" + jsonString(levelName) +
               ",\"cam\":[" + to_string(l->cameraX) + "," + to_string(l->cameraY) + "]" +
               ",\"cols\":" + to_string(viewW) + (l->playerDead ? ",\"dead\":1" : "");

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
    for (uint32_t y = 0; y < viewH; ++y) {
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
    j += "]";

    // speech bubbles: drawn by the page, translucent and fading, so people show through them
    bubblesAsOverlay = true;
    if (common::animsEnabled) {
        j += ",\"bub\":[";
        bool firstBub = true;
        for (const auto a : l->animations) {
            if (!a->isBubble || (mask && !mask->visible(a->source))) continue;
            const int bx = (int)a->coord.x - l->cameraX, by = (int)a->coord.y - l->cameraY;
            if (by < 0 || by >= (int)viewH) continue;
            const uint32_t left = a->frames ? a->ttl * 100 / a->frames : 0;
            j += string(firstBub ? "" : ",") + "[" + to_string(bx) + "," + to_string(by) + "," + jsonString(a->text) + "," + to_string(left) + "]";
            firstBub = false;
        }
        j += "]";
    }

    // what the player can see: one hex digit per 4 cells, so blood and vomit stay hidden out of sight
    if (mask) {
        static const char* HEX = "0123456789abcdef";
        j += ",\"vis\":[";
        for (uint32_t y = 0; y < viewH; ++y) {
            string row;
            for (uint32_t x = 0; x < viewW; x += 4) {
                int bits = 0;
                for (uint32_t k = 0; k < 4; ++k) {
                    const int wx = (int)(x + k) + l->cameraX, wy = (int)y + l->cameraY;
                    if (x + k < viewW && wx >= 0 && wy >= 0 && mask->visible(Coord{ (uint32_t)wx, (uint32_t)wy })) bits |= 1 << k;
                }
                row += HEX[bits];
            }
            j += string(y ? "," : "") + "\"" + row + "\"";
        }
        j += "]";
    }
    j += "}";
    webSend(j);
}

void PrinterWeb::printInventoryOverlay(Inventory* inventory, int selectedRow, bool throwMenu)
{
    string j = "{\"t\":\"invmenu\",\"sel\":" + to_string(selectedRow) + ",\"throw\":" + (throwMenu ? "1" : "0") + ",\"items\":[";
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
    // the browser draws a laser from the player along the angle, as far as the cells reach
    string j = "{\"t\":\"aim\",\"risk\":" + to_string(aimRisk) + ",\"low\":" + (aimLength < 20 ? "1" : "0") +
               ",\"a\":" + to_string(angle) + ",\"p\":[" + to_string((int)px - level->cameraX) + "," +
               to_string((int)py - level->cameraY) + "],\"cells\":[";
    bool first = true;
    for (const auto& c : aimCells(level, px, py, angle, aimLength)) {
        j += string(first ? "" : ",") + "[" + to_string(c.first) + "," + to_string(c.second) + "]";
        first = false;
    }
    webSend(j + "]}");
}
