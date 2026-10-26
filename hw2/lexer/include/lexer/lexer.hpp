#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "automata/dfa.hpp"
#include "token/token.hpp"

namespace funny::lexer {

struct LexError {
    std::size_t offset;
    std::size_t line;
    std::size_t column;
    unsigned char byte;
    std::string message;
};

struct LexResult {
    std::vector<token::Token> tokens;
    std::optional<LexError> error;

    bool ok() const { return !error.has_value(); }
};

class Lexer {
public:
    explicit Lexer(automata::Dfa dfa);

    static Lexer from_text(std::string_view serialized_dfa);

    LexResult tokenize(std::string_view input) const;

    const automata::Dfa& dfa() const { return dfa_; }

private:
    automata::Dfa dfa_;
};

}