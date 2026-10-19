#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "automata/dfa.hpp"

namespace funny::lexer {

using RuleId = automata::RuleId;

struct Token {
    RuleId rule;
    std::string_view lexeme;
    std::size_t offset;
    std::size_t line;
    std::size_t column;
};

struct LexError {
    std::size_t offset;
    std::size_t line;
    std::size_t column;
    unsigned char byte;  // непринятый байт
    std::string message;
};

struct LexResult {
    std::vector<Token> tokens;
    std::optional<LexError> error;

    bool ok() const { return !error.has_value(); }
};

class Lexer {
public:
    Lexer(automata::Dfa dfa);

    static Lexer from_text(std::string_view serialized_dfa);

    LexResult tokenize(std::string_view input) const;

    const std::string& rule_name(RuleId rule) const { return dfa_.rules().at(static_cast<std::size_t>(rule)).name; }
    const automata::Dfa& dfa() const { return dfa_; }

private:
    automata::Dfa dfa_;
};

}  // namespace funny::lexer