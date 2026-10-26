#include "parser/parser.hpp"

#include <algorithm>
#include <limits>
#include <optional>

namespace funny::parser {

struct Kinds {
    token::Kind kw_function, kw_returns, kw_requires, kw_ensures, kw_uses, kw_if, kw_else, kw_while, kw_invariant,
        kw_assert, kw_assume, kw_forall, kw_exists, kw_true, kw_false, kw_and, kw_or, kw_not, kw_int, kw_length;
    token::Kind int_lit, ident;
    token::Kind lparen, rparen, lbracket, rbracket, lbrace, rbrace, comma, colon, semicolon;
    token::Kind arrow, fat_arrow, plus, minus, star, slash, eq, ne, le, ge, lt, gt, assign, pipe;
};

Parser::Parser(const token::TokenTable& t) {
    auto k = std::make_shared<Kinds>();
    k->kw_function = t.require("KW_FUNCTION");
    k->kw_returns = t.require("KW_RETURNS");
    k->kw_requires = t.require("KW_REQUIRES");
    k->kw_ensures = t.require("KW_ENSURES");
    k->kw_uses = t.require("KW_USES");
    k->kw_if = t.require("KW_IF");
    k->kw_else = t.require("KW_ELSE");
    k->kw_while = t.require("KW_WHILE");
    k->kw_invariant = t.require("KW_INVARIANT");
    k->kw_assert = t.require("KW_ASSERT");
    k->kw_assume = t.require("KW_ASSUME");
    k->kw_forall = t.require("KW_FORALL");
    k->kw_exists = t.require("KW_EXISTS");
    k->kw_true = t.require("KW_TRUE");
    k->kw_false = t.require("KW_FALSE");
    k->kw_and = t.require("KW_AND");
    k->kw_or = t.require("KW_OR");
    k->kw_not = t.require("KW_NOT");
    k->kw_int = t.require("KW_INT");
    k->kw_length = t.require("KW_LENGTH");
    k->int_lit = t.require("INT");
    k->ident = t.require("IDENT");
    k->lparen = t.require("LPAREN");
    k->rparen = t.require("RPAREN");
    k->lbracket = t.require("LBRACKET");
    k->rbracket = t.require("RBRACKET");
    k->lbrace = t.require("LBRACE");
    k->rbrace = t.require("RBRACE");
    k->comma = t.require("COMMA");
    k->colon = t.require("COLON");
    k->semicolon = t.require("SEMICOLON");
    k->arrow = t.require("ARROW");
    k->fat_arrow = t.require("FAT_ARROW");
    k->plus = t.require("PLUS");
    k->minus = t.require("MINUS");
    k->star = t.require("STAR");
    k->slash = t.require("SLASH");
    k->eq = t.require("EQ");
    k->ne = t.require("NE");
    k->le = t.require("LE");
    k->ge = t.require("GE");
    k->lt = t.require("LT");
    k->gt = t.require("GT");
    k->assign = t.require("ASSIGN");
    k->pipe = t.require("PIPE");
    kinds_ = std::move(k);
}

std::string to_string(const Diagnostic& d) {
    return std::to_string(d.line) + ":" + std::to_string(d.column) + ": " + d.message;
}

namespace {

constexpr token::Kind kEof = -1;
constexpr int kMaxDepth = 200; 

struct ParseError {
    ast::Position pos;
    std::string message;
};

struct Violation {
    ast::Position pos;
    std::string message;
};

bool is_comparison(ast::BinaryOp op) {
    using ast::BinaryOp;
    return op == BinaryOp::Eq || op == BinaryOp::Ne || op == BinaryOp::Lt || op == BinaryOp::Le ||
           op == BinaryOp::Gt || op == BinaryOp::Ge;
}

bool is_logical(ast::BinaryOp op) {
    using ast::BinaryOp;
    return op == BinaryOp::And || op == BinaryOp::Or || op == BinaryOp::Implies;
}

const char* describe(const ast::Expr& e) {
    return std::visit(ast::Overloaded{
                          [](const ast::IntLit&) { return "a number"; },
                          [](const ast::BoolLit&) { return "a boolean constant"; },
                          [](const ast::Var&) { return "a variable"; },
                          [](const ast::Index&) { return "an array element"; },
                          [](const ast::Call&) { return "a function call"; },
                          [](const ast::Unary& u) {
                              return u.op == ast::UnaryOp::Not ? "a logical expression" : "an arithmetic expression";
                          },
                          [](const ast::Binary& b) {
                              return is_comparison(b.op) ? "a comparison"
                                     : is_logical(b.op)  ? "a logical expression"
                                                         : "an arithmetic expression";
                          },
                          [](const ast::Quantifier&) { return "a quantifier"; },
                          [](const ast::ErrorExpr&) { return "an invalid expression"; },
                      },
                      e.node);
}

using Check = std::optional<Violation>;

Violation expected(const char* what, const ast::Expr& e) {
    return Violation{e.pos, std::string("expected ") + what + ", found " + describe(e)};
}

Check check_arith(const ast::Expr& e);

Check check_args(const std::vector<ast::ExprPtr>& args) {
    for (const auto& a : args)
        if (a)
            if (auto v = check_arith(*a)) return v;
    return std::nullopt;
}

Check check_arith(const ast::Expr& e) {
    return std::visit(ast::Overloaded{
                          [&](const ast::IntLit&) -> Check { return std::nullopt; },
                          [&](const ast::Var&) -> Check { return std::nullopt; },
                          [&](const ast::ErrorExpr&) -> Check { return std::nullopt; },
                          [&](const ast::BoolLit&) -> Check { return expected("an arithmetic expression", e); },
                          [&](const ast::Quantifier&) -> Check { return expected("an arithmetic expression", e); },
                          [&](const ast::Index& i) -> Check {
                              if (auto v = check_arith(*i.base)) return v;
                              return check_arith(*i.index);
                          },
                          [&](const ast::Call& c) -> Check { return check_args(c.args); },
                          [&](const ast::Unary& u) -> Check {
                              if (u.op == ast::UnaryOp::Not) return expected("an arithmetic expression", e);
                              return check_arith(*u.operand);
                          },
                          [&](const ast::Binary& b) -> Check {
                              if (is_comparison(b.op) || is_logical(b.op)) return expected("an arithmetic expression", e);
                              if (auto v = check_arith(*b.lhs)) return v;
                              return check_arith(*b.rhs);
                          },
                      },
                      e.node);
}

Check check_condition(const ast::Expr& e) {
    return std::visit(ast::Overloaded{
                          [&](const ast::BoolLit&) -> Check { return std::nullopt; },
                          [&](const ast::ErrorExpr&) -> Check { return std::nullopt; },
                          [&](const ast::Unary& u) -> Check {
                              if (u.op != ast::UnaryOp::Not) return expected("a condition", e);
                              return check_condition(*u.operand);
                          },
                          [&](const ast::Binary& b) -> Check {
                              if (is_comparison(b.op)) {
                                  if (auto v = check_arith(*b.lhs)) return v;
                                  return check_arith(*b.rhs);
                              }
                              if (is_logical(b.op)) {
                                  if (auto v = check_condition(*b.lhs)) return v;
                                  return check_condition(*b.rhs);
                              }
                              return expected("a condition", e);
                          },
                          [&](const auto&) -> Check { return expected("a condition", e); },
                      },
                      e.node);
}

Check check_predicate(const ast::Expr& e) {
    return std::visit(ast::Overloaded{
                          [&](const ast::BoolLit&) -> Check { return std::nullopt; },
                          [&](const ast::ErrorExpr&) -> Check { return std::nullopt; },
                          [&](const ast::Call& c) -> Check { return check_args(c.args); },
                          [&](const ast::Quantifier& q) -> Check { return check_predicate(*q.body); },
                          [&](const ast::Unary& u) -> Check {
                              if (u.op != ast::UnaryOp::Not) return expected("a predicate", e);
                              return check_predicate(*u.operand);
                          },
                          [&](const ast::Binary& b) -> Check {
                              if (is_comparison(b.op)) {
                                  if (auto v = check_arith(*b.lhs)) return v;
                                  return check_arith(*b.rhs);
                              }
                              if (is_logical(b.op)) {
                                  if (auto v = check_predicate(*b.lhs)) return v;
                                  return check_predicate(*b.rhs);
                              }
                              return expected("a predicate", e);
                          },
                          [&](const auto&) -> Check { return expected("a predicate", e); },
                      },
                      e.node);
}

class State {
public:
    State(const Kinds& kinds, std::span<const token::Token> tokens)
        : k_(kinds), toks_(tokens.begin(), tokens.end()) {
        token::Token eof{kEof, std::string_view(), 0, 1, 1};
        if (!toks_.empty()) {
            const token::Token& last = toks_.back();
            eof.offset = last.offset + last.lexeme.size();
            eof.line = last.line;
            eof.column = last.column + last.lexeme.size();
        }
        toks_.push_back(eof);
    }

    ParseResult run() {
        ParseResult result;
        result.module = std::make_unique<ast::Module>();
        if (at_eof()) report(here(), "expected a function or formula definition, got end of input");

        while (!at_eof()) {
            const std::size_t start = pos_;
            try {
                result.module->definitions.push_back(definition());
            } catch (const ParseError& e) {
                report(e.pos, e.message);
                synchronize_definition();
            }
            if (pos_ == start && !at_eof()) advance(); 
        }
        result.errors = std::move(errors_);
        return result;
    }

private:
    const Kinds& k_;
    std::vector<token::Token> toks_;
    std::size_t pos_ = 0;
    int depth_ = 0;
    std::vector<Diagnostic> errors_;

    const token::Token& peek() const { return toks_[pos_]; }
    const token::Token& peek_next() const { return toks_[std::min(pos_ + 1, toks_.size() - 1)]; }
    bool at_eof() const { return peek().kind == kEof; }
    bool check(token::Kind kind) const { return peek().kind == kind; }

    const token::Token& advance() {
        const token::Token& t = toks_[pos_];
        if (pos_ + 1 < toks_.size()) ++pos_;
        return t;
    }
    bool match(token::Kind kind) {
        if (!check(kind)) return false;
        advance();
        return true;
    }

    bool at_definition_start() const {
        return check(k_.kw_function) || (check(k_.ident) && peek_next().kind == k_.lparen);
    }

    static ast::Position at(const token::Token& t) { return ast::Position{t.line, t.column}; }
    ast::Position here() const { return at(peek()); }

    static std::string describe(const token::Token& t) {
        return t.kind == kEof ? "end of input" : "'" + std::string(t.lexeme) + "'";
    }

    [[noreturn]] void fail_here(const std::string& message) const { throw ParseError{here(), message}; }

    const token::Token& expect(token::Kind kind, const char* what) {
        if (!check(kind)) fail_here(std::string("expected ") + what + ", got " + describe(peek()));
        return advance();
    }

    void report(ast::Position pos, std::string message) {
        if (!errors_.empty() && errors_.back().line == pos.line && errors_.back().column == pos.column) return;
        errors_.push_back(Diagnostic{pos.line, pos.column, std::move(message)});
    }
    void apply(const Check& v) {
        if (v) report(v->pos, v->message);
    }

    struct DepthGuard {
        State& s;
        explicit DepthGuard(State& st) : s(st) {
            if (++s.depth_ > kMaxDepth) {
                --s.depth_;
                s.fail_here("nesting is too deep");
            }
        }
        ~DepthGuard() { --s.depth_; }
        DepthGuard(const DepthGuard&) = delete;
        DepthGuard& operator=(const DepthGuard&) = delete;
    };

    void synchronize_statement() {
        while (!at_eof()) {
            if (check(k_.semicolon)) {
                advance();
                return;
            }
            if (check(k_.rbrace) || check(k_.lbrace) || check(k_.kw_function) || check(k_.kw_if) ||
                check(k_.kw_while) || check(k_.kw_assert) || check(k_.kw_assume))
                return;
            advance();
        }
    }

    void synchronize_definition() {
        int depth = 0;
        while (!at_eof()) {
            if (depth == 0 && check(k_.kw_function)) return;
            if (depth == 0 && check(k_.ident) && peek_next().kind == k_.lparen && pos_ > 0 &&
                toks_[pos_ - 1].kind == k_.rbrace)
                return;
            if (check(k_.lbrace)) ++depth;
            else if (check(k_.rbrace) && depth > 0) --depth;
            advance();
        }
    }

    ast::Definition definition() {
        const ast::Position pos = here();
        const bool has_keyword = check(k_.kw_function);
        if (has_keyword) advance();
        else if (!check(k_.ident)) fail_here("expected a function or formula definition, got " + describe(peek()));

        std::string name(expect(k_.ident, "a function name").lexeme);
        expect(k_.lparen, "'('");
        std::vector<ast::Param> params = params_until_rparen();
        expect(k_.rparen, "')'");

        if (!has_keyword) {
            if (match(k_.fat_arrow)) { 
                ast::Formula f;
                f.pos = pos;
                f.name = std::move(name);
                f.params = std::move(params);
                f.body = predicate();
                return f;
            }
            if (!check(k_.kw_requires) && !check(k_.kw_returns))
                fail_here("expected '=>', 'requires' or 'returns', got " + describe(peek()));
        }
        return function_rest(pos, std::move(name), std::move(params));
    }

    ast::Param variable_def() {
        ast::Param p;
        p.pos = here();
        p.name = std::string(expect(k_.ident, "an identifier").lexeme);
        expect(k_.colon, "':'");
        p.type = variable_type();
        return p;
    }

    ast::Type variable_type() {
        expect(k_.kw_int, "a type ('int' or 'int[]')");
        if (match(k_.lbracket)) {
            expect(k_.rbracket, "']'");
            return ast::Type::IntArray;
        }
        return ast::Type::Int;
    }

    std::vector<ast::Param> params_until_rparen() {
        std::vector<ast::Param> list;
        if (!check(k_.rparen)) {
            list.push_back(variable_def());
            while (match(k_.comma)) list.push_back(variable_def());
        }
        return list;
    }

    ast::Function function_rest(ast::Position pos, std::string name, std::vector<ast::Param> params) {
        ast::Function f;
        f.pos = pos;
        f.name = std::move(name);
        f.params = std::move(params);
        if (match(k_.kw_requires)) f.requires_clause = predicate();
        expect(k_.kw_returns, "'returns'");
        f.results.push_back(variable_def());
        while (match(k_.comma)) f.results.push_back(variable_def());
        if (match(k_.kw_ensures)) f.ensures_clause = predicate();
        if (match(k_.kw_uses)) {
            do {
                ast::LocalVar v;
                v.pos = here();
                v.name = std::string(expect(k_.ident, "an identifier").lexeme);
                if (match(k_.colon)) v.type = variable_type();
                f.locals.push_back(std::move(v));
            } while (match(k_.comma));
        }
        f.body = statement_recovering();
        return f;
    }

    ast::StmtPtr statement_recovering() {
        const std::size_t start = pos_;
        const ast::Position pos = here();
        try {
            return statement();
        } catch (const ParseError& e) {
            report(e.pos, e.message);
            synchronize_statement();
            if (pos_ == start && !at_eof()) advance(); 
            return ast::make_stmt(pos, ast::ErrorStmt{});
        }
    }

    ast::StmtPtr statement() {
        DepthGuard guard(*this);
        const token::Token& t = peek();
        if (t.kind == k_.kw_if) return if_stmt();
        if (t.kind == k_.kw_while) return while_stmt();
        if (t.kind == k_.lbrace) return block();
        if (t.kind == k_.kw_assert) return assert_like(true);
        if (t.kind == k_.kw_assume) return assert_like(false);
        if (t.kind == k_.ident) return assignment();
        fail_here("expected a statement, got " + describe(t));
    }

    ast::StmtPtr if_stmt() {
        const ast::Position pos = here();
        advance();
        expect(k_.lparen, "'('");
        ast::ExprPtr cond = condition();
        expect(k_.rparen, "')'");
        ast::StmtPtr then_branch = statement_recovering();
        ast::StmtPtr else_branch;
        if (match(k_.kw_else)) else_branch = statement_recovering(); 
        return ast::make_stmt(pos, ast::If{std::move(cond), std::move(then_branch), std::move(else_branch)});
    }

    ast::StmtPtr while_stmt() {
        const ast::Position pos = here();
        advance();
        expect(k_.lparen, "'('");
        ast::ExprPtr cond = condition();
        expect(k_.rparen, "')'");
        ast::ExprPtr invariant;
        if (match(k_.kw_invariant)) invariant = predicate();
        ast::StmtPtr body = statement_recovering();
        return ast::make_stmt(pos, ast::While{std::move(cond), std::move(invariant), std::move(body)});
    }

    ast::StmtPtr block() {
        const ast::Position pos = here();
        advance();
        ast::Block b;
        while (!check(k_.rbrace) && !at_eof() && !at_definition_start()) b.stmts.push_back(statement_recovering());
        if (check(k_.rbrace)) advance();
        else report(here(), "expected '}', got " + describe(peek()));
        return ast::make_stmt(pos, std::move(b));
    }

    ast::StmtPtr assert_like(bool is_assert) {
        const ast::Position pos = here();
        advance();
        ast::ExprPtr e = predicate();
        expect(k_.semicolon, "';'");
        if (is_assert) return ast::make_stmt(pos, ast::Assert{std::move(e)});
        return ast::make_stmt(pos, ast::Assume{std::move(e)});
    }

    ast::StmtPtr assignment() {
        const ast::Position pos = here();
        std::string name(advance().lexeme);

        if (check(k_.lbracket)) {
            ast::LValue lv;
            lv.pos = pos;
            lv.name = std::move(name);
            lv.indices = indices();
            expect(k_.assign, "'='");
            ast::ExprPtr value = arithmetic();
            expect(k_.semicolon, "';'");
            return ast::make_stmt(pos, ast::Assign{std::move(lv), std::move(value)});
        }

        if (check(k_.lparen)) fail_here("calls are only allowed on the right side of an assignment");

        std::vector<ast::Target> targets;
        targets.push_back(ast::Target{pos, std::move(name)});
        while (match(k_.comma)) {
            const ast::Position p = here();
            targets.push_back(ast::Target{p, std::string(expect(k_.ident, "an identifier").lexeme)});
        }
        expect(k_.assign, "'='");
        ast::ExprPtr value = expr();
        expect(k_.semicolon, "';'");

        if (targets.size() == 1) {
            apply(check_arith(*value));
            ast::LValue lv;
            lv.pos = pos;
            lv.name = std::move(targets[0].name);
            return ast::make_stmt(pos, ast::Assign{std::move(lv), std::move(value)});
        }
        ast::Call* call = std::get_if<ast::Call>(&value->node);
        if (!call) {
            report(value->pos, "expected a function call on the right side of a multiple assignment");
            return ast::make_stmt(pos, ast::ErrorStmt{});
        }
        apply(check_arith(*value));
        return ast::make_stmt(pos, ast::CallAssign{std::move(targets), std::move(*call)});
    }

    std::vector<ast::ExprPtr> indices() {
        std::vector<ast::ExprPtr> list;
        while (match(k_.lbracket)) {
            list.push_back(arithmetic());
            expect(k_.rbracket, "']'");
        }
        return list;
    }

    ast::ExprPtr arithmetic() {
        ast::ExprPtr e = expr();
        apply(check_arith(*e));
        return e;
    }
    ast::ExprPtr condition() {
        ast::ExprPtr e = expr();
        apply(check_condition(*e));
        return e;
    }
    ast::ExprPtr predicate() {
        ast::ExprPtr e = expr();
        apply(check_predicate(*e));
        return e;
    }

    ast::ExprPtr expr() { return implication(); }

    ast::ExprPtr implication() {
        DepthGuard guard(*this);
        ast::ExprPtr left = disjunction();
        if (!check(k_.arrow)) return left;
        advance();
        ast::ExprPtr right = implication();
        const ast::Position pos = left->pos;
        return ast::make_expr(pos, ast::Binary{ast::BinaryOp::Implies, std::move(left), std::move(right)});
    }

    ast::ExprPtr disjunction() {
        ast::ExprPtr left = conjunction();
        while (check(k_.kw_or)) {
            advance();
            ast::ExprPtr right = conjunction();
            const ast::Position pos = left->pos;
            left = ast::make_expr(pos, ast::Binary{ast::BinaryOp::Or, std::move(left), std::move(right)});
        }
        return left;
    }

    ast::ExprPtr conjunction() {
        ast::ExprPtr left = negation();
        while (check(k_.kw_and)) {
            advance();
            ast::ExprPtr right = negation();
            const ast::Position pos = left->pos;
            left = ast::make_expr(pos, ast::Binary{ast::BinaryOp::And, std::move(left), std::move(right)});
        }
        return left;
    }

    ast::ExprPtr negation() {
        if (!check(k_.kw_not)) return comparison();
        DepthGuard guard(*this);
        const ast::Position pos = here();
        advance();
        ast::ExprPtr operand = negation();
        return ast::make_expr(pos, ast::Unary{ast::UnaryOp::Not, std::move(operand)});
    }

    std::optional<ast::BinaryOp> relation() const {
        const token::Kind k = peek().kind;
        if (k == k_.eq) return ast::BinaryOp::Eq;
        if (k == k_.ne) return ast::BinaryOp::Ne;
        if (k == k_.lt) return ast::BinaryOp::Lt;
        if (k == k_.le) return ast::BinaryOp::Le;
        if (k == k_.gt) return ast::BinaryOp::Gt;
        if (k == k_.ge) return ast::BinaryOp::Ge;
        return std::nullopt;
    }

    ast::ExprPtr comparison() {
        ast::ExprPtr left = additive();
        const std::optional<ast::BinaryOp> op = relation();
        if (!op) return left;
        advance();
        ast::ExprPtr right = additive();
        if (relation()) fail_here("comparison operators cannot be chained");
        const ast::Position pos = left->pos;
        return ast::make_expr(pos, ast::Binary{*op, std::move(left), std::move(right)});
    }

    ast::ExprPtr additive() {
        ast::ExprPtr left = multiplicative();
        while (check(k_.plus) || check(k_.minus)) {
            const ast::BinaryOp op = check(k_.plus) ? ast::BinaryOp::Add : ast::BinaryOp::Sub;
            advance();
            ast::ExprPtr right = multiplicative();
            const ast::Position pos = left->pos;
            left = ast::make_expr(pos, ast::Binary{op, std::move(left), std::move(right)});
        }
        return left;
    }

    ast::ExprPtr multiplicative() {
        ast::ExprPtr left = unary();
        while (check(k_.star) || check(k_.slash)) {
            const ast::BinaryOp op = check(k_.star) ? ast::BinaryOp::Mul : ast::BinaryOp::Div;
            advance();
            ast::ExprPtr right = unary();
            const ast::Position pos = left->pos;
            left = ast::make_expr(pos, ast::Binary{op, std::move(left), std::move(right)});
        }
        return left;
    }

    ast::ExprPtr unary() {
        if (!check(k_.minus)) return primary();
        DepthGuard guard(*this);
        const ast::Position pos = here();
        advance();
        ast::ExprPtr operand = unary();
        return ast::make_expr(pos, ast::Unary{ast::UnaryOp::Neg, std::move(operand)});
    }

    std::vector<ast::ExprPtr> call_args() {
        std::vector<ast::ExprPtr> args;
        expect(k_.lparen, "'('");
        if (!check(k_.rparen)) {
            args.push_back(expr());
            while (match(k_.comma)) args.push_back(expr());
        }
        expect(k_.rparen, "')'");
        return args;
    }

    ast::ExprPtr int_literal(const token::Token& t) {
        const ast::Position pos = at(t);
        std::int64_t value = 0;
        for (char c : t.lexeme) {
            const int digit = c - '0';
            if (value > (std::numeric_limits<std::int64_t>::max() - digit) / 10) {
                report(pos, "integer literal is too large");
                return ast::make_expr(pos, ast::ErrorExpr{});
            }
            value = value * 10 + digit;
        }
        return ast::make_expr(pos, ast::IntLit{value});
    }

    ast::ExprPtr primary() {
        const token::Token& t = peek();
        const ast::Position pos = at(t);

        if (t.kind == k_.int_lit) {
            advance();
            return int_literal(t);
        }
        if (t.kind == k_.kw_true || t.kind == k_.kw_false) {
            advance();
            return ast::make_expr(pos, ast::BoolLit{t.kind == k_.kw_true});
        }
        if (t.kind == k_.ident) {
            std::string name(t.lexeme);
            advance();
            if (check(k_.lparen)) return ast::make_expr(pos, ast::Call{std::move(name), call_args()});
            ast::ExprPtr e = ast::make_expr(pos, ast::Var{std::move(name)});
            while (match(k_.lbracket)) {
                ast::ExprPtr index = arithmetic();
                expect(k_.rbracket, "']'");
                e = ast::make_expr(pos, ast::Index{std::move(e), std::move(index)});
            }
            return e;
        }
        if (t.kind == k_.kw_length) {
            advance();
            return ast::make_expr(pos, ast::Call{"length", call_args()});
        }
        if (t.kind == k_.kw_forall || t.kind == k_.kw_exists) {
            const ast::QuantifierKind kind =
                t.kind == k_.kw_forall ? ast::QuantifierKind::Forall : ast::QuantifierKind::Exists;
            advance();
            expect(k_.lparen, "'('");
            ast::Param var = variable_def();
            expect(k_.pipe, "'|'");
            ast::ExprPtr body = expr();
            expect(k_.rparen, "')'");
            return ast::make_expr(pos, ast::Quantifier{kind, std::move(var), std::move(body)});
        }
        if (t.kind == k_.lparen) {
            advance();
            ast::ExprPtr e = expr();
            expect(k_.rparen, "')'");
            return e;
        }
        fail_here("expected an expression, got " + describe(t));
    }
};

}  // namespace

ParseResult Parser::parse(std::span<const token::Token> tokens) const {
    return State(*kinds_, tokens).run();
}

}  // namespace funny::parser