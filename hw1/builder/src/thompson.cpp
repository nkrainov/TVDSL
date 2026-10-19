#include "builder/thompson.hpp"

#include <stdexcept>

namespace funny::builder {

namespace {

using automata::Nfa;
using automata::StateId;
using automata::SymbolSet;
using lexspec::Node;

struct Fragment {
    StateId start;
    StateId end;
};

SymbolSet to_symbol_set(const lexspec::CharSet& cs) {
    SymbolSet out; 
    for (std::size_t i = 0; i < lexspec::kAsciiSize; ++i)
        if (cs[i]) out.set(i);
    return out;
}

class Thompson {
public:
    explicit Thompson(Nfa& nfa) : nfa_(nfa) {}

    Fragment build(const Node& node) {
        switch (node.kind) {
            case Node::Kind::Set: return build_set(node);
            case Node::Kind::Concat: return build_concat(node);
            case Node::Kind::Alt: return build_alt(node);
            case Node::Kind::Star: return build_star(node);
            case Node::Kind::Plus: return build_plus(node);
            case Node::Kind::Optional: return build_optional(node);
        }
        throw std::logic_error("unknown regex node kind");
    }

private:
    Nfa& nfa_;

    Fragment build_set(const Node& node) {
        StateId s = nfa_.add_state();
        StateId e = nfa_.add_state();
        nfa_.add_edge(s, to_symbol_set(node.set), e);
        return {s, e};
    }

    Fragment build_concat(const Node& node) {
        Fragment first = build(*node.children.front());
        Fragment prev = first;
        for (std::size_t i = 1; i < node.children.size(); ++i) {
            Fragment cur = build(*node.children[i]);
            nfa_.add_epsilon(prev.end, cur.start);
            prev = cur;
        }
        return {first.start, prev.end};
    }

    Fragment build_alt(const Node& node) {
        StateId s = nfa_.add_state();
        StateId e = nfa_.add_state();
        for (const auto& child : node.children) {
            Fragment f = build(*child);
            nfa_.add_epsilon(s, f.start);
            nfa_.add_epsilon(f.end, e);
        }
        return {s, e};
    }

    Fragment build_star(const Node& node) {
        StateId s = nfa_.add_state();
        Fragment f = build(*node.children[0]);
        StateId e = nfa_.add_state();
        nfa_.add_epsilon(s, f.start);
        nfa_.add_epsilon(s, e);
        nfa_.add_epsilon(f.end, f.start);
        nfa_.add_epsilon(f.end, e);
        return {s, e};
    }

    Fragment build_plus(const Node& node) {
        StateId s = nfa_.add_state();
        Fragment f = build(*node.children[0]);
        StateId e = nfa_.add_state();
        nfa_.add_epsilon(s, f.start);
        nfa_.add_epsilon(f.end, f.start);
        nfa_.add_epsilon(f.end, e);
        return {s, e};
    }

    Fragment build_optional(const Node& node) {
        StateId s = nfa_.add_state();
        Fragment f = build(*node.children[0]);
        StateId e = nfa_.add_state();
        nfa_.add_epsilon(s, f.start);
        nfa_.add_epsilon(s, e);
        nfa_.add_epsilon(f.end, e);
        return {s, e};
    }
};

}  // namespace

automata::Nfa build_nfa(const std::vector<lexspec::TokenSpec>& specs) {
    if (specs.empty()) throw std::invalid_argument("build_nfa: no rules");

    Nfa nfa;
    const StateId start = nfa.add_state();
    nfa.set_start(start);

    Thompson thompson(nfa);
    for (const auto& spec : specs) {
        if (!spec.regex) throw std::invalid_argument("build_nfa: rule '" + spec.name + "' has no expression");
        const automata::RuleId rule = nfa.add_rule(spec.name, spec.skip);
        const Fragment f = thompson.build(*spec.regex);
        nfa.add_epsilon(start, f.start);
        nfa.set_accept(f.end, rule);
    }
    return nfa;
}

automata::Nfa build_nfa(const lexspec::Node& regex, const std::string& rule_name) {
    Nfa nfa;
    const StateId start = nfa.add_state();
    nfa.set_start(start);
    const automata::RuleId rule = nfa.add_rule(rule_name);
    Thompson thompson(nfa);
    const Fragment f = thompson.build(regex);
    nfa.add_epsilon(start, f.start);
    nfa.set_accept(f.end, rule);
    return nfa;
}

}  // namespace funny::builder