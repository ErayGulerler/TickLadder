#include "map_book.hpp"
#include "tickladder/book.hpp"
#include "tickladder/replay.hpp"

#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {
using namespace tickladder;

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void boundaries() {
    LadderBook ladder;
    MapBook oracle;
    auto apply = [&](Update update) {
        check(ladder.apply(update) && oracle.apply(update), "valid tick rejected");
        check(ladder.quote() == oracle.quote(), "boundary quote mismatch");
    };
    check(ladder.quote() == Quote{}, "initial quote");
    // Fill and drain in best-first order so every level must become visible.
    for (Side side : {Side::bid, Side::ask}) {
        for (int offset = 0; offset < tick_count; ++offset) {
            apply({side, first_tick + offset, static_cast<std::uint32_t>(offset + 1)});
        }
        for (int i = 0; i < tick_count; ++i) {
            const int offset = side == Side::bid ? tick_count - 1 - i : i;
            apply({side, first_tick + offset, 0});
        }
        check(ladder.quote() == Quote{}, "side did not empty");
    }
    apply({Side::bid, first_tick, std::numeric_limits<std::uint32_t>::max()});
    apply({Side::ask, first_tick + 64, 8});
    const Quote saved = ladder.quote();
    for (int bad : {std::numeric_limits<int>::min(), first_tick - 1,
                    first_tick + tick_count, std::numeric_limits<int>::max()}) {
        check(!ladder.apply({Side::bid, bad, 1}), "bad price accepted");
        check(ladder.quote() == saved, "rejection changed quote");
    }
}

void differential() {
    for (unsigned seed : {2u, 37u, 2026u}) {
        std::mt19937 random(seed);
        LadderBook ladder;
        MapBook oracle;
        for (int i = 0; i < 100000; ++i) {
            const Side side = random() % 2 ? Side::ask : Side::bid;
            const int offset = static_cast<int>(random() % tick_count);
            const auto size = static_cast<std::uint32_t>(random() % 16);
            const Update update{side, first_tick + offset, size};
            check(ladder.apply(update) && oracle.apply(update), "valid update rejected");
            if (ladder.quote() != oracle.quote()) {
                throw std::runtime_error("quote mismatch at seed " + std::to_string(seed)
                                         + " event " + std::to_string(i));
            }
        }
    }
}

void invalid_tapes() {
    const std::string header = "sequence,side,tick,size\n";
    for (const std::string row : {"3,A,10001,9", "1,A,10001,9", "0,A,10001,9",
            "2,S,10001,9", "2,A,9999,9", "2,A,14096,1", "2,A,10001,-1",
            "2,A,10001,4294967296", "2,A,10001,2x", "2,A,10001,2,", "2,A,10001",
            "2,A,,1", ""}) {
        std::istringstream input(header + "1,B,10000,5\n" + row + "\n");
        std::ostringstream output;
        bool rejected = false;
        try {
            replay(input, output);
        } catch (const std::runtime_error& error) {
            rejected = std::string_view(error.what()).starts_with("line 3:");
        }
        check(rejected, "invalid row was not identified");
        check(output.str() == "sequence,bid_tick,bid_size,ask_tick,ask_size\n1,10000,5,,\n",
              "invalid row published a quote");
    }
    std::istringstream windows("sequence,side,tick,size\r\n1,A,14095,4294967295\r\n");
    std::ostringstream output;
    replay(windows, output);
    check(output.str().ends_with("1,,,14095,4294967295\n"), "CRLF / upper bounds");
}
} // namespace

int main() {
    try {
        boundaries();
        differential();
        invalid_tapes();
        std::cout << "All book and replay checks passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
