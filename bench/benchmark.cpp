#include "map_book.hpp"
#include "tickladder/book.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <stdexcept>
#include <vector>

namespace {
using namespace tickladder;
using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;
constexpr unsigned seed = 3367;
constexpr int warmup_count = 8192;
constexpr int measured_count = 250000;
constexpr int trials = 5;

std::vector<Update> make_tape(int width) {
    std::mt19937 random(seed);
    std::vector<Update> tape;
    tape.reserve(warmup_count + measured_count);
    for (int i = 0; i < warmup_count + measured_count; ++i) {
        const Side side = (random() % 2 == 0) ? Side::bid : Side::ask;
        const int distance = static_cast<int>(random() % width);
        const int middle = first_tick + tick_count / 2;
        const int tick = side == Side::bid ? middle - 1 - distance : middle + distance;
        const bool remove = random() % 4 == 0;
        const auto size = static_cast<std::uint32_t>(random() % 500 + 1);
        tape.push_back({side, tick, remove ? 0u : size});
    }
    return tape;
}

// Consume both fields of both quotes on every iteration, including unchanged quotes.
std::uint64_t mix(std::uint64_t digest, Quote top) {
    const auto pack = [](Level level) {
        return (static_cast<std::uint64_t>(level.tick) << 32) | level.size;
    };
    return ((digest ^ pack(top.bid)) * 1099511628211ULL) ^ pack(top.ask);
}

template<class Book>
std::uint64_t step(Book& book, Update update, std::uint64_t digest) {
    if (!book.apply(update)) throw std::logic_error("invalid benchmark update");
    return mix(digest, book.quote());
}

std::int64_t percentile(const std::vector<std::int64_t>& sorted, unsigned percent) {
    // Nearest rank with a zero-based vector.
    return sorted[(sorted.size() * percent + 99) / 100 - 1];
}

template<class Book>
std::uint64_t measure(const char* name, const char* profile, int trial,
                      std::span<const Update> tape) {
    Book bulk, sampled;
    for (Update update : tape.first(warmup_count)) {
        if (!bulk.apply(update) || !sampled.apply(update)) throw std::logic_error("invalid warmup");
    }
    const auto measured = tape.subspan(warmup_count);
    std::uint64_t digest = 0;
    const auto start = Clock::now();
    for (Update update : measured) digest = step(bulk, update, digest);
    const auto elapsed = std::chrono::duration<double>(Clock::now() - start).count();

    std::vector<std::int64_t> times;
    times.reserve(measured.size());
    std::uint64_t sampled_digest = 0;
    for (Update update : measured) {
        const auto before = Clock::now();
        sampled_digest = step(sampled, update, sampled_digest);
        times.push_back(std::chrono::duration_cast<Nanoseconds>(Clock::now() - before).count());
    }
    if (sampled_digest != digest) throw std::logic_error("timing passes disagree");
    std::sort(times.begin(), times.end());
    std::cout << profile << ',' << name << ',' << trial << ',' << measured.size()
              << ',' << std::fixed << std::setprecision(0) << measured.size() / elapsed
              << ',' << percentile(times, 50) << ',' << percentile(times, 99)
              << ',' << times.back() << ',' << digest << '\n';
    return digest;
}

void compare(int width, const char* profile) {
    const auto tape = make_tape(width);
    for (int trial = 1; trial <= trials; ++trial) {
        std::uint64_t tree, ladder;
        if (trial % 2 == 1) {
            tree = measure<MapBook>("map", profile, trial, tape);
            ladder = measure<LadderBook>("ladder", profile, trial, tape);
        } else {
            ladder = measure<LadderBook>("ladder", profile, trial, tape);
            tree = measure<MapBook>("map", profile, trial, tape);
        }
        if (tree != ladder) throw std::logic_error("map and ladder produced different quotes");
    }
}

void clock_probe() {
    std::vector<std::int64_t> times;
    times.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        const auto before = Clock::now();
        const auto after = Clock::now();
        times.push_back(std::chrono::duration_cast<Nanoseconds>(after - before).count());
    }
    std::sort(times.begin(), times.end());
    std::cerr << "clock_pair_p50_ns=" << percentile(times, 50)
              << " clock_pair_p99_ns=" << percentile(times, 99)
              << " ladder_bytes=" << sizeof(LadderBook) << '\n';
}
} // namespace

int main() {
    try {
        clock_probe();
        std::cout << "profile,book,trial,updates,updates_per_s,p50_ns,p99_ns,max_ns,digest\n";
        compare(32, "near");
        compare(2048, "wide");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
