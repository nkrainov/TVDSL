#include "lexspec/ast.hpp"

#include <cstdio>

namespace funny::lexspec {

NodePtr make_set(const CharSet& set) {
    auto n = std::make_unique<Node>();
    n->kind = Node::Kind::Set;
    n->set = set;
    return n;
}

NodePtr make_char(unsigned char c) {
    CharSet s;
    s.set(c);
    return make_set(s);
}

static NodePtr make_nary(Node::Kind kind, std::vector<NodePtr> items) {
    if (items.size() == 1) return std::move(items.front());
    auto n = std::make_unique<Node>();
    n->kind = kind;
    for (auto& item : items) {
        if (item->kind == kind) {  // склеиваем вложенные Concat/Alt
            for (auto& c : item->children) n->children.push_back(std::move(c));
        } else {
            n->children.push_back(std::move(item));
        }
    }
    return n;
}

NodePtr make_concat(std::vector<NodePtr> items) { return make_nary(Node::Kind::Concat, std::move(items)); }
NodePtr make_alt(std::vector<NodePtr> items) { return make_nary(Node::Kind::Alt, std::move(items)); }

NodePtr make_unary(Node::Kind kind, NodePtr child) {
    auto n = std::make_unique<Node>();
    n->kind = kind;
    n->children.push_back(std::move(child));
    return n;
}

bool nullable(const Node& node) {
    using K = Node::Kind;
    switch (node.kind) {
        case K::Set: return false;
        case K::Concat:
            for (const auto& c : node.children)
                if (!nullable(*c)) return false;
            return true;
        case K::Alt:
            for (const auto& c : node.children)
                if (nullable(*c)) return true;
            return false;
        case K::Star:
        case K::Optional: return true;
        case K::Plus: return nullable(*node.children[0]);
    }
    return false;
}

namespace {

std::string ch(int c) {
    if (c > 32 && c < 127) {
        switch (c) {
            case '\\': case '[': case ']': case '-': case '^': case '\'':
                return std::string("\\") + static_cast<char>(c);
            default:
                return std::string(1, static_cast<char>(c));
        }
    }
    char buf[8];
    std::snprintf(buf, sizeof buf, "\\x%02x", c);
    return buf;
}

std::string set_to_string(const CharSet& s) {
    if (s.count() == 1) {
        for (std::size_t i = 0; i < kAsciiSize; ++i)
            if (s[i]) return "'" + ch(static_cast<int>(i)) + "'";
    }
    std::string out = "[";
    std::size_t i = 0;
    while (i < kAsciiSize) {
        if (!s[i]) { ++i; continue; }
        std::size_t j = i;
        while (j + 1 < kAsciiSize && s[j + 1]) ++j;
        out += ch(static_cast<int>(i));
        if (j > i) out += "-" + ch(static_cast<int>(j));
        i = j + 1;
    }
    return out + "]";
}

}  // namespace

std::string to_string(const Node& node) {
    using K = Node::Kind;
    if (node.kind == K::Set) return set_to_string(node.set);
    const char* tag = "";
    switch (node.kind) {
        case K::Concat: tag = "cat"; break;
        case K::Alt: tag = "alt"; break;
        case K::Star: tag = "*"; break;
        case K::Plus: tag = "+"; break;
        case K::Optional: tag = "?"; break;
        case K::Set: break;
    }
    std::string out = std::string("(") + tag;
    for (const auto& c : node.children) out += " " + to_string(*c);
    return out + ")";
}

}  // namespace funny::lexspec