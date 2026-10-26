#include "ast/equal.hpp"

#include <type_traits>

namespace funny::ast {

namespace {

struct Cmp {
    bool positions;

    bool same(Position a, Position b) const { return !positions || (a.line == b.line && a.column == b.column); }

    bool ptr(const ExprPtr& a, const ExprPtr& b) const { return (!a || !b) ? (!a && !b) : expr(*a, *b); }
    bool ptr(const StmtPtr& a, const StmtPtr& b) const { return (!a || !b) ? (!a && !b) : stmt(*a, *b); }

    template <class T>
    bool list(const std::vector<T>& a, const std::vector<T>& b) const {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i)
            if (!item(a[i], b[i])) return false;
        return true;
    }

    bool item(const ExprPtr& a, const ExprPtr& b) const { return ptr(a, b); }
    bool item(const StmtPtr& a, const StmtPtr& b) const { return ptr(a, b); }
    bool item(const Param& a, const Param& b) const { return same(a.pos, b.pos) && a.name == b.name && a.type == b.type; }
    bool item(const LocalVar& a, const LocalVar& b) const { return same(a.pos, b.pos) && a.name == b.name && a.type == b.type; }
    bool item(const Target& a, const Target& b) const { return same(a.pos, b.pos) && a.name == b.name; }
    bool item(const LValue& a, const LValue& b) const {
        return same(a.pos, b.pos) && a.name == b.name && list(a.indices, b.indices);
    }

    bool expr(const Expr& a, const Expr& b) const {
        if (!same(a.pos, b.pos)) return false;
        return std::visit(
            [this](const auto& x, const auto& y) -> bool {
                if constexpr (std::is_same_v<std::decay_t<decltype(x)>, std::decay_t<decltype(y)>>) return node(x, y);
                else return false;
            },
            a.node, b.node);
    }
    bool node(const IntLit& a, const IntLit& b) const { return a.value == b.value; }
    bool node(const BoolLit& a, const BoolLit& b) const { return a.value == b.value; }
    bool node(const Var& a, const Var& b) const { return a.name == b.name; }
    bool node(const Index& a, const Index& b) const { return ptr(a.base, b.base) && ptr(a.index, b.index); }
    bool node(const Call& a, const Call& b) const { return a.name == b.name && list(a.args, b.args); }
    bool node(const Unary& a, const Unary& b) const { return a.op == b.op && ptr(a.operand, b.operand); }
    bool node(const Binary& a, const Binary& b) const {
        return a.op == b.op && ptr(a.lhs, b.lhs) && ptr(a.rhs, b.rhs);
    }
    bool node(const Quantifier& a, const Quantifier& b) const {
        return a.kind == b.kind && item(a.var, b.var) && ptr(a.body, b.body);
    }
    bool node(const ErrorExpr&, const ErrorExpr&) const { return true; }

    bool stmt(const Stmt& a, const Stmt& b) const {
        if (!same(a.pos, b.pos)) return false;
        return std::visit(
            [this](const auto& x, const auto& y) -> bool {
                if constexpr (std::is_same_v<std::decay_t<decltype(x)>, std::decay_t<decltype(y)>>) return snode(x, y);
                else return false;
            },
            a.node, b.node);
    }
    bool snode(const Assign& a, const Assign& b) const { return item(a.target, b.target) && ptr(a.value, b.value); }
    bool snode(const CallAssign& a, const CallAssign& b) const {
        return list(a.targets, b.targets) && node(a.call, b.call);
    }
    bool snode(const If& a, const If& b) const {
        return ptr(a.cond, b.cond) && ptr(a.then_branch, b.then_branch) && ptr(a.else_branch, b.else_branch);
    }
    bool snode(const While& a, const While& b) const {
        return ptr(a.cond, b.cond) && ptr(a.invariant, b.invariant) && ptr(a.body, b.body);
    }
    bool snode(const Block& a, const Block& b) const { return list(a.stmts, b.stmts); }
    bool snode(const Assert& a, const Assert& b) const { return ptr(a.predicate, b.predicate); }
    bool snode(const Assume& a, const Assume& b) const { return ptr(a.predicate, b.predicate); }
    bool snode(const ErrorStmt&, const ErrorStmt&) const { return true; }

    bool def(const Function& a, const Function& b) const {
        return same(a.pos, b.pos) && a.name == b.name && list(a.params, b.params) &&
               ptr(a.requires_clause, b.requires_clause) && list(a.results, b.results) &&
               ptr(a.ensures_clause, b.ensures_clause) && list(a.locals, b.locals) && ptr(a.body, b.body);
    }
    bool def(const Formula& a, const Formula& b) const {
        return same(a.pos, b.pos) && a.name == b.name && list(a.params, b.params) && ptr(a.body, b.body);
    }
    bool module(const Module& a, const Module& b) const {
        if (a.definitions.size() != b.definitions.size()) return false;
        for (std::size_t i = 0; i < a.definitions.size(); ++i) {
            const bool same_def = std::visit(
                [this](const auto& x, const auto& y) -> bool {
                    if constexpr (std::is_same_v<std::decay_t<decltype(x)>, std::decay_t<decltype(y)>>) return def(x, y);
                    else return false;
                },
                a.definitions[i], b.definitions[i]);
            if (!same_def) return false;
        }
        return true;
    }
};

}  // namespace

bool equal(const Expr& a, const Expr& b, EqualOptions o) { return Cmp{o.positions}.expr(a, b); }
bool equal(const Stmt& a, const Stmt& b, EqualOptions o) { return Cmp{o.positions}.stmt(a, b); }
bool equal(const Module& a, const Module& b, EqualOptions o) { return Cmp{o.positions}.module(a, b); }

}  // namespace funny::ast