#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "lexer/lexer.hpp"

namespace {

using namespace funny;

struct Case {
    std::size_t line = 0;
    std::string literal;
    std::string input;
    std::string expected;
};

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

enum class ParseResult { Case, Skip, Error };

ParseResult parse_case(const std::string& raw, std::size_t line_no, Case& out, std::string& err) {
    std::string line = raw;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= line.size() || line[i] == '#') return ParseResult::Skip;

    auto fail = [&](std::size_t at, const std::string& msg) {
        err = std::to_string(line_no) + ":" + std::to_string(at + 1) + ": " + msg;
        return ParseResult::Error;
    };

    if (line[i] != '"') return fail(i, "expected '\"' at the start of the input literal");
    const std::size_t lit_start = i;
    ++i;
    std::string input;
    while (true) {
        if (i >= line.size()) return fail(lit_start, "unterminated input literal");
        char c = line[i];
        if (c == '"') { ++i; break; }
        if (c != '\\') { input += c; ++i; continue; }

        const std::size_t esc = i;
        ++i;
        if (i >= line.size()) return fail(esc, "dangling backslash");
        char e = line[i++];
        switch (e) {
            case 'n': input += '\n'; break;
            case 'r': input += '\r'; break;
            case 't': input += '\t'; break;
            case '\\': input += '\\'; break;
            case '"': input += '"'; break;
            case 'x': {
                int hi = i < line.size() ? hex_value(line[i]) : -1;
                int lo = i + 1 < line.size() ? hex_value(line[i + 1]) : -1;
                if (hi < 0 || lo < 0) return fail(esc, "\\x must be followed by two hex digits");
                input += static_cast<char>(hi * 16 + lo);
                i += 2;
                break;
            }
            default: return fail(esc, std::string("unknown escape '\\") + e + "'");
        }
    }
    out.literal = line.substr(lit_start, i - lit_start);

    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (line.compare(i, 2, "=>") != 0) return fail(i, "expected '=>' after the input literal");
    i += 2;

    out.line = line_no;
    out.input = std::move(input);
    out.expected = trim(line.substr(i));
    return ParseResult::Case;
}

std::string render(const lexer::Lexer& lx, const std::string& input) {
    const lexer::LexResult res = lx.tokenize(input);
    std::string out;
    auto append = [&](const std::string& piece) {
        if (!out.empty()) out += ' ';
        out += piece;
    };
    for (const auto& t : res.tokens) append(lx.rule_name(t.rule) + "(" + std::string(t.lexeme) + ")");
    if (res.error) {
        const auto& e = *res.error;
        append("ERROR@" + std::to_string(e.offset) + " " + std::to_string(e.line) + ":" + std::to_string(e.column));
    }
    return out;
}

bool read_file(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

void usage() { std::cerr << "usage: funny_runner <dfa-file> <cases-file> [-q]\n"; }

}  // namespace

int main(int argc, char** argv) {
    std::string dfa_path, cases_path;
    bool quiet = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-q" || a == "--quiet") quiet = true;
        else if (dfa_path.empty()) dfa_path = a;
        else if (cases_path.empty()) cases_path = a;
        else { usage(); return 2; }
    }
    if (dfa_path.empty() || cases_path.empty()) { usage(); return 2; }

    std::string dfa_text, cases_text;
    if (!read_file(dfa_path, dfa_text)) { std::cerr << "error: cannot read " << dfa_path << "\n"; return 2; }
    if (!read_file(cases_path, cases_text)) { std::cerr << "error: cannot read " << cases_path << "\n"; return 2; }

    try {
        const lexer::Lexer lx = lexer::Lexer::from_text(dfa_text);

        std::size_t total = 0, passed = 0, format_errors = 0;
        std::istringstream in(cases_text);
        std::string raw;
        for (std::size_t line_no = 1; std::getline(in, raw); ++line_no) {
            Case c;
            std::string err;
            switch (parse_case(raw, line_no, c, err)) {
                case ParseResult::Skip: continue;
                case ParseResult::Error:
                    ++format_errors;
                    std::cout << "ERROR " << cases_path << ":" << err << "\n";
                    continue;
                case ParseResult::Case: break;
            }
            ++total;
            const std::string actual = render(lx, c.input);
            if (actual == c.expected) {
                ++passed;
                if (!quiet) std::cout << "ok    line " << c.line << ": " << c.literal << "\n";
            } else {
                std::cout << "FAIL  line " << c.line << ": " << c.literal << "\n"
                          << "        expected: " << c.expected << "\n"
                          << "        got:      " << actual << "\n";
            }
        }

        std::cout << passed << "/" << total << " passed";
        if (format_errors) std::cout << ", " << format_errors << " malformed case line(s)";
        std::cout << "\n";
        return (passed == total && format_errors == 0) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "error: invalid DFA file " << dfa_path << ": " << e.what() << "\n";
        return 2;
    }
}