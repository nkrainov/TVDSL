#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ast/print.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "token/tokentable.hpp"

namespace {

bool read_file(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

void usage() { std::cerr << "usage: funny_parser <dfa-file> <source-file> [--positions]\n"; }

}  // namespace

int main(int argc, char** argv) {
    using namespace funny;

    std::string dfa_path, source_path;
    bool positions = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--positions") positions = true;
        else if (dfa_path.empty()) dfa_path = a;
        else if (source_path.empty()) source_path = a;
        else { usage(); return 2; }
    }
    if (dfa_path.empty() || source_path.empty()) { usage(); return 2; }

    std::string dfa_text, source;
    if (!read_file(dfa_path, dfa_text)) { std::cerr << "error: cannot read " << dfa_path << "\n"; return 2; }
    if (!read_file(source_path, source)) { std::cerr << "error: cannot read " << source_path << "\n"; return 2; }

    try {
        const lexer::Lexer lexer = lexer::Lexer::from_text(dfa_text);
        const token::TokenTable table = token::TokenTable::from_rules(lexer.dfa().rules());
        const parser::Parser parser(table);

        const lexer::LexResult lexed = lexer.tokenize(source);
        if (!lexed.ok()) {
            const auto& e = *lexed.error;
            std::cerr << source_path << ":" << e.line << ":" << e.column << ": error: " << e.message << "\n";
            return 1; 
        }

        const parser::ParseResult parsed = parser.parse(lexed.tokens);
        for (const auto& d : parsed.errors)
            std::cerr << source_path << ":" << d.line << ":" << d.column << ": error: " << d.message << "\n";

        std::cout << ast::to_string(*parsed.module, ast::PrintOptions{positions});
        if (!parsed.ok()) return 1;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}