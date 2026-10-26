#pragma once

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "ast/ast.hpp"

namespace funny::parser_tests::b {

using namespace funny::ast;

inline constexpr Type Int = Type::Int;
inline constexpr Type IntArray = Type::IntArray;

inline ExprPtr num(std::int64_t v) { return make_expr({}, IntLit{v}); }
inline ExprPtr boolean(bool v) { return make_expr({}, BoolLit{v}); }
inline ExprPtr var(std::string name) { return make_expr({}, Var{std::move(name)}); }
inline ExprPtr idx(ExprPtr base, ExprPtr index) { return make_expr({}, Index{std::move(base), std::move(index)}); }
inline ExprPtr err_expr() { return make_expr({}, ErrorExpr{}); }

template <class... Es>
ExprPtr call(std::string name, Es... args) {
    Call c;
    c.name = std::move(name);
    (c.args.push_back(std::move(args)), ...);
    return make_expr({}, std::move(c));
}

inline ExprPtr neg(ExprPtr e) { return make_expr({}, Unary{UnaryOp::Neg, std::move(e)}); }
inline ExprPtr not_(ExprPtr e) { return make_expr({}, Unary{UnaryOp::Not, std::move(e)}); }

inline ExprPtr binary(BinaryOp op, ExprPtr l, ExprPtr r) { return make_expr({}, Binary{op, std::move(l), std::move(r)}); }
inline ExprPtr add(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Add, std::move(l), std::move(r)); }
inline ExprPtr sub(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Sub, std::move(l), std::move(r)); }
inline ExprPtr mul(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Mul, std::move(l), std::move(r)); }
inline ExprPtr divide(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Div, std::move(l), std::move(r)); }
inline ExprPtr eq(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Eq, std::move(l), std::move(r)); }
inline ExprPtr ne(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Ne, std::move(l), std::move(r)); }
inline ExprPtr lt(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Lt, std::move(l), std::move(r)); }
inline ExprPtr le(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Le, std::move(l), std::move(r)); }
inline ExprPtr gt(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Gt, std::move(l), std::move(r)); }
inline ExprPtr ge(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Ge, std::move(l), std::move(r)); }
inline ExprPtr and_(ExprPtr l, ExprPtr r) { return binary(BinaryOp::And, std::move(l), std::move(r)); }
inline ExprPtr or_(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Or, std::move(l), std::move(r)); }
inline ExprPtr implies(ExprPtr l, ExprPtr r) { return binary(BinaryOp::Implies, std::move(l), std::move(r)); }

inline Param param(std::string name, Type type) {
    Param p;
    p.name = std::move(name);
    p.type = type;
    return p;
}

inline ExprPtr forall(std::string name, Type type, ExprPtr body) {
    return make_expr({}, Quantifier{QuantifierKind::Forall, param(std::move(name), type), std::move(body)});
}
inline ExprPtr exists(std::string name, Type type, ExprPtr body) {
    return make_expr({}, Quantifier{QuantifierKind::Exists, param(std::move(name), type), std::move(body)});
}

inline StmtPtr assign(std::string name, ExprPtr value) {
    LValue lv;
    lv.name = std::move(name);
    return make_stmt({}, Assign{std::move(lv), std::move(value)});
}

template <class... Is>
StmtPtr assign_at(std::string name, ExprPtr value, Is... indices) {
    LValue lv;
    lv.name = std::move(name);
    (lv.indices.push_back(std::move(indices)), ...);
    return make_stmt({}, Assign{std::move(lv), std::move(value)});
}

template <class... Es>
StmtPtr call_assign(const std::vector<std::string>& targets, std::string callee, Es... args) {
    CallAssign ca;
    for (const auto& t : targets) ca.targets.push_back(Target{{}, t});
    ca.call.name = std::move(callee);
    (ca.call.args.push_back(std::move(args)), ...);
    return make_stmt({}, std::move(ca));
}

inline StmtPtr if_(ExprPtr cond, StmtPtr then_branch, StmtPtr else_branch = nullptr) {
    return make_stmt({}, If{std::move(cond), std::move(then_branch), std::move(else_branch)});
}
inline StmtPtr while_(ExprPtr cond, ExprPtr invariant, StmtPtr body) {
    return make_stmt({}, While{std::move(cond), std::move(invariant), std::move(body)});
}
template <class... Ss>
StmtPtr block(Ss... stmts) {
    Block bl;
    (bl.stmts.push_back(std::move(stmts)), ...);
    return make_stmt({}, std::move(bl));
}
inline StmtPtr assert_(ExprPtr e) { return make_stmt({}, Assert{std::move(e)}); }
inline StmtPtr assume_(ExprPtr e) { return make_stmt({}, Assume{std::move(e)}); }
inline StmtPtr err_stmt() { return make_stmt({}, ErrorStmt{}); }

inline LocalVar local(std::string name) {
    LocalVar v;
    v.name = std::move(name);
    return v;
}
inline LocalVar local(std::string name, Type type) {
    LocalVar v = local(std::move(name));
    v.type = type;
    return v;
}

class Fn {
public:
    Fn(std::string name, std::vector<Param> params, std::vector<Param> results) {
        f_.name = std::move(name);
        f_.params = std::move(params);
        f_.results = std::move(results);
    }
    Fn&& requires_(ExprPtr e) && { f_.requires_clause = std::move(e); return std::move(*this); }
    Fn&& ensures(ExprPtr e) && { f_.ensures_clause = std::move(e); return std::move(*this); }
    Fn&& uses(std::vector<LocalVar> locals) && { f_.locals = std::move(locals); return std::move(*this); }
    Fn&& body(StmtPtr s) && { f_.body = std::move(s); return std::move(*this); }
    Function build() && { return std::move(f_); }

private:
    Function f_;
};

inline Fn fn(std::string name, std::vector<Param> params, std::vector<Param> results) {
    return Fn(std::move(name), std::move(params), std::move(results));
}

inline Formula formula(std::string name, std::vector<Param> params, ExprPtr body) {
    Formula f;
    f.name = std::move(name);
    f.params = std::move(params);
    f.body = std::move(body);
    return f;
}

inline Definition to_definition(Fn f) { return std::move(f).build(); }
inline Definition to_definition(Formula f) { return f; }

template <class... Ds>
Module mod(Ds... defs) {
    Module m;
    (m.definitions.push_back(to_definition(std::move(defs))), ...);
    return m;
}

}  // namespace funny::parser_tests::b