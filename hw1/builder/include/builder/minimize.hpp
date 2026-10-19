#pragma once

#include "automata/dfa.hpp"

namespace funny::builder {

automata::Dfa minimize(const automata::Dfa& dfa);

}  // namespace funny::builder