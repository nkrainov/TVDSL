#pragma once

#include <vector>

#include "automata/nfa.hpp"
#include "lexspec/parser.hpp"

namespace funny::builder {

automata::Nfa build_nfa(const std::vector<lexspec::TokenSpec>& specs);

}  // namespace funny::builder