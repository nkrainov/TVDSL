#include "lexspec/parser.hpp"

#include <cctype>
#include <unordered_set>

namespace funny::lexspec {

RegexError::RegexError(std::size_t line, std::size_t column, std::string message)
    : std::runtime_error(std::to_string(line) + ":" + std::to_string(column) + ": " + message),
      line_(line),
      column_(column),
      message_(std::move(message)) {}

namespace {

bool is_ascii(unsigned char c) { return c < 128; }
bool is_ident_start(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool is_ident_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Разбирает регулярное выражение, начиная с позиции `pos` внутри строки `text`.
// Столбец ошибки = индекс в строке + 1, поэтому текст всей строки файла передаётся целиком.
class RegexParser {
public:
    RegexParser(std::string_view text, std::size_t line, std::size_t pos)
        : text_(text), line_(line), pos_(pos) {}

    NodePtr parse() {
        skip_ws();
        NodePtr n = parse_alt();
        skip_ws();
        if (!at_end()) {
            if (peek() == ')') fail("unmatched ')'");
            fail("unexpected character");
        }
        return n;
    }

private:
    std::string_view text_;
    std::size_t line_;
    std::size_t pos_;

    bool at_end() const { return pos_ >= text_.size(); }
    char peek() const { return text_[pos_]; }
    void skip_ws() {
        while (!at_end() && (peek() == ' ' || peek() == '\t')) ++pos_;
    }
    [[noreturn]] void fail(const std::string& msg) const { fail_at(pos_, msg); }
    [[noreturn]] void fail_at(std::size_t at, const std::string& msg) const {
        throw RegexError(line_, at + 1, msg);
    }

    NodePtr parse_alt() {
        std::vector<NodePtr> alts;
        alts.push_back(parse_cat());
        skip_ws();
        while (!at_end() && peek() == '|') {
            ++pos_;
            alts.push_back(parse_cat());
            skip_ws();
        }
        return make_alt(std::move(alts));
    }

    NodePtr parse_cat() {
        std::vector<NodePtr> items;
        while (true) {
            skip_ws();
            if (at_end() || peek() == '|' || peek() == ')') break;
            items.push_back(parse_rep());
        }
        if (items.empty()) fail("expected expression");
        return make_concat(std::move(items));
    }

    NodePtr parse_rep() {
        NodePtr n = parse_atom();
        while (true) {
            skip_ws();
            if (at_end()) break;
            Node::Kind k;
            switch (peek()) {
                case '*': k = Node::Kind::Star; break;
                case '+': k = Node::Kind::Plus; break;
                case '?': k = Node::Kind::Optional; break;
                default: return n;
            }
            ++pos_;
            n = make_unary(k, std::move(n));
        }
        return n;
    }

    NodePtr parse_atom() {
        char c = peek();
        if (!is_ascii(static_cast<unsigned char>(c))) fail("non-ASCII character");
        switch (c) {
            case '(': {
                ++pos_;
                NodePtr inner = parse_alt();
                skip_ws();
                if (at_end() || peek() != ')') fail("expected ')'");
                ++pos_;
                return inner;
            }
            case '[': return parse_class();
            case '"': return parse_string();
            case '\\': return make_char(read_escape());
            case '*':
            case '+':
            case '?': fail("nothing to repeat");
            default: break;
        }
        if (std::isalnum(static_cast<unsigned char>(c))) {
            ++pos_;
            return make_char(static_cast<unsigned char>(c));
        }
        fail(std::string("unexpected character '") + c + "'; put literals in quotes");
    }

    // pos_ указывает на '\\'. Возвращает код символа.
    unsigned char read_escape() {
        const std::size_t start = pos_;
        ++pos_;
        if (at_end()) fail_at(start, "dangling backslash");
        char c = text_[pos_++];
        switch (c) {
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            case 'x': {
                int hi = pos_ < text_.size() ? hex_value(text_[pos_]) : -1;
                int lo = pos_ + 1 < text_.size() ? hex_value(text_[pos_ + 1]) : -1;
                if (hi < 0 || lo < 0) fail_at(start, "\\x must be followed by two hex digits");
                pos_ += 2;
                int v = hi * 16 + lo;
                if (v >= 128) fail_at(start, "\\xHH must be below 0x80 (ASCII only)");
                return static_cast<unsigned char>(v);
            }
            default: break;
        }
        unsigned char u = static_cast<unsigned char>(c);
        if (is_ascii(u) && (std::ispunct(u) || c == ' ')) return u;
        fail_at(start, std::string("unknown escape '\\") + c + "'");
    }

    NodePtr parse_string() {
        const std::size_t start = pos_;
        ++pos_;
        std::vector<NodePtr> chars;
        while (true) {
            if (at_end()) fail_at(start, "unterminated string literal");
            char c = peek();
            if (c == '"') { ++pos_; break; }
            unsigned char v;
            if (c == '\\') {
                v = read_escape();
            } else {
                v = static_cast<unsigned char>(c);
                if (!is_ascii(v)) fail("non-ASCII character");
                ++pos_;
            }
            chars.push_back(make_char(v));
        }
        if (chars.empty()) fail_at(start, "empty string literal");
        return make_concat(std::move(chars));
    }

    unsigned char class_char() {
        if (peek() == '\\') return read_escape();
        unsigned char v = static_cast<unsigned char>(peek());
        if (!is_ascii(v)) fail("non-ASCII character");
        ++pos_;
        return v;
    }

    NodePtr parse_class() {
        const std::size_t start = pos_;
        ++pos_;
        bool negate = false;
        if (!at_end() && peek() == '^') { negate = true; ++pos_; }
        CharSet set;
        std::size_t items = 0;
        while (true) {
            if (at_end()) fail_at(start, "unterminated character class");
            if (peek() == ']') { ++pos_; break; }
            const std::size_t item_pos = pos_;
            unsigned char lo = class_char();
            if (pos_ + 1 < text_.size() && peek() == '-' && text_[pos_ + 1] != ']') {
                ++pos_;
                unsigned char hi = class_char();
                if (hi < lo) fail_at(item_pos, "invalid range: start is greater than end");
                for (unsigned c = lo; c <= hi; ++c) set.set(c);
            } else {
                set.set(lo);
            }
            ++items;
        }
        if (items == 0) fail_at(start, "empty character class");
        if (negate) set = ~set;  // в пределах ASCII 0..127
        if (set.none()) fail_at(start, "character class matches nothing");
        return make_set(set);
    }
};

}  // namespace

NodePtr parse_regex(std::string_view text) {
    return RegexParser(text, 1, 0).parse();
}

std::vector<TokenSpec> parse_token_file(std::string_view text) {
    std::vector<TokenSpec> specs;
    std::unordered_set<std::string> seen;

    std::size_t line_no = 0;
    std::size_t begin = 0;
    while (begin <= text.size()) {
        std::size_t end = text.find('\n', begin);
        if (end == std::string_view::npos) end = text.size();
        std::string_view line = text.substr(begin, end - begin);
        begin = end + 1;
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);

        std::size_t i = 0;
        auto skip_ws = [&] {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
        };
        auto fail = [&](std::size_t at, const std::string& msg) {
            throw RegexError(line_no, at + 1, msg);
        };
        auto read_ident = [&] {
            std::size_t s = i;
            while (i < line.size() && is_ident_char(line[i])) ++i;
            return std::string(line.substr(s, i - s));
        };

        skip_ws();
        if (i >= line.size() || line[i] == '#') continue;

        const std::size_t name_pos = i;
        if (!is_ident_start(line[i])) fail(i, "expected rule name");
        TokenSpec spec;
        spec.name = read_ident();
        skip_ws();
        if (i < line.size() && line[i] != ':') {
            const std::size_t word_pos = i;
            if (!is_ident_start(line[i])) fail(i, "expected 'skip' or ':'");
            if (read_ident() != "skip") fail(word_pos, "expected 'skip' or ':'");
            spec.skip = true;
            skip_ws();
        }
        if (i >= line.size() || line[i] != ':') fail(i, "expected ':'");
        ++i;
        skip_ws();
        const std::size_t regex_pos = i;
        if (i >= line.size()) fail(i, "expected regular expression");

        if (!seen.insert(spec.name).second) fail(name_pos, "duplicate rule name '" + spec.name + "'");

        spec.regex = RegexParser(line, line_no, i).parse();
        if (nullable(*spec.regex))
            fail(regex_pos, "rule '" + spec.name + "' accepts the empty string");
        specs.push_back(std::move(spec));
    }

    if (specs.empty()) throw RegexError(1, 1, "no token rules");
    return specs;
}

}  // namespace funny::lexspec