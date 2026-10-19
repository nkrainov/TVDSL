#pragma once

#include <string>
#include <vector>

#include "automata/dfa.hpp"
#include "automata/nfa.hpp"

namespace funny::builder {

struct Diagnostic {
    enum class Kind {
        Override,       
        NeverAccepted,  
    };
    Kind kind;
    std::string message;
};

struct DeterminizeResult {
    automata::Dfa dfa;
    std::vector<Diagnostic> warnings;
};

DeterminizeResult determinize(const automata::Nfa& nfa);

}  // namespace funny::builder