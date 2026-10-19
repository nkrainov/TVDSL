#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "automata/dfa_serialization.hpp"
#include "automata/nfa_serialization.hpp"
#include "builder/minimize.hpp"
#include "builder/subset.hpp"
#include "builder/thompson.hpp"
#include "lexspec/parser.hpp"

namespace {

using namespace funny;

struct Options {
    std::string tokens_path;
    std::string out_dir = "out";
    bool verbose = false;
};

void print_usage(std::ostream& os) {
    os << "usage: funny_builder <tokens-file> [-o <out-dir>] [-v]\n"
          "  -o, --out DIR   output directory (default: out)\n"
          "  -v, --verbose   list every priority override (e.g. FUNCTION overrides IDENT)\n"
          "  -h, --help      show this help\n";
}

bool parse_args(int argc, char** argv, Options& opt, bool& help) {
    help = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            help = true;
        } else if (a == "-v" || a == "--verbose") {
            opt.verbose = true;
        } else if (a == "-o" || a == "--out") {
            if (i + 1 >= argc) { std::cerr << "error: " << a << " needs a directory\n"; return false; }
            opt.out_dir = argv[++i];
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "error: unknown option " << a << "\n";
            return false;
        } else if (opt.tokens_path.empty()) {
            opt.tokens_path = a;
        } else {
            std::cerr << "error: unexpected argument " << a << "\n";
            return false;
        }
    }
    if (!help && opt.tokens_path.empty()) { std::cerr << "error: tokens file is required\n"; return false; }
    return true;
}

bool read_file(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

bool write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    f << content;
    f.flush();
    return static_cast<bool>(f);
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    bool help = false;
    if (!parse_args(argc, argv, opt, help)) { print_usage(std::cerr); return 2; }
    if (help) { print_usage(std::cout); return 0; }

    std::string text;
    if (!read_file(opt.tokens_path, text)) {
        std::cerr << "error: cannot read " << opt.tokens_path << "\n";
        return 2;
    }

    try {
        auto specs = lexspec::parse_token_file(text);

        const automata::Nfa nfa = builder::build_nfa(specs);
        builder::DeterminizeResult det = builder::determinize(nfa);
        const automata::Dfa min_dfa = builder::minimize(det.dfa);

        std::error_code ec;
        std::filesystem::create_directories(opt.out_dir, ec);
        if (ec) {
            std::cerr << "error: cannot create " << opt.out_dir << ": " << ec.message() << "\n";
            return 2;
        }
        const std::filesystem::path dir = opt.out_dir;
        const auto nfa_path = dir / "nfa.txt";
        const auto dfa_path = dir / "dfa.txt";
        const auto min_path = dir / "dfa_min.txt";
        if (!write_file(nfa_path, automata::serialize(nfa)) ||
            !write_file(dfa_path, automata::serialize(det.dfa)) ||
            !write_file(min_path, automata::serialize(min_dfa))) {
            std::cerr << "error: cannot write output files to " << opt.out_dir << "\n";
            return 2;
        }

        std::size_t skip_count = 0;
        for (const auto& s : specs) skip_count += s.skip ? 1 : 0;

        std::size_t overrides = 0;
        for (const auto& w : det.warnings) {
            if (w.kind == builder::Diagnostic::Kind::NeverAccepted) {
                std::cerr << "warning: " << w.message << "\n";
            } else {
                ++overrides;
                if (opt.verbose) std::cout << "note: " << w.message << "\n";
            }
        }

        std::cout << "tokens file:          " << opt.tokens_path << "\n"
                  << "rules:                " << specs.size() << " (" << skip_count << " skip)\n"
                  << "NFA states:           " << nfa.state_count() << "\n"
                  << "DFA states:           " << det.dfa.state_count() << "\n"
                  << "minimized DFA states: " << min_dfa.state_count() << "\n"
                  << "trap state:           " << min_dfa.trap() << "\n"
                  << "priority overrides:   " << overrides << (opt.verbose || overrides == 0 ? "" : " (use -v to list)") << "\n"
                  << "written:              " << nfa_path.string() << ", " << dfa_path.string() << ", "
                  << min_path.string() << "\n";
        return 0;
    } catch (const lexspec::RegexError& e) {
        std::cerr << opt.tokens_path << ":" << e.line() << ":" << e.column() << ": error: " << e.message() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}