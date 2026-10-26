#include "automata/nfa.hpp"

#include <stdexcept>

namespace funny::automata {

void Nfa::check_state(StateId s) const {
    if (s >= state_count()) throw std::out_of_range("Nfa: state id out of range");
}

RuleId Nfa::add_rule(std::string name, bool skip) {
    rules_.push_back(Rule{std::move(name), skip});
    return static_cast<RuleId>(rules_.size() - 1);
}

StateId Nfa::add_state() {
    accept_.push_back(kNoRule);
    eps_.emplace_back();
    edges_.emplace_back();
    return static_cast<StateId>(accept_.size() - 1);
}

void Nfa::add_epsilon(StateId from, StateId to) {
    check_state(from);
    check_state(to);
    eps_[from].push_back(to);
}

void Nfa::add_edge(StateId from, const SymbolSet& symbols, StateId to) {
    check_state(from);
    check_state(to);
    edges_[from].push_back(NfaEdge{symbols, to});
}

void Nfa::set_accept(StateId state, RuleId rule) {
    check_state(state);
    if (rule != kNoRule && (rule < 0 || static_cast<std::size_t>(rule) >= rules_.size())) {
        throw std::out_of_range("Nfa: rule id out of range");
    }
    accept_[state] = rule;
}

void Nfa::set_start(StateId state) {
    check_state(state);
    start_ = state;
}

std::vector<StateId> Nfa::epsilon_closure(const std::vector<StateId>& seeds) const {
    std::vector<bool> seen(state_count(), 0);
    std::vector<StateId> stack;
    for (StateId s : seeds) {
        check_state(s);
        if (!seen[s]) { 
            seen[s] = true; 
            stack.push_back(s); 
        }
    }
    while (!stack.empty()) {
        StateId s = stack.back();
        stack.pop_back();
        for (StateId t : eps_[s]){
            if (!seen[t]) { 
                seen[t] = true; 
                stack.push_back(t); 
            }
        }
    }
    std::vector<StateId> result;
    for (std::size_t i = 0; i < seen.size(); ++i) {
        if (seen[i]) {
            result.push_back(static_cast<StateId>(i));
        }
    }
    return result;
}

RuleId Nfa::match_full(std::string_view input) const {
    check_state(start_);
    std::vector<StateId> current = epsilon_closure({start_});
    for (char ch : input) {
        const unsigned char c = static_cast<unsigned char>(ch);
        std::vector<StateId> moved;
        for (StateId s : current) {
            for (const NfaEdge& e : edges_[s]) {
                if (e.symbols[c]) {
                    moved.push_back(e.to);
                }
            }
        }
        if (moved.empty()) {
            return kNoRule;
        }
        current = epsilon_closure(moved);
    }
    RuleId res = kNoRule;
    for (StateId s : current) {
        RuleId r = accept_[s];
        if (r != kNoRule && (res == kNoRule || r < res)) {
            res = r;
        }
    }
    return res;
}

}  // namespace funny::automata