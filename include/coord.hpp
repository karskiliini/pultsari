#ifndef COORD_HPP
#define COORD_HPP

#include <cstdint>

template<typename T>
class Coordinate {
public:
    Coordinate() {};
    Coordinate(T ax, T ay) : x(ax), y(ay) { }
    Coordinate(const Coordinate& c) { x = c.x; y = c.y; }

    virtual ~Coordinate() = default;

    Coordinate& operator=(const Coordinate& c) {
        x = c.x; y = c.y;
        return *this;
    }
    bool operator==(const Coordinate& c) const { return ((x == c.x) && (y == c.y)); }
    bool operator!=(const Coordinate& c) const { return !(*this == c); }
    Coordinate operator+(const Coordinate& c) const { return Coordinate { x + c.x, y + c.y }; }
    Coordinate operator-(const Coordinate& c) const { return Coordinate { x - c.x, y - c.y }; }
    Coordinate& operator+=(const Coordinate& c) { x += c.x; y += c.y; return *this; }
    Coordinate& operator-=(const Coordinate& c) { x -= c.x; y -= c.y; return *this; }

    T distance(const Coordinate& c) const
    {
        return distance(*this, c);
    }

    static T distance(Coordinate c1, const Coordinate& c2)
    {
        T distance = 0;

        while(c1 != c2) {
            if (c1.x < c2.x) ++c1.x;
            else if (c1.x > c2.x) --c1.x;
            if (c1.y < c2.y) ++c1.y;
            else if (c1.y > c2.y) --c1.y;

            ++distance;
        }
        return distance;
    }

    T x;
    T y;
};

using Coord = Coordinate<uint32_t>;

#endif
