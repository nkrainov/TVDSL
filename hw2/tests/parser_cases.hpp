#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ast/ast.hpp"
#include "ast_builder.hpp"
#include "token/tokentable.hpp"

namespace funny::parser_tests {

namespace b = funny::parser_tests::b;

struct Tok {
    std::string kind;
    std::string lexeme;
};
using Tokens = std::vector<Tok>;

namespace tk {
inline const Tok KW_FUNCTION{"KW_FUNCTION", "function"};
inline const Tok KW_RETURNS{"KW_RETURNS", "returns"};
inline const Tok KW_REQUIRES{"KW_REQUIRES", "requires"};
inline const Tok KW_ENSURES{"KW_ENSURES", "ensures"};
inline const Tok KW_USES{"KW_USES", "uses"};
inline const Tok KW_IF{"KW_IF", "if"};
inline const Tok KW_ELSE{"KW_ELSE", "else"};
inline const Tok KW_WHILE{"KW_WHILE", "while"};
inline const Tok KW_INVARIANT{"KW_INVARIANT", "invariant"};
inline const Tok KW_ASSERT{"KW_ASSERT", "assert"};
inline const Tok KW_ASSUME{"KW_ASSUME", "assume"};
inline const Tok KW_FORALL{"KW_FORALL", "forall"};
inline const Tok KW_EXISTS{"KW_EXISTS", "exists"};
inline const Tok KW_TRUE{"KW_TRUE", "true"};
inline const Tok KW_FALSE{"KW_FALSE", "false"};
inline const Tok KW_AND{"KW_AND", "and"};
inline const Tok KW_OR{"KW_OR", "or"};
inline const Tok KW_NOT{"KW_NOT", "not"};
inline const Tok KW_INT{"KW_INT", "int"};
inline const Tok KW_LENGTH{"KW_LENGTH", "length"};
inline const Tok LPAREN{"LPAREN", "("};
inline const Tok RPAREN{"RPAREN", ")"};
inline const Tok LBRACKET{"LBRACKET", "["};
inline const Tok RBRACKET{"RBRACKET", "]"};
inline const Tok LBRACE{"LBRACE", "{"};
inline const Tok RBRACE{"RBRACE", "}"};
inline const Tok COMMA{"COMMA", ","};
inline const Tok COLON{"COLON", ":"};
inline const Tok SEMICOLON{"SEMICOLON", ";"};
inline const Tok ARROW{"ARROW", "->"};
inline const Tok FAT_ARROW{"FAT_ARROW", "=>"};
inline const Tok PLUS{"PLUS", "+"};
inline const Tok MINUS{"MINUS", "-"};
inline const Tok STAR{"STAR", "*"};
inline const Tok SLASH{"SLASH", "/"};
inline const Tok EQ{"EQ", "=="};
inline const Tok NE{"NE", "!="};
inline const Tok LE{"LE", "<="};
inline const Tok GE{"GE", ">="};
inline const Tok LT{"LT", "<"};
inline const Tok GT{"GT", ">"};
inline const Tok ASSIGN{"ASSIGN", "="};
inline const Tok PIPE{"PIPE", "|"};
inline const Tok HERE{"", ""};
inline Tok IDENT(std::string name) { return Tok{"IDENT", std::move(name)}; }
inline Tok NUMBER(std::string digits) { return Tok{"INT", std::move(digits)}; } 
}  // namespace tk

inline std::vector<token::TokenInfo> token_infos() {
    std::vector<token::TokenInfo> v;
    for (const char* n : {"KW_FUNCTION", "KW_RETURNS", "KW_REQUIRES", "KW_ENSURES", "KW_USES", "KW_IF", "KW_ELSE",
                          "KW_WHILE", "KW_INVARIANT", "KW_ASSERT", "KW_ASSUME", "KW_FORALL", "KW_EXISTS", "KW_TRUE",
                          "KW_FALSE", "KW_AND", "KW_OR", "KW_NOT", "KW_INT", "KW_LENGTH", "INT", "IDENT"})
        v.push_back({n, false});
    v.push_back({"WS", true});
    v.push_back({"COMMENT", true});
    for (const char* n : {"LPAREN", "RPAREN", "LBRACKET", "RBRACKET", "LBRACE", "RBRACE", "COMMA", "COLON", "SEMICOLON",
                          "ARROW", "FAT_ARROW", "PLUS", "MINUS", "STAR", "SLASH", "EQ", "NE", "LE", "GE", "LT", "GT",
                          "ASSIGN", "PIPE"})
        v.push_back({n, false});
    return v;
}

inline Tokens cat(Tokens a, const Tokens& rest) {
    a.insert(a.end(), rest.begin(), rest.end());
    return a;
}
inline Tokens in_f(const Tokens& body) {
    using namespace tk;
    return cat(cat({KW_FUNCTION, IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE}, body), {RBRACE});
}
template <class... Ss>
ast::Module module_f(Ss... stmts) {
    return b::mod(b::fn("f", {}, {b::param("r", b::Int)}).body(b::block(std::move(stmts)...)));
}

struct OkCase {
    std::string name;
    Tokens tokens;
    ast::Module expected;
};

struct ErrorCase {
    std::string name;
    Tokens tokens;
    std::vector<std::string> messages;        
    std::optional<ast::Module> expected;      
};

inline std::vector<OkCase> ok_cases() {
    using namespace tk;
    std::vector<OkCase> c;

    c.push_back({"minimal function",
                 {KW_FUNCTION, IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE,
                  IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON, RBRACE},
                 module_f(b::assign("r", b::num(1)))});

    c.push_back({"function without the 'function' keyword",
                 {IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE,
                  IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON, RBRACE},
                 module_f(b::assign("r", b::num(1)))});

    c.push_back({"requires, ensures, uses and arithmetic precedence",
                 {KW_FUNCTION, IDENT("f"), LPAREN, IDENT("a"), COLON, KW_INT, LBRACKET, RBRACKET, COMMA, IDENT("n"),
                  COLON, KW_INT, RPAREN, KW_REQUIRES, IDENT("n"), GT, NUMBER("0"), KW_RETURNS, IDENT("r"), COLON,
                  KW_INT, KW_ENSURES, IDENT("r"), GE, NUMBER("0"), KW_USES, IDENT("i"), LBRACE,
                  IDENT("i"), ASSIGN, NUMBER("0"), SEMICOLON,
                  IDENT("r"), ASSIGN, IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, PLUS, NUMBER("2"), STAR, IDENT("n"),
                  MINUS, NUMBER("1"), SEMICOLON, RBRACE},
                 b::mod(b::fn("f", {b::param("a", b::IntArray), b::param("n", b::Int)}, {b::param("r", b::Int)})
                            .requires_(b::gt(b::var("n"), b::num(0)))
                            .ensures(b::ge(b::var("r"), b::num(0)))
                            .uses({b::local("i")})
                            .body(b::block(b::assign("i", b::num(0)),
                                           b::assign("r", b::sub(b::add(b::idx(b::var("a"), b::var("i")),
                                                                        b::mul(b::num(2), b::var("n"))),
                                                                 b::num(1))))))});

    c.push_back({"several results, typed and untyped locals",
                 {KW_FUNCTION, IDENT("g"), LPAREN, RPAREN, KW_RETURNS, IDENT("x"), COLON, KW_INT, COMMA, IDENT("y"),
                  COLON, KW_INT, LBRACKET, RBRACKET, KW_USES, IDENT("t"), COLON, KW_INT, COMMA, IDENT("u"), LBRACE,
                  IDENT("x"), ASSIGN, NUMBER("0"), SEMICOLON, RBRACE},
                 b::mod(b::fn("g", {}, {b::param("x", b::Int), b::param("y", b::IntArray)})
                            .uses({b::local("t", b::Int), b::local("u")})
                            .body(b::block(b::assign("x", b::num(0)))))});

    c.push_back({"body is a single statement, not a block",
                 {KW_FUNCTION, IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT,
                  IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON},
                 b::mod(b::fn("f", {}, {b::param("r", b::Int)}).body(b::assign("r", b::num(1))))});

    c.push_back({"empty block", in_f({}), module_f()});

    c.push_back({"while with invariant, if and else",
                 {KW_FUNCTION, IDENT("g"), LPAREN, IDENT("n"), COLON, KW_INT, RPAREN, KW_RETURNS, IDENT("s"), COLON,
                  KW_INT, KW_USES, IDENT("i"), LBRACE,
                  IDENT("i"), ASSIGN, NUMBER("0"), SEMICOLON, IDENT("s"), ASSIGN, NUMBER("0"), SEMICOLON,
                  KW_WHILE, LPAREN, IDENT("i"), LT, IDENT("n"), RPAREN, KW_INVARIANT, IDENT("s"), GE, NUMBER("0"),
                  LBRACE, IDENT("s"), ASSIGN, IDENT("s"), PLUS, IDENT("i"), SEMICOLON,
                  IDENT("i"), ASSIGN, IDENT("i"), PLUS, NUMBER("1"), SEMICOLON, RBRACE,
                  KW_IF, LPAREN, IDENT("s"), EQ, NUMBER("0"), RPAREN, IDENT("s"), ASSIGN, NUMBER("1"), SEMICOLON,
                  KW_ELSE, IDENT("s"), ASSIGN, NUMBER("2"), SEMICOLON, RBRACE},
                 b::mod(b::fn("g", {b::param("n", b::Int)}, {b::param("s", b::Int)})
                            .uses({b::local("i")})
                            .body(b::block(
                                b::assign("i", b::num(0)), b::assign("s", b::num(0)),
                                b::while_(b::lt(b::var("i"), b::var("n")), b::ge(b::var("s"), b::num(0)),
                                          b::block(b::assign("s", b::add(b::var("s"), b::var("i"))),
                                                   b::assign("i", b::add(b::var("i"), b::num(1))))),
                                b::if_(b::eq(b::var("s"), b::num(0)), b::assign("s", b::num(1)),
                                       b::assign("s", b::num(2))))))});

    c.push_back({"dangling else binds to the nearest if",
                 in_f({KW_IF, LPAREN, IDENT("r"), LT, NUMBER("1"), RPAREN,
                       KW_IF, LPAREN, IDENT("r"), LT, NUMBER("2"), RPAREN, IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON,
                       KW_ELSE, IDENT("r"), ASSIGN, NUMBER("2"), SEMICOLON}),
                 module_f(b::if_(b::lt(b::var("r"), b::num(1)),
                                 b::if_(b::lt(b::var("r"), b::num(2)), b::assign("r", b::num(1)),
                                        b::assign("r", b::num(2)))))});

    c.push_back({"indexing, tuple assignment, assert, assume",
                 in_f({IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, LBRACKET, IDENT("j"), RBRACKET, ASSIGN,
                       IDENT("a"), LBRACKET, IDENT("j"), RBRACKET, LBRACKET, IDENT("i"), RBRACKET, SEMICOLON,
                       IDENT("x"), COMMA, IDENT("y"), ASSIGN, IDENT("g"), LPAREN, IDENT("x"), COMMA, NUMBER("1"),
                       RPAREN, SEMICOLON,
                       KW_ASSERT, KW_TRUE, SEMICOLON,
                       KW_ASSUME, IDENT("r"), GE, NUMBER("0"), SEMICOLON}),
                 module_f(b::assign_at("a", b::idx(b::idx(b::var("a"), b::var("j")), b::var("i")), b::var("i"), b::var("j")),
                          b::call_assign({"x", "y"}, "g", b::var("x"), b::num(1)),
                          b::assert_(b::boolean(true)),
                          b::assume_(b::ge(b::var("r"), b::num(0))))});

    c.push_back({"quantifiers and length",
                 in_f({KW_ASSERT, KW_FORALL, LPAREN, IDENT("i"), COLON, KW_INT, PIPE, IDENT("i"), GE, NUMBER("0"), ARROW,
                       IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, GE, NUMBER("0"), RPAREN, SEMICOLON,
                       KW_ASSERT, KW_EXISTS, LPAREN, IDENT("j"), COLON, KW_INT, LBRACKET, RBRACKET, PIPE,
                       KW_LENGTH, LPAREN, IDENT("j"), RPAREN, EQ, NUMBER("1"), RPAREN, SEMICOLON}),
                 module_f(b::assert_(b::forall("i", b::Int,
                                               b::implies(b::ge(b::var("i"), b::num(0)),
                                                          b::ge(b::idx(b::var("a"), b::var("i")), b::num(0))))),
                          b::assert_(b::exists("j", b::IntArray,
                                               b::eq(b::call("length", b::var("j")), b::num(1)))))});

    c.push_back({"implication is right-associative",
                 in_f({KW_ASSERT, IDENT("p"), LPAREN, NUMBER("1"), RPAREN, ARROW, IDENT("q"), LPAREN, NUMBER("2"),
                       RPAREN, ARROW, IDENT("s"), LPAREN, NUMBER("3"), RPAREN, SEMICOLON}),
                 module_f(b::assert_(b::implies(b::call("p", b::num(1)),
                                                b::implies(b::call("q", b::num(2)), b::call("s", b::num(3))))))});

    c.push_back({"logical precedence: not > and > or > ->",
                 in_f({KW_ASSERT, KW_NOT, IDENT("a"), LT, IDENT("b"), KW_AND, IDENT("c"), LT, IDENT("d"), KW_OR,
                       IDENT("e"), LT, IDENT("f"), ARROW, IDENT("g"), LT, IDENT("h"), SEMICOLON}),
                 module_f(b::assert_(b::implies(
                     b::or_(b::and_(b::not_(b::lt(b::var("a"), b::var("b"))), b::lt(b::var("c"), b::var("d"))),
                            b::lt(b::var("e"), b::var("f"))),
                     b::lt(b::var("g"), b::var("h")))))});

    c.push_back({"unary minus, left associativity, parentheses",
                 in_f({IDENT("r"), ASSIGN, MINUS, MINUS, NUMBER("3"), MINUS, MINUS, IDENT("y"), SEMICOLON,
                       IDENT("r"), ASSIGN, NUMBER("8"), SLASH, NUMBER("2"), SLASH, NUMBER("2"), SEMICOLON,
                       IDENT("r"), ASSIGN, LPAREN, NUMBER("1"), PLUS, NUMBER("2"), RPAREN, STAR, NUMBER("3"), SEMICOLON}),
                 module_f(b::assign("r", b::sub(b::neg(b::neg(b::num(3))), b::neg(b::var("y")))),
                          b::assign("r", b::divide(b::divide(b::num(8), b::num(2)), b::num(2))),
                          b::assign("r", b::mul(b::add(b::num(1), b::num(2)), b::num(3))))});

    c.push_back({"parenthesised arithmetic and parenthesised condition",
                 in_f({KW_ASSERT, LPAREN, IDENT("a"), PLUS, IDENT("b"), RPAREN, LT, IDENT("c"), SEMICOLON,
                       KW_ASSERT, LPAREN, IDENT("a"), LT, IDENT("b"), RPAREN, SEMICOLON}),
                 module_f(b::assert_(b::lt(b::add(b::var("a"), b::var("b")), b::var("c"))),
                          b::assert_(b::lt(b::var("a"), b::var("b"))))});

    c.push_back({"all six comparisons",
                 in_f({KW_ASSERT, IDENT("a"), EQ, IDENT("b"), SEMICOLON, KW_ASSERT, IDENT("a"), NE, IDENT("b"), SEMICOLON,
                       KW_ASSERT, IDENT("a"), LT, IDENT("b"), SEMICOLON, KW_ASSERT, IDENT("a"), LE, IDENT("b"), SEMICOLON,
                       KW_ASSERT, IDENT("a"), GT, IDENT("b"), SEMICOLON, KW_ASSERT, IDENT("a"), GE, IDENT("b"), SEMICOLON}),
                 module_f(b::assert_(b::eq(b::var("a"), b::var("b"))), b::assert_(b::ne(b::var("a"), b::var("b"))),
                          b::assert_(b::lt(b::var("a"), b::var("b"))), b::assert_(b::le(b::var("a"), b::var("b"))),
                          b::assert_(b::gt(b::var("a"), b::var("b"))), b::assert_(b::ge(b::var("a"), b::var("b"))))});

    c.push_back({"nested calls",
                 in_f({IDENT("r"), ASSIGN, IDENT("f"), LPAREN, IDENT("g"), LPAREN, RPAREN, COMMA, IDENT("h"), LPAREN,
                       NUMBER("1"), COMMA, NUMBER("2"), RPAREN, RPAREN, SEMICOLON}),
                 module_f(b::assign("r", b::call("f", b::call("g"), b::call("h", b::num(1), b::num(2)))))});

    c.push_back({"boolean constants",
                 in_f({KW_ASSERT, KW_TRUE, KW_AND, KW_FALSE, SEMICOLON, KW_ASSERT, KW_NOT, KW_TRUE, SEMICOLON}),
                 module_f(b::assert_(b::and_(b::boolean(true), b::boolean(false))), b::assert_(b::not_(b::boolean(true))))});

    c.push_back({"length in a loop condition",
                 in_f({KW_WHILE, LPAREN, IDENT("i"), LT, KW_LENGTH, LPAREN, IDENT("a"), RPAREN, RPAREN, LBRACE, RBRACE}),
                 module_f(b::while_(b::lt(b::var("i"), b::call("length", b::var("a"))), nullptr, b::block()))});

    c.push_back({"formula with a quantifier",
                 {IDENT("all_pos"), LPAREN, IDENT("a"), COLON, KW_INT, LBRACKET, RBRACKET, RPAREN, FAT_ARROW, KW_FORALL,
                  LPAREN, IDENT("i"), COLON, KW_INT, PIPE, IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, GT, NUMBER("0"), RPAREN},
                 b::mod(b::formula("all_pos", {b::param("a", b::IntArray)},
                                   b::forall("i", b::Int, b::gt(b::idx(b::var("a"), b::var("i")), b::num(0)))))});

    c.push_back({"formulas and functions (with and without the keyword) in one module",
                 {IDENT("p"), LPAREN, IDENT("x"), COLON, KW_INT, RPAREN, FAT_ARROW, IDENT("x"), GT, NUMBER("0"), KW_AND,
                  IDENT("x"), LT, NUMBER("10"),
                  IDENT("q"), LPAREN, RPAREN, FAT_ARROW, KW_TRUE,
                  KW_FUNCTION, IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE,
                  IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON, RBRACE,
                  IDENT("g"), LPAREN, RPAREN, KW_RETURNS, IDENT("s"), COLON, KW_INT, LBRACE,
                  IDENT("s"), ASSIGN, NUMBER("2"), SEMICOLON, RBRACE},
                 b::mod(b::formula("p", {b::param("x", b::Int)},
                                   b::and_(b::gt(b::var("x"), b::num(0)), b::lt(b::var("x"), b::num(10)))),
                        b::formula("q", {}, b::boolean(true)),
                        b::fn("f", {}, {b::param("r", b::Int)}).body(b::block(b::assign("r", b::num(1)))),
                        b::fn("g", {}, {b::param("s", b::Int)}).body(b::block(b::assign("s", b::num(2)))))});

    c.push_back({"example: minmax and normalize",
                 {IDENT("minmax"), LPAREN, IDENT("a"), COLON, KW_INT, LBRACKET, RBRACKET, RPAREN, KW_REQUIRES,
                  KW_LENGTH, LPAREN, IDENT("a"), RPAREN, GT, NUMBER("0"), KW_RETURNS, IDENT("mn"), COLON, KW_INT, COMMA,
                  IDENT("mx"), COLON, KW_INT, LBRACE,
                  IDENT("mn"), ASSIGN, IDENT("a"), LBRACKET, NUMBER("0"), RBRACKET, SEMICOLON,
                  IDENT("mx"), ASSIGN, IDENT("a"), LBRACKET, NUMBER("0"), RBRACKET, SEMICOLON,
                  IDENT("i"), ASSIGN, NUMBER("1"), SEMICOLON,
                  KW_WHILE, LPAREN, IDENT("i"), LT, KW_LENGTH, LPAREN, IDENT("a"), RPAREN, RPAREN, LBRACE,
                  KW_IF, LPAREN, IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, LT, IDENT("mn"), RPAREN,
                  IDENT("mn"), ASSIGN, IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, SEMICOLON,
                  KW_IF, LPAREN, IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, GT, IDENT("mx"), RPAREN,
                  IDENT("mx"), ASSIGN, IDENT("a"), LBRACKET, IDENT("i"), RBRACKET, SEMICOLON,
                  IDENT("i"), ASSIGN, IDENT("i"), PLUS, NUMBER("1"), SEMICOLON,
                  RBRACE, RBRACE,
                  IDENT("normalize"), LPAREN, IDENT("a"), COLON, KW_INT, LBRACKET, RBRACKET, RPAREN, KW_REQUIRES,
                  KW_LENGTH, LPAREN, IDENT("a"), RPAREN, GE, NUMBER("2"), KW_RETURNS, IDENT("b"), COLON, KW_INT,
                  LBRACKET, RBRACKET, LBRACE,
                  IDENT("b"), ASSIGN, IDENT("a"), SEMICOLON,
                  IDENT("mn"), COMMA, IDENT("mx"), ASSIGN, IDENT("minmax"), LPAREN, IDENT("b"), RPAREN, SEMICOLON,
                  KW_IF, LPAREN, IDENT("mx"), NE, IDENT("mn"), RPAREN, LBRACE,
                  IDENT("b"), LBRACKET, NUMBER("0"), RBRACKET, ASSIGN, IDENT("mn"), SEMICOLON,
                  IDENT("b"), LBRACKET, NUMBER("1"), RBRACKET, ASSIGN, IDENT("mx"), SEMICOLON,
                  RBRACE, RBRACE},
                 b::mod(
                     b::fn("minmax", {b::param("a", b::IntArray)}, {b::param("mn", b::Int), b::param("mx", b::Int)})
                         .requires_(b::gt(b::call("length", b::var("a")), b::num(0)))
                         .body(b::block(
                             b::assign("mn", b::idx(b::var("a"), b::num(0))),
                             b::assign("mx", b::idx(b::var("a"), b::num(0))),
                             b::assign("i", b::num(1)),
                             b::while_(b::lt(b::var("i"), b::call("length", b::var("a"))), nullptr,
                                       b::block(b::if_(b::lt(b::idx(b::var("a"), b::var("i")), b::var("mn")),
                                                       b::assign("mn", b::idx(b::var("a"), b::var("i")))),
                                                b::if_(b::gt(b::idx(b::var("a"), b::var("i")), b::var("mx")),
                                                       b::assign("mx", b::idx(b::var("a"), b::var("i")))),
                                                b::assign("i", b::add(b::var("i"), b::num(1))))))),
                     b::fn("normalize", {b::param("a", b::IntArray)}, {b::param("b", b::IntArray)})
                         .requires_(b::ge(b::call("length", b::var("a")), b::num(2)))
                         .body(b::block(
                             b::assign("b", b::var("a")),
                             b::call_assign({"mn", "mx"}, "minmax", b::var("b")),
                             b::if_(b::ne(b::var("mx"), b::var("mn")),
                                    b::block(b::assign_at("b", b::var("mn"), b::num(0)),
                                             b::assign_at("b", b::var("mx"), b::num(1)))))))});
    return c;
}

inline std::vector<ErrorCase> error_cases() {
    using namespace tk;
    std::vector<ErrorCase> c;

    c.push_back({"empty input", {HERE}, {"expected a function or formula definition, got end of input"}, b::mod()});
    c.push_back({"extra token after the program",
                 cat(in_f({IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON}), {HERE, RBRACE}),
                 {"expected a function or formula definition, got '}'"}, module_f(b::assign("r", b::num(1)))});
    c.push_back({"statement at the top level",
                 cat(in_f({IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON}), {IDENT("r"), HERE, ASSIGN, NUMBER("2"), SEMICOLON}),
                 {"expected '(', got '='"}, module_f(b::assign("r", b::num(1)))});

    c.push_back({"missing 'returns'", {KW_FUNCTION, IDENT("f"), LPAREN, RPAREN, HERE, LBRACE, RBRACE},
                 {"expected 'returns', got '{'"}, b::mod()});
    c.push_back({"unknown parameter type",
                 {KW_FUNCTION, IDENT("f"), LPAREN, IDENT("a"), COLON, HERE, IDENT("bool"), RPAREN, KW_RETURNS,
                  IDENT("r"), COLON, KW_INT, LBRACE, RBRACE},
                 {"expected a type ('int' or 'int[]'), got 'bool'"}, b::mod()});
    c.push_back({"unclosed type brackets",
                 {KW_FUNCTION, IDENT("f"), LPAREN, IDENT("a"), COLON, KW_INT, LBRACKET, HERE, RPAREN, KW_RETURNS,
                  IDENT("r"), COLON, KW_INT, LBRACE, RBRACE},
                 {"expected ']', got ')'"}, b::mod()});
    c.push_back({"function without a name",
                 {KW_FUNCTION, HERE, LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE, RBRACE},
                 {"expected a function name, got '('"}, b::mod()});
    c.push_back({"formula without '=>'",
                 {IDENT("p"), LPAREN, IDENT("x"), COLON, KW_INT, RPAREN, HERE, IDENT("x"), GT, NUMBER("0")},
                 {"expected '=>', 'requires' or 'returns', got 'x'"}, b::mod()});

    c.push_back({"unclosed parenthesis",
                 in_f({IDENT("r"), ASSIGN, LPAREN, NUMBER("1"), PLUS, NUMBER("2"), HERE, SEMICOLON}),
                 {"expected ')', got ';'"}, module_f(b::err_stmt())});
    c.push_back({"missing operand", in_f({IDENT("r"), ASSIGN, NUMBER("1"), PLUS, HERE, SEMICOLON}),
                 {"expected an expression, got ';'"}, module_f(b::err_stmt())});
    c.push_back({"missing semicolon", in_f({IDENT("r"), ASSIGN, NUMBER("1"), HERE}),
                 {"expected ';', got '}'"}, module_f(b::err_stmt())});
    c.push_back({"unclosed brace at the end of input",
                 {KW_FUNCTION, IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE, IDENT("r"),
                  ASSIGN, NUMBER("1"), SEMICOLON, HERE},
                 {"expected '}', got end of input"}, module_f(b::assign("r", b::num(1)))});
    c.push_back({"chained comparison",
                 in_f({KW_ASSERT, NUMBER("1"), LT, NUMBER("2"), HERE, LT, NUMBER("3"), SEMICOLON}),
                 {"comparison operators cannot be chained"}, module_f(b::err_stmt())});
    c.push_back({"unclosed index",
                 in_f({IDENT("a"), LBRACKET, NUMBER("1"), HERE, ASSIGN, NUMBER("2"), SEMICOLON}),
                 {"expected ']', got '='"}, module_f(b::err_stmt())});
    c.push_back({"missing ')' after while condition",
                 in_f({KW_WHILE, LPAREN, IDENT("r"), LT, NUMBER("1"), HERE, IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON}),
                 {"expected ')', got 'r'"}, module_f(b::err_stmt())});
    c.push_back({"not a statement",
                 in_f({HERE, LPAREN, IDENT("r"), RPAREN, ASSIGN, NUMBER("1"), SEMICOLON}),
                 {"expected a statement, got '('"}, module_f(b::err_stmt())});
    c.push_back({"call used as a statement",
                 in_f({KW_IF, LPAREN, IDENT("r"), LT, NUMBER("1"), RPAREN, IDENT("g"), HERE, LPAREN, NUMBER("1"), RPAREN, SEMICOLON}),
                 {"calls are only allowed on the right side of an assignment"},
                 module_f(b::if_(b::lt(b::var("r"), b::num(1)), b::err_stmt()))});
    c.push_back({"integer literal is too large",
                 in_f({IDENT("r"), ASSIGN, HERE, NUMBER("99999999999999999999"), SEMICOLON}),
                 {"integer literal is too large"}, module_f(b::assign("r", b::err_expr()))});

    c.push_back({"number as a condition",
                 in_f({KW_IF, LPAREN, HERE, NUMBER("1"), RPAREN, IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON}),
                 {"expected a condition, found a number"}, module_f(b::if_(b::num(1), b::assign("r", b::num(1))))});
    c.push_back({"comparison as an arithmetic value",
                 in_f({IDENT("r"), ASSIGN, HERE, NUMBER("1"), LT, NUMBER("2"), SEMICOLON}),
                 {"expected an arithmetic expression, found a comparison"},
                 module_f(b::assign("r", b::lt(b::num(1), b::num(2))))});
    c.push_back({"arithmetic as a predicate",
                 in_f({KW_ASSERT, HERE, IDENT("r"), PLUS, NUMBER("1"), SEMICOLON}),
                 {"expected a predicate, found an arithmetic expression"},
                 module_f(b::assert_(b::add(b::var("r"), b::num(1))))});
    c.push_back({"multiple assignment from a non-call",
                 in_f({IDENT("x"), COMMA, IDENT("y"), ASSIGN, HERE, NUMBER("1"), SEMICOLON}),
                 {"expected a function call on the right side of a multiple assignment"}, module_f(b::err_stmt())});

    c.push_back({"two broken statements, parsing continues",
                 in_f({IDENT("x"), ASSIGN, HERE, SEMICOLON, IDENT("y"), ASSIGN, HERE, RPAREN, SEMICOLON,
                       IDENT("z"), ASSIGN, NUMBER("1"), SEMICOLON}),
                 {"expected an expression, got ';'", "expected an expression, got ')'"},
                 module_f(b::err_stmt(), b::err_stmt(), b::assign("z", b::num(1)))});
    c.push_back({"error in the then-branch, else-branch survives",
                 in_f({KW_IF, LPAREN, IDENT("r"), LT, NUMBER("1"), RPAREN, IDENT("r"), ASSIGN, HERE, SEMICOLON,
                       KW_ELSE, IDENT("r"), ASSIGN, NUMBER("2"), SEMICOLON}),
                 {"expected an expression, got ';'"},
                 module_f(b::if_(b::lt(b::var("r"), b::num(1)), b::err_stmt(), b::assign("r", b::num(2))))});
    c.push_back({"missing '}' before the next function (with keyword)",
                 {KW_FUNCTION, IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE,
                  IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON, HERE,
                  KW_FUNCTION, IDENT("g"), LPAREN, RPAREN, KW_RETURNS, IDENT("s"), COLON, KW_INT, LBRACE,
                  IDENT("s"), ASSIGN, NUMBER("2"), SEMICOLON, RBRACE},
                 {"expected '}', got 'function'"},
                 b::mod(b::fn("f", {}, {b::param("r", b::Int)}).body(b::block(b::assign("r", b::num(1)))),
                        b::fn("g", {}, {b::param("s", b::Int)}).body(b::block(b::assign("s", b::num(2)))))});
    c.push_back({"missing '}' before the next function (without keyword)",
                 {IDENT("f"), LPAREN, RPAREN, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE,
                  IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON, HERE,
                  IDENT("g"), LPAREN, RPAREN, KW_RETURNS, IDENT("s"), COLON, KW_INT, LBRACE,
                  IDENT("s"), ASSIGN, NUMBER("2"), SEMICOLON, RBRACE},
                 {"expected '}', got 'g'"},
                 b::mod(b::fn("f", {}, {b::param("r", b::Int)}).body(b::block(b::assign("r", b::num(1)))),
                        b::fn("g", {}, {b::param("s", b::Int)}).body(b::block(b::assign("s", b::num(2)))))});
    c.push_back({"broken header, the next function (without keyword) survives",
                 {KW_FUNCTION, IDENT("f"), LPAREN, HERE, KW_RETURNS, IDENT("r"), COLON, KW_INT, LBRACE,
                  IDENT("r"), ASSIGN, NUMBER("1"), SEMICOLON, RBRACE,
                  IDENT("g"), LPAREN, RPAREN, KW_RETURNS, IDENT("s"), COLON, KW_INT, LBRACE,
                  IDENT("s"), ASSIGN, NUMBER("2"), SEMICOLON, RBRACE},
                 {"expected an identifier, got 'returns'"},
                 b::mod(b::fn("g", {}, {b::param("s", b::Int)}).body(b::block(b::assign("s", b::num(2)))))});

    c.push_back({"example: missing ';' after the tuple assignment",
                 {IDENT("normalize"), LPAREN, IDENT("a"), COLON, KW_INT, LBRACKET, RBRACKET, RPAREN, KW_REQUIRES,
                  KW_LENGTH, LPAREN, IDENT("a"), RPAREN, GE, NUMBER("2"), KW_RETURNS, IDENT("b"), COLON, KW_INT,
                  LBRACKET, RBRACKET, LBRACE,
                  IDENT("b"), ASSIGN, IDENT("a"), SEMICOLON,
                  IDENT("mn"), COMMA, IDENT("mx"), ASSIGN, IDENT("minmax"), LPAREN, IDENT("b"), RPAREN, HERE,
                  KW_IF, LPAREN, IDENT("mx"), NE, IDENT("mn"), RPAREN, LBRACE,
                  IDENT("b"), LBRACKET, NUMBER("0"), RBRACKET, ASSIGN, IDENT("mn"), SEMICOLON,
                  IDENT("b"), LBRACKET, NUMBER("1"), RBRACKET, ASSIGN, IDENT("mx"), SEMICOLON,
                  RBRACE, RBRACE},
                 {"expected ';', got 'if'"},
                 b::mod(b::fn("normalize", {b::param("a", b::IntArray)}, {b::param("b", b::IntArray)})
                            .requires_(b::ge(b::call("length", b::var("a")), b::num(2)))
                            .body(b::block(b::assign("b", b::var("a")), b::err_stmt(),
                                           b::if_(b::ne(b::var("mx"), b::var("mn")),
                                                  b::block(b::assign_at("b", b::var("mn"), b::num(0)),
                                                           b::assign_at("b", b::var("mx"), b::num(1)))))))});
    return c;
}

}  // namespace funny::parser_tests