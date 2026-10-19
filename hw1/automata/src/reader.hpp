#pragma once

// Приватный помощник для разбора текстовых форматов: слова и числа через пробельные символы.
#include <sstream>
#include <string>
#include <string_view>

namespace funny::automata::detail {

template <class Error>
class Reader {
public:
    explicit Reader(std::string_view text) : in_(std::string(text)) {}

    void expect(const char* word) {
        std::string w;
        if (!(in_ >> w) || w != word) throw Error(std::string("expected '") + word + "', got '" + w + "'");
    }
    std::string word(const char* what) {
        std::string w;
        if (!(in_ >> w)) throw Error(std::string("unexpected end while reading ") + what);
        return w;
    }
    long long integer(const char* what) {
        long long v;
        if (!(in_ >> v)) throw Error(std::string("expected integer for ") + what);
        return v;
    }
    long long keyed(const char* key) {
        expect(key);
        return integer(key);
    }
    bool at_end() {
        in_ >> std::ws;
        return in_.peek() == std::istringstream::traits_type::eof();
    }

private:
    std::istringstream in_;
};

}  // namespace funny::automata::detail