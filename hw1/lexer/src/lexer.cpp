#include "lexer/lexer.hpp"

#include <cstdio>
#include <stdexcept>

#include "automata/dfa_serialization.hpp"

namespace funny::lexer {

namespace {

std::string describe_byte(unsigned char c) {
    char buf[64];
    if (c >= 128) std::snprintf(buf, sizeof buf, "non-ASCII byte 0x%02X", c);
    else if (c > 32 && c < 127) std::snprintf(buf, sizeof buf, "unexpected character '%c'", c);
    else std::snprintf(buf, sizeof buf, "unexpected byte 0x%02X", c);
    return buf;
}

}  // namespace

Lexer::Lexer(automata::Dfa dfa) : dfa_(std::move(dfa)) {
    dfa_.validate();
    if (dfa_.accept(dfa_.start()) != automata::kNoRule)
        throw std::invalid_argument("lexer: DFA start state is accepting (a rule accepts the empty string)");
}

Lexer Lexer::from_text(std::string_view serialized_dfa) {
    return Lexer(automata::deserialize(serialized_dfa));
}

LexResult Lexer::tokenize(std::string_view input) const {
    LexResult result;
    const std::size_t n = input.size();
    const automata::StateId trap = dfa_.trap();

    std::size_t pos = 0, line = 1, column = 1;
    while (pos < n) {
        automata::StateId state = dfa_.start();
        RuleId last_rule = automata::kNoRule;
        std::size_t last_end = pos;

        for (std::size_t p = pos; p < n;) {
            state = dfa_.next(state, static_cast<unsigned char>(input[p]));
            if (state == trap) break;
            ++p;
            const RuleId r = dfa_.accept(state);
            if (r != automata::kNoRule) {
                last_rule = r;
                last_end = p;
            }
        }

        if (last_rule == automata::kNoRule) {
            const auto byte = static_cast<unsigned char>(input[pos]);
            result.error = LexError{pos, line, column, byte, describe_byte(byte)};
            return result;
        }

        const std::string_view lexeme = input.substr(pos, last_end - pos);
        if (!dfa_.rules()[static_cast<std::size_t>(last_rule)].skip)
            result.tokens.push_back(Token{last_rule, lexeme, pos, line, column});

        for (char ch : lexeme) {
            if (ch == '\n') { ++line; column = 1; }
            else ++column;
        }
        pos = last_end;
    }
    return result;
}

}  // namespace funny::lexer