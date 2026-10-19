#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "automata/nfa.hpp"

namespace funny::automata {

struct NfaError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::string serialize(const Nfa& nfa);

Nfa deserialize_nfa(std::string_view text);

}  // namespace funny::automata