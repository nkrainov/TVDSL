#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace funny::token {

using Kind = std::int32_t;

struct Token {
    Kind kind;
    std::string_view lexeme;
    std::size_t offset;
    std::size_t line;
    std::size_t column;
};

}  // namespace funny::token