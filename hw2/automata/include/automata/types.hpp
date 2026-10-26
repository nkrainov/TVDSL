#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace funny::automata {

using StateId = std::uint32_t;
using RuleId = std::int32_t;

inline constexpr StateId kNoState = std::numeric_limits<StateId>::max();
inline constexpr RuleId kNoRule = -1;
inline constexpr std::size_t kAlphabetSize = 256; 

struct Rule {
    std::string name;
    bool skip = false;
};

}  // namespace funny::automata