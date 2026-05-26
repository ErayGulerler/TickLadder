#pragma once

#include "tickladder/types.hpp"

#include <map>

namespace tickladder {

// A plain ordered map is also the independent oracle for randomized tests.
class MapBook {
    std::map<int, std::uint32_t> bids_;
    std::map<int, std::uint32_t> asks_;

public:
    [[nodiscard]] bool apply(Update update) {
        if (update.tick < first_tick || update.tick >= first_tick + tick_count) {
            return false;
        }
        auto& levels = update.side == Side::bid ? bids_ : asks_;
        if (update.size == 0) levels.erase(update.tick);
        else levels.insert_or_assign(update.tick, update.size);
        return true;
    }

    [[nodiscard]] Quote quote() const {
        Quote top{};
        if (!bids_.empty()) {
            const auto& [tick, size] = *bids_.rbegin();
            top.bid = {tick, size};
        }
        if (!asks_.empty()) {
            const auto& [tick, size] = *asks_.begin();
            top.ask = {tick, size};
        }
        return top;
    }
};

} // namespace tickladder
