
#include <iostream>
#include <string>

#include "ast/equal.hpp"
#include "ast/print.hpp"
#include "parser_cases.hpp"
#include "parser/parser.hpp"

using namespace funny;
using namespace funny::parser_tests;

namespace {

int failures = 0;
bool verbose = false;

void fail(const std::string& name, const std::string& details) {
    ++failures;
    std::cout << "FAIL " << name << "\n" << details << "\n";
}

struct Prepared {
    Tokens clean;                       
    std::vector<std::size_t> marks;     
    std::vector<token::Token> tokens;
};

void prepare(const Tokens& raw, const token::TokenTable& table, Prepared& out) {
    for (const Tok& t : raw) {
        if (t.kind.empty()) out.marks.push_back(out.clean.size());
        else out.clean.push_back(t);
    }
    std::size_t offset = 0;
    for (const Tok& t : out.clean) {
        out.tokens.push_back(token::Token{table.require(t.kind), std::string_view(t.lexeme), offset, 1, offset + 1});
        offset += t.lexeme.size() + 1;
    }
}

std::pair<std::size_t, std::size_t> expected_position(const Prepared& p, std::size_t index) {
    if (index < p.tokens.size()) return {p.tokens[index].line, p.tokens[index].column};
    if (p.tokens.empty()) return {1, 1};
    const token::Token& last = p.tokens.back();
    return {last.line, last.column + last.lexeme.size()};
}

std::string errors_to_string(const std::vector<parser::Diagnostic>& errors) {
    if (errors.empty()) return "  (no errors)\n";
    std::string s;
    for (const auto& e : errors) s += "  " + parser::to_string(e) + "\n";
    return s;
}

std::string tree_diff(const ast::Module& expected, const ast::Module& got) {
    return "--- expected tree\n" + ast::to_string(expected) + "--- got tree\n" + ast::to_string(got);
}

void run_ok(const parser::Parser& parser, const token::TokenTable& table, const OkCase& c) {
    Prepared p;
    prepare(c.tokens, table, p);
    if (!p.marks.empty()) return fail(c.name, "test bug: HERE marker in a positive case");

    parser::ParseResult r = parser.parse(p.tokens);
    if (!r.ok()) return fail(c.name, "unexpected errors:\n" + errors_to_string(r.errors));
    if (!ast::equal(*r.module, c.expected)) return fail(c.name, tree_diff(c.expected, *r.module));
    if (verbose) std::cout << "ok   " << c.name << "\n";
}

void run_error(const parser::Parser& parser, const token::TokenTable& table, const ErrorCase& c) {
    Prepared p;
    prepare(c.tokens, table, p);
    if (p.marks.size() != c.messages.size())
        return fail(c.name, "test bug: " + std::to_string(p.marks.size()) + " HERE markers, " +
                                std::to_string(c.messages.size()) + " messages");

    parser::ParseResult r = parser.parse(p.tokens);

    std::string expected;
    bool same = r.errors.size() == c.messages.size();
    for (std::size_t i = 0; i < c.messages.size(); ++i) {
        const auto pos = expected_position(p, p.marks[i]);
        parser::Diagnostic d{pos.first, pos.second, c.messages[i]};
        expected += "  " + parser::to_string(d) + "\n";
        if (i < r.errors.size() &&
            (r.errors[i].line != pos.first || r.errors[i].column != pos.second || r.errors[i].message != c.messages[i]))
            same = false;
    }
    if (!same) return fail(c.name, "expected errors:\n" + expected + "got errors:\n" + errors_to_string(r.errors));

    if (c.expected && !ast::equal(*r.module, *c.expected)) return fail(c.name, tree_diff(*c.expected, *r.module));
    if (verbose) std::cout << "ok   " << c.name << "\n";
}

void run_deep_nesting(const parser::Parser& parser, const token::TokenTable& table) {
    using namespace tk;
    Tokens body{IDENT("r"), ASSIGN};
    for (int i = 0; i < 5000; ++i) body.push_back(LPAREN);
    body.push_back(NUMBER("1"));
    body.push_back(SEMICOLON);

    Prepared p;
    prepare(in_f(body), table, p);
    parser::ParseResult r = parser.parse(p.tokens);
    bool found = false;
    for (const auto& e : r.errors) found = found || e.message == "nesting is too deep";
    if (!found || !r.module) fail("deeply nested parentheses", "errors:\n" + errors_to_string(r.errors));
    else if (verbose) std::cout << "ok   deeply nested parentheses\n";
}

}  // namespace

int main(int argc, char** argv) {
    verbose = argc > 1 && std::string(argv[1]) == "-v";

    const token::TokenTable table(token_infos());
    const parser::Parser parser(table);

    const auto oks = ok_cases();
    const auto errs = error_cases();
    for (const auto& c : oks) run_ok(parser, table, c);
    for (const auto& c : errs) run_error(parser, table, c);
    run_deep_nesting(parser, table);

    std::cout << oks.size() << " positive and " << errs.size() << " negative cases, "
              << (failures ? std::to_string(failures) + " failed" : "all passed") << "\n";
    return failures ? 1 : 0;
}