#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

// Tipi di base condivisi da Model, View e Controller (senza dipendenze da raylib).

enum class Direction
{
    Up,    // y - 1  (sullo schermo isometrico: in alto a destra)
    Down,  // y + 1  (in basso a sinistra)
    Left,  // x - 1  (in alto a sinistra)
    Right  // x + 1  (in basso a destra)
};

struct Cell
{
    int x = 0;
    int y = 0;

    bool operator==(const Cell &other) const { return x == other.x && y == other.y; }
    bool operator!=(const Cell &other) const { return !(*this == other); }
};

struct CellHash
{
    std::size_t operator()(const Cell &c) const noexcept
    {
        std::uint64_t k = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.x)) << 32) |
                          static_cast<std::uint32_t>(c.y);
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        return static_cast<std::size_t>(k);
    }
};

inline Cell moveCell(Cell c, Direction d)
{
    switch (d)
    {
    case Direction::Up: c.y -= 1; break;
    case Direction::Down: c.y += 1; break;
    case Direction::Left: c.x -= 1; break;
    case Direction::Right: c.x += 1; break;
    }
    return c;
}

inline Direction oppositeOf(Direction d)
{
    switch (d)
    {
    case Direction::Up: return Direction::Down;
    case Direction::Down: return Direction::Up;
    case Direction::Left: return Direction::Right;
    case Direction::Right: return Direction::Left;
    }
    return Direction::Up;
}

inline bool areOpposite(Direction a, Direction b) { return oppositeOf(a) == b; }

inline int manhattan(Cell a, Cell b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

inline int chebyshev(Cell a, Cell b)
{
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return dx > dy ? dx : dy;
}

enum class GameMode
{
    SinglePlayer,
    VersusCpu
};

// Colori disponibili per i serpenti (l'RGB vero e proprio e' definito nella View)
constexpr int kSkinCount = 5;

inline const char *skinName(int skin)
{
    static const char *names[kSkinCount] = {"Verde", "Blu", "Fucsia", "Rosa", "Giallo"};
    return names[((skin % kSkinCount) + kSkinCount) % kSkinCount];
}
