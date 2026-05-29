#pragma once

#include <iosfwd>

namespace tickladder {

// Throws std::runtime_error on a bad row. All previous output remains a valid prefix.
void replay(std::istream& input, std::ostream& output);

} // namespace tickladder
