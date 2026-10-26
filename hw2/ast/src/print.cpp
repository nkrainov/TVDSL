#include "ast/print.hpp"

namespace funny::ast {

namespace {

class Printer {
public:
    explicit Printer(PrintOptions options) : options_(options) {}

    std::string expr(const Expr& e) const {
        return std::visit(
            Overloaded{
                [&](const IntLit& n) { return std::to_string(n.value) + at(e.pos); },
                [&](const BoolLit& b) { return std::string(b.value ? "true" : "false") + at(e.pos); },
                [&](const Var& v) { return v.name + at(e.pos); },
                [&](const Index& i) {
                    return "(index" + at(e.pos) + " " + expr_or_null(i.base) + " " + expr_or_null(i.index) + ")";
                },
                [&](const Call& c) { return call(c, at(e.pos)); },
                [&](const Unary& u) {
                    return "(" + std::string(to_string(u.op)) + at(e.pos) + " " + expr_or_null(u.operand) + ")";
                },
                [&](const Binary& b) {
                    return "(" + std::string(to_string(b.op)) + at(e.pos) + " " + expr_or_null(b.lhs) + " " +
                           expr_or_null(b.rhs) + ")";
                },
                [&](const Quantifier& q) {
                    return "(" + std::string(to_string(q.kind)) + at(e.pos) + " (" + q.var.name + ": " +
                           to_string(q.var.type) + ") " + expr_or_null(q.body) + ")";
                },
                [&](const ErrorExpr&) { return "<error>" + at(e.pos); },
            },
            e.node);
    }

    void stmt(const Stmt& s, int indent) {
        std::visit(
            Overloaded{
                [&](const Assign& a) {
                    line(indent, "assign" + at(s.pos) + " " + lvalue(a.target) + " = " + expr_or_null(a.value));
                },
                [&](const CallAssign& a) {
                    std::string names;
                    for (std::size_t i = 0; i < a.targets.size(); ++i)
                        names += (i ? ", " : "") + a.targets[i].name;
                    line(indent, "call-assign" + at(s.pos) + " " + names + " = " + call(a.call, ""));
                },
                [&](const If& i) {
                    line(indent, "if" + at(s.pos) + " " + expr_or_null(i.cond));
                    stmt_or_null(i.then_branch, indent + 1);
                    if (i.else_branch) {
                        line(indent, "else");
                        stmt(*i.else_branch, indent + 1);
                    }
                },
                [&](const While& w) {
                    line(indent, "while" + at(s.pos) + " " + expr_or_null(w.cond));
                    if (w.invariant) line(indent + 1, "invariant " + expr(*w.invariant));
                    stmt_or_null(w.body, indent + 1);
                },
                [&](const Block& b) {
                    line(indent, "block" + at(s.pos));
                    for (const auto& child : b.stmts) stmt_or_null(child, indent + 1);
                },
                [&](const Assert& a) { line(indent, "assert" + at(s.pos) + " " + expr_or_null(a.predicate)); },
                [&](const Assume& a) { line(indent, "assume" + at(s.pos) + " " + expr_or_null(a.predicate)); },
                [&](const ErrorStmt&) { line(indent, "<error>" + at(s.pos)); },
            },
            s.node);
    }

    void module(const Module& m) {
        for (const Definition& d : m.definitions) {
            std::visit(
                Overloaded{
                    [&](const Function& f) {
                        line(0, "function" + at(f.pos) + " " + f.name + "(" + params(f.params) + ") returns (" +
                                    params(f.results) + ")");
                        if (f.requires_clause) line(1, "requires " + expr(*f.requires_clause));
                        if (f.ensures_clause) line(1, "ensures " + expr(*f.ensures_clause));
                        if (!f.locals.empty()) {
                            std::string list;
                            for (std::size_t i = 0; i < f.locals.size(); ++i) {
                                list += (i ? ", " : "") + f.locals[i].name;
                                if (f.locals[i].type) list += std::string(": ") + to_string(*f.locals[i].type);
                            }
                            line(1, "uses " + list);
                        }
                        stmt_or_null(f.body, 1);
                    },
                    [&](const Formula& f) {
                        line(0, "formula" + at(f.pos) + " " + f.name + "(" + params(f.params) + ") => " +
                                    expr_or_null(f.body));
                    },
                },
                d);
        }
    }

    std::string take() { return std::move(out_); }

private:
    PrintOptions options_;
    std::string out_;

    std::string at(Position p) const {
        if (!options_.positions) return "";
        return "@" + std::to_string(p.line) + ":" + std::to_string(p.column);
    }

    void line(int indent, const std::string& text) {
        out_.append(static_cast<std::size_t>(indent) * 2, ' ');
        out_ += text;
        out_ += '\n';
    }

    std::string expr_or_null(const ExprPtr& p) const { return p ? expr(*p) : "<null>"; }

    void stmt_or_null(const StmtPtr& p, int indent) {
        if (p) stmt(*p, indent);
        else line(indent, "<null>");
    }

    std::string call(const Call& c, const std::string& pos_suffix) const {
        std::string s = "(call" + pos_suffix + " " + c.name;
        for (const auto& a : c.args) s += " " + expr_or_null(a);
        return s + ")";
    }

    std::string lvalue(const LValue& lv) const {
        std::string s = lv.name;
        for (const auto& idx : lv.indices) s += "[" + expr_or_null(idx) + "]";
        return s;
    }

    static std::string params(const std::vector<Param>& list) {
        std::string s;
        for (std::size_t i = 0; i < list.size(); ++i)
            s += (i ? ", " : "") + list[i].name + ": " + to_string(list[i].type);
        return s;
    }
};

}  // namespace

std::string to_string(const Expr& expr, PrintOptions options) {
    return Printer(options).expr(expr);
}

std::string to_string(const Stmt& stmt, PrintOptions options) {
    Printer p(options);
    p.stmt(stmt, 0);
    return p.take();
}

std::string to_string(const Module& module, PrintOptions options) {
    Printer p(options);
    p.module(module);
    return p.take();
}

}  // namespace funny::ast