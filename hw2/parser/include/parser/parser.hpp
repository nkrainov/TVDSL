#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "ast/ast.hpp"
#include "token/token.hpp"
#include "token/tokentable.hpp"

namespace funny::parser {

struct Diagnostic {
    std::size_t line = 0;
    std::size_t column = 0;
    std::string message;
};

std::string to_string(const Diagnostic& d);

struct ParseResult {
    std::unique_ptr<ast::Module> module;
    std::vector<Diagnostic> errors;

    bool ok() const { return errors.empty(); }
};

struct Kinds;

class Parser {
public:
    explicit Parser(const token::TokenTable& table);

    ParseResult parse(std::span<const token::Token> tokens) const;

private:
    std::shared_ptr<const Kinds> kinds_;
};

}  // namespace funny::parser