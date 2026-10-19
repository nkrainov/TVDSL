#include "builder/minimize.hpp"

#include <map>
#include <queue>

namespace funny::builder {

namespace {

using automata::Dfa;
using automata::kAlphabetSize;
using automata::kNoState;
using automata::RuleId;
using automata::StateId;

}  // namespace

Dfa minimize(const Dfa& dfa) {
    dfa.validate();
    const std::size_t n = dfa.state_count();

    std::vector<bool> reachable(n, 0);
    {
        std::vector<StateId> stack{dfa.start(), dfa.trap()};
        reachable[dfa.start()] = reachable[dfa.trap()] = true;
        while (!stack.empty()) {
            StateId s = stack.back();
            stack.pop_back();
            for (std::size_t c = 0; c < kAlphabetSize; ++c) {
                StateId t = dfa.next(s, static_cast<unsigned char>(c));
                if (!reachable[t]) { 
                    reachable[t] = true; 
                    stack.push_back(t); 
                }
            }
        }
    }

    std::vector<std::vector<std::vector<StateId>>> inv(kAlphabetSize, std::vector<std::vector<StateId>>(n));
    for (std::size_t s = 0; s < n; ++s) {
        if (!reachable[s]) continue;
        for (std::size_t c = 0; c < kAlphabetSize; ++c) {
            inv[c][dfa.next(static_cast<StateId>(s), static_cast<unsigned char>(c))].push_back(static_cast<StateId>(s));
        }
    }

    std::vector<std::vector<StateId>> blocks;
    std::vector<std::size_t> block_of(n, 0);
    {
        std::map<RuleId, std::size_t> by_accept;
        for (std::size_t s = 0; s < n; ++s) {
            if (!reachable[s]) continue;
            auto [it, inserted] = by_accept.try_emplace(dfa.accept(static_cast<StateId>(s)), blocks.size());
            if (inserted) blocks.emplace_back();
            blocks[it->second].push_back(static_cast<StateId>(s));
            block_of[s] = it->second;
        }
    }

    std::vector<std::size_t> work;
    std::vector<bool> in_work(blocks.size(), 1);
    for (std::size_t b = 0; b < blocks.size(); ++b) work.push_back(b);

    std::vector<bool> mark(n, 0);
    while (!work.empty()) {
        const std::size_t a = work.back();
        work.pop_back();
        in_work[a] = true;
        const std::vector<StateId> splitter = blocks[a];

        for (std::size_t c = 0; c < kAlphabetSize; ++c) {
            std::map<std::size_t, std::vector<StateId>> hits;
            for (StateId t : splitter) {
                for (StateId s : inv[c][t]) {
                    hits[block_of[s]].push_back(s);
                }
            }
                
            for (auto& [y, hit] : hits) {
                if (hit.size() == blocks[y].size()) continue;

                for (StateId s : hit) mark[s] = true;
                std::vector<StateId> rest;
                for (StateId s : blocks[y])
                    if (!mark[s]) rest.push_back(s);

                const std::size_t ny = blocks.size();
                blocks[y] = std::move(rest);
                blocks.push_back(hit);
                in_work.push_back(0);
                for (StateId s : hit) { 
                    block_of[s] = ny; 
                    mark[s] = false; 
                }

                if (in_work[y]) {
                    work.push_back(ny);
                    in_work[ny] = true;
                } else {
                    const std::size_t smaller = blocks[y].size() <= blocks[ny].size() ? y : ny;
                    work.push_back(smaller);
                    in_work[smaller] = true;
                }
            }
        }
    }
    Dfa out;
    for (const auto& r : dfa.rules()) out.add_rule(r.name, r.skip);

    const std::size_t trap_block = block_of[dfa.trap()];
    std::vector<StateId> new_id(blocks.size(), kNoState);
    std::vector<std::size_t> order;

    new_id[trap_block] = out.add_state();
    order.push_back(trap_block);

    const std::size_t start_block = block_of[dfa.start()];
    std::queue<std::size_t> queue;
    auto discover = [&](std::size_t b) {
        if (new_id[b] != kNoState) return;
        new_id[b] = out.add_state(dfa.accept(blocks[b].front()));
        order.push_back(b);
        queue.push(b);
    };
    discover(start_block);
    while (!queue.empty()) {
        std::size_t b = queue.front();
        queue.pop();
        const StateId rep = blocks[b].front();
        for (std::size_t c = 0; c < kAlphabetSize; ++c)
            discover(block_of[dfa.next(rep, static_cast<unsigned char>(c))]);
    }

    for (std::size_t b : order) {
        const StateId rep = blocks[b].front();
        for (std::size_t c = 0; c < kAlphabetSize; ++c) {
            const auto byte = static_cast<unsigned char>(c);
            out.set_transition(new_id[b], byte, new_id[block_of[dfa.next(rep, byte)]]);
        }
    }

    out.set_trap(0);
    out.set_start(new_id[start_block]);
    out.validate();
    return out;
}

}  // namespace funny::builder