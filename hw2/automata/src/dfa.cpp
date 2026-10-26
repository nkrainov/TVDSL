#include "automata/dfa.hpp"

#include <cctype>

namespace funny::automata {

RuleId Dfa::add_rule(std::string name, bool skip) {
    rules_.push_back(Rule{std::move(name), skip});
    return static_cast<RuleId>(rules_.size() - 1);
}

StateId Dfa::add_state(RuleId accept) {
    accept_.push_back(accept);
    trans_.resize(trans_.size() + kAlphabetSize, kNoState);
    return static_cast<StateId>(accept_.size() - 1);
}

void Dfa::set_transition(StateId from, unsigned char symbol, StateId to) {
    if (from >= state_count()) throw DfaError("set_transition: bad source state");
    trans_[static_cast<std::size_t>(from) * kAlphabetSize + symbol] = to;
}

void Dfa::set_accept(StateId state, RuleId rule) {
    accept_.at(state) = rule;
}

RuleId Dfa::match_full(std::string_view input) const {
    StateId s = start_;
    for (char ch : input) {
        s = next(s, static_cast<unsigned char>(ch));
        if (s == trap_) return kNoRule;
    }
    return accept_[s];
}

// проверяется количество состояний, допустимость номера состояний ловушки и старта, проверка что ловушка не принимает состояния
// проверяется корректность переходов
void Dfa::validate() const {
    const std::size_t n = state_count();
    if (n == 0) throw DfaError("no states");
    if (start_ >= n) throw DfaError("start state out of range");
    if (trap_ >= n) throw DfaError("trap state out of range");
    if (accept_[trap_] != kNoRule) throw DfaError("trap state must not be accepting");

    for (std::size_t i = 0; i < rules_.size(); ++i) {
        const std::string& name = rules_[i].name;
        if (name.empty()) throw DfaError("rule " + std::to_string(i) + ": empty name");
        for (unsigned char c : name)
            if (std::isspace(c))
                throw DfaError("rule " + std::to_string(i) + ": name contains whitespace");
    }
    for (std::size_t s = 0; s < n; ++s) {
        RuleId r = accept_[s];
        if (r != kNoRule && (r < 0 || static_cast<std::size_t>(r) >= rules_.size()))
            throw DfaError("state " + std::to_string(s) + ": accept rule out of range");
        for (std::size_t c = 0; c < kAlphabetSize; ++c) {
            StateId t = trans_[s * kAlphabetSize + c];
            if (t >= n)
                throw DfaError("state " + std::to_string(s) + ", byte " + std::to_string(c) +
                               ": missing or invalid transition");
        }
    }
    for (std::size_t c = 0; c < kAlphabetSize; ++c)
        if (trans_[static_cast<std::size_t>(trap_) * kAlphabetSize + c] != trap_)
            throw DfaError("trap state must loop to itself");
}

}  // namespace funny::automata