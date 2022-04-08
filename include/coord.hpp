#ifndef COORD_HPP
#define COORD_HPP

#include <cstdint>

template<typename T>
class Coordinate {
public:
    Coordinate();
    Coordinate(T ax, T ay) : x(ax), y(ay) { }
    Coordinate(const Coordinate& c) { x = c.x; y = c.y; }

    virtual ~Coordinate() = default;

    Coordinate& operator=(const Coordinate& c) {
        x = c.x; y = c.y;
        return *this;
    }
    bool operator==(const Coordinate& c) { return ((x == c.x) && (y == c.y)); }

    T distance(const Coordinate& c)
    {
        T distance = 0;
        while(*this != c) {
            if (x < c.x) ++x;
            else if (x > c.x) --x;
            if (y < c.y) ++y;
            else if (y > c.x) --y;

            ++distance;
        }
        return distance;
    }

    T x;
    T y;
};

using Coord = Coordinate<uint32_t>;

#endif
