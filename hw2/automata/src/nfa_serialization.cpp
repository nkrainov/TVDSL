#include "automata/nfa_serialization.hpp"

#include <charconv>
#include <sstream>

#include "reader.hpp"

namespace funny::automata {

namespace {

std::string label_of(const SymbolSet& set) {
    std::string out = "[";
    bool first = true;
    std::size_t i = 0;
    while (i < kAlphabetSize) {
        if (!set[i]) { ++i; continue; }
        std::size_t j = i;
        while (j + 1 < kAlphabetSize && set[j + 1]) ++j;
        if (!first) out += ',';
        first = false;
        out += std::to_string(i);
        if (j > i) out += "-" + std::to_string(j);
        i = j + 1;
    }
    return out + "]";
}

std::size_t parse_byte(std::string_view s) {
    std::size_t v = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc() || ptr != s.data() + s.size() || v >= kAlphabetSize)
        throw NfaError("bad byte value in label: '" + std::string(s) + "'");
    return v;
}

SymbolSet parse_label(const std::string& word) {
    if (word.size() < 3 || word.front() != '[' || word.back() != ']')
        throw NfaError("bad edge label: '" + word + "'");
    SymbolSet set;
    std::string_view body = std::string_view(word).substr(1, word.size() - 2);
    while (!body.empty()) {
        std::size_t comma = body.find(',');
        std::string_view item = body.substr(0, comma);
        body = comma == std::string_view::npos ? std::string_view() : body.substr(comma + 1);
        std::size_t dash = item.find('-');
        std::size_t lo = parse_byte(item.substr(0, dash));
        std::size_t hi = dash == std::string_view::npos ? lo : parse_byte(item.substr(dash + 1));
        if (hi < lo) throw NfaError("bad range in label: '" + word + "'");
        for (std::size_t c = lo; c <= hi; ++c) set.set(c);
    }
    if (set.none()) throw NfaError("empty label: '" + word + "'");
    return set;
}

}  // namespace

std::string serialize(const Nfa& nfa) {
    const std::size_t n = nfa.state_count();
    std::ostringstream out;
    out << "nfa\n";
    out << "states " << n << "\n";
    out << "start " << nfa.start() << "\n";
    out << "rules " << nfa.rules().size() << "\n";
    for (std::size_t i = 0; i < nfa.rules().size(); ++i)
        out << "rule " << i << ' ' << nfa.rules()[i].name << " skip=" << (nfa.rules()[i].skip ? 1 : 0) << "\n";

    std::size_t accepts = 0, edges = 0;
    for (std::size_t s = 0; s < n; ++s) {
        if (nfa.accept(static_cast<StateId>(s)) != kNoRule) ++accepts;
        edges += nfa.epsilon(static_cast<StateId>(s)).size() + nfa.edges(static_cast<StateId>(s)).size();
    }
    out << "accepts " << accepts << "\n";
    for (std::size_t s = 0; s < n; ++s)
        if (nfa.accept(static_cast<StateId>(s)) != kNoRule)
            out << "accept " << s << ' ' << nfa.accept(static_cast<StateId>(s)) << "\n";

    out << "edges " << edges << "\n";
    for (std::size_t s = 0; s < n; ++s) {
        const auto id = static_cast<StateId>(s);
        for (StateId t : nfa.epsilon(id)) out << "edge " << s << " eps " << t << "\n";
        for (const NfaEdge& e : nfa.edges(id)) out << "edge " << s << ' ' << label_of(e.symbols) << ' ' << e.to << "\n";
    }
    return out.str();
}

Nfa deserialize_nfa(std::string_view text) {
    detail::Reader<NfaError> r(text);
    r.expect("nfa");
    const long long n = r.keyed("states");
    if (n <= 0 || n >= static_cast<long long>(kNoState)) throw NfaError("bad state count");
    const long long start = r.keyed("start");
    if (start < 0 || start >= n) throw NfaError("start out of range");
    const long long rule_count = r.keyed("rules");
    if (rule_count < 0) throw NfaError("bad rule count");

    Nfa nfa;
    for (long long i = 0; i < rule_count; ++i) {
        r.expect("rule");
        if (r.integer("rule id") != i) throw NfaError("rules must be numbered 0..N-1 in order");
        std::string name = r.word("rule name");
        std::string skip = r.word("skip flag");
        if (skip != "skip=0" && skip != "skip=1") throw NfaError("bad skip flag: " + skip);
        nfa.add_rule(std::move(name), skip == "skip=1");
    }
    for (long long s = 0; s < n; ++s) nfa.add_state();
    nfa.set_start(static_cast<StateId>(start));

    const long long accepts = r.keyed("accepts");
    for (long long i = 0; i < accepts; ++i) {
        r.expect("accept");
        long long s = r.integer("accept state"), rule = r.integer("accept rule");
        if (s < 0 || s >= n) throw NfaError("accept state out of range");
        if (rule < 0 || rule >= rule_count) throw NfaError("accept rule out of range");
        nfa.set_accept(static_cast<StateId>(s), static_cast<RuleId>(rule));
    }

    const long long edges = r.keyed("edges");
    for (long long i = 0; i < edges; ++i) {
        r.expect("edge");
        long long from = r.integer("edge source");
        std::string label = r.word("edge label");
        long long to = r.integer("edge target");
        if (from < 0 || from >= n || to < 0 || to >= n) throw NfaError("edge endpoint out of range");
        if (label == "eps") nfa.add_epsilon(static_cast<StateId>(from), static_cast<StateId>(to));
        else nfa.add_edge(static_cast<StateId>(from), parse_label(label), static_cast<StateId>(to));
    }
    if (!r.at_end()) throw NfaError("trailing data after edges");
    return nfa;
}

}  // namespace funny::automata