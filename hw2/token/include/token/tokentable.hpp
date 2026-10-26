#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "token/token.hpp"

namespace funny::token {

struct TokenError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct TokenInfo {
    std::string name;
    bool skip = false;
};

class TokenTable {
public:
    TokenTable() = default;

    explicit TokenTable(std::vector<TokenInfo> tokens);

    template <class Rules>
    static TokenTable from_rules(const Rules& rules) {
        std::vector<TokenInfo> infos;
        infos.reserve(rules.size());
        for (const auto& r : rules) infos.push_back(TokenInfo{r.name, r.skip});
        return TokenTable(std::move(infos));
    }

    std::size_t size() const { return infos_.size(); }

    const TokenInfo& info(Kind kind) const;
    const std::string& name(Kind kind) const { return info(kind).name; }
    bool skip(Kind kind) const { return info(kind).skip; }

    std::optional<Kind> find(std::string_view name) const;

    Kind require(std::string_view name) const;

private:
    std::vector<TokenInfo> infos_;
    std::unordered_map<std::string, Kind> by_name_;
};

std::string describe(const Token& token, const TokenTable& table);

}  // namespace funny::token