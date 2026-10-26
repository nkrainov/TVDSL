#include "token/tokentable.hpp"

namespace funny::token {

TokenTable::TokenTable(std::vector<TokenInfo> tokens) : infos_(std::move(tokens)) {
    for (std::size_t i = 0; i < infos_.size(); ++i) {
        const std::string& name = infos_[i].name;
        if (name.empty()) throw TokenError("token " + std::to_string(i) + " has an empty name");
        if (!by_name_.emplace(name, static_cast<Kind>(i)).second)
            throw TokenError("duplicate token name '" + name + "'");
    }
}

const TokenInfo& TokenTable::info(Kind kind) const {
    if (kind < 0 || static_cast<std::size_t>(kind) >= infos_.size())
        throw std::out_of_range("token kind " + std::to_string(kind) + " is not in the token table");
    return infos_[static_cast<std::size_t>(kind)];
}

std::optional<Kind> TokenTable::find(std::string_view name) const {
    auto it = by_name_.find(std::string(name));
    if (it == by_name_.end()) return std::nullopt;
    return it->second;
}

Kind TokenTable::require(std::string_view name) const {
    if (auto kind = find(name)) return *kind;
    throw TokenError("token '" + std::string(name) + "' is not defined in the token table");
}

std::string describe(const Token& token, const TokenTable& table) {
    return table.name(token.kind) + "(" + std::string(token.lexeme) + ")";
}

}  // namespace funny::token