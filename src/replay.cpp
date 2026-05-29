#include "tickladder/book.hpp"
#include "tickladder/replay.hpp"

#include <array>
#include <charconv>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace tickladder {
namespace {

void trim_cr(std::string& line) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
}

std::array<std::string_view, 4> fields(std::string_view row) {
    std::array<std::string_view, 4> parts;
    for (int index = 0; index < 3; ++index) {
        const auto comma = row.find(',');
        if (comma == row.npos) throw std::runtime_error("expected four CSV fields");
        parts[index] = row.substr(0, comma);
        row.remove_prefix(comma + 1);
    }
    parts[3] = row;
    return parts;
}

template<class T>
T number(std::string_view field) {
    T value{};
    const auto end = field.data() + field.size();
    const auto parsed = std::from_chars(field.data(), end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        throw std::runtime_error("invalid integer");
    }
    return value;
}

Side parse_side(std::string_view side) {
    if (side == "B") return Side::bid;
    if (side == "A") return Side::ask;
    throw std::runtime_error("expected B or A");
}

void write_level(std::ostream& out, Level level) {
    if (level.size) out << level.tick << ',' << level.size;
    else out << ',';
}

} // namespace

void replay(std::istream& input, std::ostream& output) {
    LadderBook book;
    std::string row;
    std::getline(input, row);
    trim_cr(row);
    if (row != "sequence,side,tick,size") throw std::runtime_error("line 1: invalid header");
    output << "sequence,bid_tick,bid_size,ask_tick,ask_size\n";
    std::uint64_t last_sequence = 0;
    std::size_t line = 1;
    while (std::getline(input, row)) {
        ++line;
        trim_cr(row);
        try {
            const auto part = fields(row);
            const auto sequence = number<std::uint64_t>(part[0]);
            if (sequence == 0 || sequence - 1 != last_sequence) {
                throw std::runtime_error("non-contiguous sequence after " + std::to_string(last_sequence));
            }
            const Update update{parse_side(part[1]), number<int>(part[2]),
                                number<std::uint32_t>(part[3])};
            if (!book.apply(update)) throw std::runtime_error("price outside ladder");
            const Quote top = book.quote();
            output << sequence << ',';
            write_level(output, top.bid);
            output << ',';
            write_level(output, top.ask);
            output << '\n';
            last_sequence = sequence;
        } catch (const std::runtime_error& error) {
            throw std::runtime_error("line " + std::to_string(line) + ": " + error.what());
        }
    }
    if (input.bad() || !input.eof()) throw std::runtime_error("input read failed");
    output.flush();
    if (!output) throw std::runtime_error("output write failed");
}

} // namespace tickladder
