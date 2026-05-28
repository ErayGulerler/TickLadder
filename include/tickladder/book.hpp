#pragma once

#include "tickladder/types.hpp"

#include <array>
#include <bit>

namespace tickladder {

class LadderBook {
    struct Ladder {
        static_assert(tick_count == 64 * 64);
        std::array<std::uint32_t, tick_count> sizes{};
        std::array<std::uint64_t, 64> occupied{};
        std::uint64_t groups{};

        void set(int offset, std::uint32_t size) noexcept {
            const int group = offset / 64;
            const auto tick_bit = std::uint64_t{1} << (offset % 64);
            const auto group_bit = std::uint64_t{1} << group;
            sizes[offset] = size;
            if (size != 0) {
                occupied[group] |= tick_bit;
                groups |= group_bit;
            } else {
                occupied[group] &= ~tick_bit;
                if (occupied[group] == 0) {
                    groups &= ~group_bit;
                }
            }
        }

        Level best(Side side) const noexcept {
            if (groups == 0) return {};
            // Pick the outermost occupied group, then the outermost tick in it.
            const auto extreme = [side](std::uint64_t bits) {
                return side == Side::bid ? 63 - std::countl_zero(bits)
                                         : std::countr_zero(bits);
            };
            const int group = extreme(groups);
            const int offset = group * 64 + extreme(occupied[group]);
            return {first_tick + offset, sizes[offset]};
        }
    };

    Ladder bids_;
    Ladder asks_;

public:
    [[nodiscard]] bool apply(Update update) noexcept {
        if (update.tick < first_tick || update.tick >= first_tick + tick_count) {
            return false;
        }
        auto& ladder = update.side == Side::bid ? bids_ : asks_;
        ladder.set(update.tick - first_tick, update.size);
        return true;
    }

    [[nodiscard]] Quote quote() const noexcept {
        return {bids_.best(Side::bid), asks_.best(Side::ask)};
    }
};

} // namespace tickladder
