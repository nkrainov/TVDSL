#include "automata/dfa_serialization.hpp"

#include <sstream>

#include "reader.hpp"

namespace funny::automata {


std::string serialize(const Dfa& dfa) {
    dfa.validate();
    const std::size_t n = dfa.state_count();
    std::ostringstream out;
    out << "dfa\n";
    out << "states " << n << "\n";
    out << "start " << dfa.start() << "\n";
    out << "trap " << dfa.trap() << "\n";
    out << "rules " << dfa.rules().size() << "\n";
    for (std::size_t i = 0; i < dfa.rules().size(); ++i)
        out << "rule " << i << ' ' << dfa.rules()[i].name
            << " skip=" << (dfa.rules()[i].skip ? 1 : 0) << "\n";
    out << "accept";
    for (std::size_t s = 0; s < n; ++s) out << ' ' << dfa.accept(static_cast<StateId>(s));
    out << "\ntrans\n";
    for (std::size_t s = 0; s < n; ++s) {
        for (std::size_t c = 0; c < kAlphabetSize; ++c) {
            if (c) out << ' ';
            out << dfa.next(static_cast<StateId>(s), static_cast<unsigned char>(c));
        }
        out << "\n";
    }
    return out.str();
}

Dfa deserialize(std::string_view text) {
    detail::Reader<DfaError> r(text);
    r.expect("dfa");
    const long long n = r.keyed("states");
    if (n <= 0 || n >= static_cast<long long>(kNoState)) throw DfaError("bad state count");
    const long long start = r.keyed("start");
    const long long trap = r.keyed("trap");
    const long long rule_count = r.keyed("rules");
    if (rule_count < 0) throw DfaError("bad rule count");

    Dfa dfa;
    for (long long i = 0; i < rule_count; ++i) {
        r.expect("rule");
        if (r.integer("rule id") != i) throw DfaError("rules must be numbered 0..N-1 in order");
        std::string name = r.word("rule name");
        std::string skip = r.word("skip flag");
        if (skip != "skip=0" && skip != "skip=1") throw DfaError("bad skip flag: " + skip);
        dfa.add_rule(std::move(name), skip == "skip=1");
    }

    r.expect("accept");
    for (long long s = 0; s < n; ++s) {
        long long a = r.integer("accept");
        if (a < -1 || a >= rule_count) throw DfaError("accept rule out of range");
        dfa.add_state(static_cast<RuleId>(a));
    }

    r.expect("trans");
    for (long long s = 0; s < n; ++s)
        for (std::size_t c = 0; c < kAlphabetSize; ++c) {
            long long t = r.integer("transition");
            if (t < 0 || t >= n) throw DfaError("transition target out of range");
            dfa.set_transition(static_cast<StateId>(s), static_cast<unsigned char>(c),
                               static_cast<StateId>(t));
        }
    if (!r.at_end()) throw DfaError("trailing data after table");

    if (start < 0 || start >= n) throw DfaError("start out of range");
    if (trap < 0 || trap >= n) throw DfaError("trap out of range");
    dfa.set_start(static_cast<StateId>(start));
    dfa.set_trap(static_cast<StateId>(trap));
    dfa.validate();
    return dfa;
}

}  // namespace funny::automata