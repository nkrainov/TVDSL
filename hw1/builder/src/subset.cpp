#include "builder/subset.hpp"

#include <cstdio>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace funny::builder {

namespace {

using automata::Dfa;
using automata::kAlphabetSize;
using automata::kNoRule;
using automata::kNoState;
using automata::Nfa;
using automata::RuleId;
using automata::StateId;

using Subset = std::vector<StateId>; 

struct Origin {
    StateId parent = kNoState;  
    unsigned char byte = 0;
};

std::string escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c > 32 && c < 127 && c != '"' && c != '\\') {
            out += static_cast<char>(c);
        } else {
            char buf[8];
            std::snprintf(buf, sizeof buf, "\\x%02x", c);
            out += buf;
        }
    }
    return out;
}

RuleId pick_rule(const Nfa& nfa, const Subset& subset, std::set<RuleId>& all_rules) {
    all_rules.clear();
    for (StateId s : subset) {
        RuleId r = nfa.accept(s);
        if (r != kNoRule) all_rules.insert(r);
    }
    return all_rules.empty() ? kNoRule : *all_rules.begin();
}

}  // namespace

DeterminizeResult determinize(const Nfa& nfa) {
    if (nfa.start() == kNoState) throw std::invalid_argument("determinize: NFA has no start state");

    DeterminizeResult result;
    Dfa& dfa = result.dfa;
    for (const auto& r : nfa.rules()) dfa.add_rule(r.name, r.skip);

    const StateId trap = dfa.add_state();
    dfa.set_trap(trap);
    for (std::size_t c = 0; c < kAlphabetSize; ++c) dfa.set_transition(trap, static_cast<unsigned char>(c), trap);

    std::vector<Subset> subsets{Subset{}};
    std::vector<Origin> origins{Origin{}};
    std::map<Subset, StateId> ids;
    std::set<RuleId> reached_rules;
    std::set<std::pair<RuleId, RuleId>> reported;

    auto path_to = [&](StateId s) {
        std::string path;
        while (origins[s].parent != kNoState) {
            path.insert(path.begin(), static_cast<char>(origins[s].byte));
            s = origins[s].parent;
        }
        return path;
    };

    auto add_subset = [&](const Subset& subset, StateId from, unsigned char byte) {
        std::set<RuleId> rules;
        RuleId winner = pick_rule(nfa, subset, rules);
        StateId id = dfa.add_state(winner);
        subsets.push_back(subset);
        origins.push_back(Origin{from, byte});
        ids.emplace(subset, id);

        if (winner != kNoRule) {
            reached_rules.insert(winner);
            for (RuleId loser : rules) {
                if (loser == winner || !reported.insert({winner, loser}).second) continue;
                result.warnings.push_back(Diagnostic{
                    Diagnostic::Kind::Override,
                    nfa.rules()[static_cast<std::size_t>(winner)].name + " overrides " +
                        nfa.rules()[static_cast<std::size_t>(loser)].name + " on \"" + escape(path_to(id)) + "\""});
            }
        }
        return id;
    };

    const Subset start_set = nfa.epsilon_closure({nfa.start()});
    {
        std::set<RuleId> rules;
        RuleId r = pick_rule(nfa, start_set, rules);
        if (r != kNoRule)
            throw std::invalid_argument("determinize: rule '" + nfa.rules()[static_cast<std::size_t>(r)].name +
                                        "' accepts the empty string");
    }
    dfa.set_start(add_subset(start_set, kNoState, 0));

    for (std::size_t i = 1; i < subsets.size(); ++i) {
        const Subset current = subsets[i];
        for (std::size_t c = 0; c < kAlphabetSize; ++c) {
            const auto byte = static_cast<unsigned char>(c);
            Subset moved;
            for (StateId s : current)
                for (const auto& edge : nfa.edges(s))
                    if (edge.symbols[byte]) moved.push_back(edge.to);

            StateId target = trap;
            if (!moved.empty()) {
                Subset closure = nfa.epsilon_closure(moved);
                auto it = ids.find(closure);
                target = it != ids.end() ? it->second : add_subset(closure, static_cast<StateId>(i), byte);
            }
            dfa.set_transition(static_cast<StateId>(i), byte, target);
        }
    }

    for (std::size_t r = 0; r < nfa.rules().size(); ++r)
        if (!reached_rules.count(static_cast<RuleId>(r)))
            result.warnings.push_back(Diagnostic{
                Diagnostic::Kind::NeverAccepted,
                "rule " + nfa.rules()[r].name + " is never accepted (shadowed by higher-priority rules)"});

    dfa.validate();
    return result;
}

}  // namespace funny::builder