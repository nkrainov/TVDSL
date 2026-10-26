#pragma once

#include <string>

#include "ast/ast.hpp"

namespace funny::ast {

struct PrintOptions {
    bool positions = false;
};

std::string to_string(const Expr& expr, PrintOptions options = {});
std::string to_string(const Stmt& stmt, PrintOptions options = {});
std::string to_string(const Module& module, PrintOptions options = {});

}  // namespace funny::ast