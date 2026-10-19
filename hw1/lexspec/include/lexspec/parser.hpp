#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "lexspec/ast.hpp"

namespace funny::lexspec {

// Ошибка разбора. Строка и столбец нумеруются с 1 (столбец считается в байтах).
class RegexError : public std::runtime_error {
public:
    RegexError(std::size_t line, std::size_t column, std::string message);

    std::size_t line() const { return line_; }
    std::size_t column() const { return column_; }
    const std::string& message() const { return message_; }

private:
    std::size_t line_;
    std::size_t column_;
    std::string message_;
};

// Описание одного токена из файла. Номер в векторе = номер правила = приоритет.
struct TokenSpec {
    std::string name;
    bool skip = false;
    NodePtr regex;
};

// Разбор одного регулярного выражения (синтаксис см. README формата).
NodePtr parse_regex(std::string_view text);

// Разбор файла токенов: строки вида `NAME [skip] : regex`, `#` комментарии.
// Проверяет: уникальность имён, что правило не принимает пустую строку, что правила есть.
std::vector<TokenSpec> parse_token_file(std::string_view text);

}  // namespace funny::lexspec