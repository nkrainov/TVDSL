#pragma once

#include "ast/ast.hpp"

namespace funny::ast {

struct EqualOptions {
    bool positions = false;
};

bool equal(const Expr& a, const Expr& b, EqualOptions options = {});
bool equal(const Stmt& a, const Stmt& b, EqualOptions options = {});
bool equal(const Module& a, const Module& b, EqualOptions options = {});

}  // namespace funny::ast