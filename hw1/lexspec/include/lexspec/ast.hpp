#pragma once

#include <bitset>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace funny::lexspec {

inline constexpr std::size_t kAsciiSize = 128;
using CharSet = std::bitset<kAsciiSize>;  // подмножество ASCII 0..127

struct Node;
using NodePtr = std::unique_ptr<Node>;

// AST регулярного выражения.
//   Set            : один символ из множества `set` (литерал "a" это Set из одного символа)
//   Concat, Alt    : >= 2 потомков
//   Star/Plus/Optional : ровно 1 потомок
struct Node {
    enum class Kind { Set, Concat, Alt, Star, Plus, Optional };

    Kind kind = Kind::Set;
    CharSet set;
    std::vector<NodePtr> children;
};

NodePtr make_set(const CharSet& set);
NodePtr make_char(unsigned char c);
// Для Concat/Alt: при одном элементе возвращает его самого.
NodePtr make_concat(std::vector<NodePtr> items);
NodePtr make_alt(std::vector<NodePtr> items);
NodePtr make_unary(Node::Kind kind, NodePtr child);

// Принимает ли выражение пустую строку.
bool nullable(const Node& node);

// S-выражение для отладки и snapshot-тестов, например:
//   (cat [A-Z_a-z] (* [0-9A-Z_a-z]))
std::string to_string(const Node& node);

}  // namespace funny::lexspec