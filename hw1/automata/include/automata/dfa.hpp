#pragma once

#include <stdexcept>
#include <string_view>
#include <vector>

#include "automata/types.hpp"

namespace funny::automata {

struct DfaError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Dfa {
private:
    std::vector<Rule> rules_;
    std::vector<RuleId> accept_;
    std::vector<StateId> trans_;
    StateId start_ = kNoState;
    StateId trap_ = kNoState;
public:
    RuleId add_rule(std::string name, bool skip = false);
    StateId add_state(RuleId accept = kNoRule);
    void set_transition(StateId from, unsigned char symbol, StateId to);
    void set_accept(StateId state, RuleId rule);
    void set_start(StateId state) { start_ = state; }
    void set_trap(StateId state) { trap_ = state; }

    std::size_t state_count() const { return accept_.size(); }
    StateId start() const { return start_; }
    StateId trap() const { return trap_; }
    const std::vector<Rule>& rules() const { return rules_; }
    RuleId accept(StateId state) const { return accept_.at(state); }
    StateId next(StateId state, unsigned char symbol) const {
        return trans_[static_cast<std::size_t>(state) * kAlphabetSize + symbol];
    }

    RuleId match_full(std::string_view input) const;

    void validate() const;
};
} // namespace funny::automata