#pragma once

#include <bitset>
#include <string_view>
#include <vector>

#include "automata/types.hpp"

namespace funny::automata {

using SymbolSet = std::bitset<kAlphabetSize>;

struct NfaEdge {
    SymbolSet symbols;
    StateId to;
};

class Nfa {
public:
    RuleId add_rule(std::string name, bool skip = false);
    StateId add_state();
    void add_epsilon(StateId from, StateId to);
    void add_edge(StateId from, const SymbolSet& symbols, StateId to);
    void set_accept(StateId state, RuleId rule);
    void set_start(StateId state);

    std::size_t state_count() const { return accept_.size(); }
    StateId start() const { return start_; }
    const std::vector<Rule>& rules() const { return rules_; }
    RuleId accept(StateId state) const { return accept_.at(state); }
    const std::vector<StateId>& epsilon(StateId state) const { return eps_.at(state); }
    const std::vector<NfaEdge>& edges(StateId state) const { return edges_.at(state); }

    std::vector<StateId> epsilon_closure(const std::vector<StateId>& seeds) const;

    RuleId match_full(std::string_view input) const;

private:
    void check_state(StateId s) const;

    std::vector<Rule> rules_;
    std::vector<RuleId> accept_;
    std::vector<std::vector<StateId>> eps_;
    std::vector<std::vector<NfaEdge>> edges_;
    StateId start_ = kNoState;
};

}  // namespace funny::automata