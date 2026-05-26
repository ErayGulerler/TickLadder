#pragma once

#include <cstdint>

namespace tickladder {

inline constexpr int first_tick = 10000;
inline constexpr int tick_count = 4096;

enum class Side { bid, ask };

struct Update {
    Side side;
    int tick;
    std::uint32_t size;
};

struct Level {
    int tick{};
    std::uint32_t size{};
    bool operator==(const Level&) const = default;
};

// An empty side has both fields set to zero. Valid prices start at first_tick.
struct Quote {
    Level bid;
    Level ask;
    bool operator==(const Quote&) const = default;
};

} // namespace tickladder
