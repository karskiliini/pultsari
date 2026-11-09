#include "common.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdio>
#include <cstdlib>

using std::cout;
using std::endl;

namespace common {

bool random(uint32_t pct)
{
    return (uint32_t)(std::rand() % 101) <= pct;
}

bool checkBounds(Coord& coord) {
    auto ox = coord.x;
    auto oy = coord.y;

    coord.x = (coord.x == SIZEX) ? SIZEX - 1 : coord.x;
    coord.x = (coord.x > SIZEX)  ? 0 : coord.x;
    coord.y = (coord.y == SIZEY) ? SIZEY - 1 : coord.y;
    coord.y = (coord.y > SIZEY)  ? 0 : coord.y;

    return !((ox == coord.x) && (oy == coord.y));
}

void sleep(uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void frameSleep(uint32_t ms)
{
    // hold a steady frame rate: sleep until the next frame is due, not a full frame on top of the work
    using clock = std::chrono::steady_clock;
    static clock::time_point next = clock::now();
    next += std::chrono::milliseconds(ms);
    const auto now = clock::now();
    if (next < now - std::chrono::milliseconds(200)) next = now; // far behind: don't try to catch up
    std::this_thread::sleep_until(next);
}

bool webMode()
{
    static const bool web = std::getenv("PULTSARI_WEB") != nullptr;
    return web;
}

void webSend(const std::string& json)
{
    if (!webMode()) return;
    cout << '\x1e' << json << '\n' << std::flush;
}

std::string jsonString(const std::string& s)
{
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
                else out += (char)c;
        }
    }
    return out + "\"";
}

void sound(const char* name, float volume)
{
    if (!webMode()) return;
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"t\":\"snd\",\"n\":\"%s\",\"v\":%.2f}", name, volume);
    webSend(buf);
}

static int hitDx = 0, hitDy = 0;

void setHitDirection(int dx, int dy)
{
    hitDx = dx;
    hitDy = dy;
}

void hitEvent(const Coord& at, uint32_t damage)
{
    if (!webMode()) return;
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"t\":\"hit\",\"x\":%u,\"y\":%u,\"n\":%u,\"dx\":%d,\"dy\":%d}",
             at.x, at.y, damage, hitDx, hitDy);
    hitDx = hitDy = 0;
    webSend(buf);
}

void vomitEvent(const Coord& at, int dx, int dy)
{
    if (!webMode()) return;
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"t\":\"vomit\",\"x\":%u,\"y\":%u,\"dx\":%d,\"dy\":%d}", at.x, at.y, dx, dy);
    webSend(buf);
}

void shellEvent(const Coord& from, const Coord& to)
{
    if (!webMode()) return;
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"t\":\"shell\",\"x0\":%u,\"y0\":%u,\"x1\":%u,\"y1\":%u}", from.x, from.y, to.x, to.y);
    webSend(buf);
}

void boomEvent(const Coord& at, uint32_t radius)
{
    if (!webMode()) return;
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"t\":\"boom\",\"x\":%u,\"y\":%u,\"r\":%u}", at.x, at.y, radius);
    webSend(buf);
}

}