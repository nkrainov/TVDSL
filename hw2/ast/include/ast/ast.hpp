#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace funny::ast {

struct Position {
    std::size_t line = 0;
    std::size_t column = 0;
};

enum class Type { Int, IntArray };
enum class UnaryOp { Neg, Not };  
enum class BinaryOp {
    Add, Sub, Mul, Div,                                  
    Eq, Ne, Lt, Le, Gt, Ge,                              
    And, Or, Implies                                     
};
enum class QuantifierKind { Forall, Exists };

const char* to_string(Type type);
const char* to_string(UnaryOp op);
const char* to_string(BinaryOp op);
const char* to_string(QuantifierKind kind);

struct Param {
    Position pos;
    std::string name;
    Type type = Type::Int;
};

struct LocalVar {
    Position pos;
    std::string name;
    std::optional<Type> type;
};

struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct IntLit { std::int64_t value = 0; };
struct BoolLit { bool value = false; };
struct Var { std::string name; };
struct Index { ExprPtr base; ExprPtr index; };
struct Call { std::string name; std::vector<ExprPtr> args; }; 
struct Unary { UnaryOp op = UnaryOp::Neg; ExprPtr operand; };
struct Binary { BinaryOp op = BinaryOp::Add; ExprPtr lhs; ExprPtr rhs; };
struct Quantifier { QuantifierKind kind = QuantifierKind::Forall; Param var; ExprPtr body; };
struct ErrorExpr {};                                          

using ExprNode = std::variant<IntLit, BoolLit, Var, Index, Call, Unary, Binary, Quantifier, ErrorExpr>;

struct Expr {
    Position pos;
    ExprNode node;
};

struct LValue {
    Position pos;
    std::string name;
    std::vector<ExprPtr> indices;
};

struct Target {
    Position pos;
    std::string name;
};

struct Assign { LValue target; ExprPtr value; };
struct CallAssign { std::vector<Target> targets; Call call; };
struct If { ExprPtr cond; StmtPtr then_branch; StmtPtr else_branch; };
struct While { ExprPtr cond; ExprPtr invariant; StmtPtr body; };
struct Block { std::vector<StmtPtr> stmts; };
struct Assert { ExprPtr predicate; };
struct Assume { ExprPtr predicate; };
struct ErrorStmt {};                                                  

using StmtNode = std::variant<Assign, CallAssign, If, While, Block, Assert, Assume, ErrorStmt>;

struct Stmt {
    Position pos;
    StmtNode node;
};

struct Function {
    Position pos;
    std::string name;
    std::vector<Param> params;
    ExprPtr requires_clause;
    std::vector<Param> results;
    ExprPtr ensures_clause;
    std::vector<LocalVar> locals;
    StmtPtr body;
};

struct Formula {
    Position pos; 
    std::string name;
    std::vector<Param> params;
    ExprPtr body;
};

using Definition = std::variant<Function, Formula>;

struct Module {
    std::vector<Definition> definitions;
};

template <class Node>
ExprPtr make_expr(Position pos, Node node) {
    auto e = std::make_unique<Expr>();
    e->pos = pos;
    e->node = std::move(node);
    return e;
}

template <class Node>
StmtPtr make_stmt(Position pos, Node node) {
    auto s = std::make_unique<Stmt>();
    s->pos = pos;
    s->node = std::move(node);
    return s;
}
template <class... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};
template <class... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

}  // namespace funny::ast