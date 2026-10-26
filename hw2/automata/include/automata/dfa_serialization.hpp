#pragma once

#include <string>
#include <string_view>

#include "automata/dfa.hpp"

namespace funny::automata {

std::string serialize(const Dfa& dfa);

Dfa deserialize(std::string_view text);

}  // namespace funny::automata