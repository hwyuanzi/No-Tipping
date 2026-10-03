#include "strategy.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

bool occupied_positions(const GameState& s, int p) {
    for (const auto& b : s.board) if (b.position == p) return true;
    return false;
}
bool stable_after_add(const GameState& s, int p, int w) {
    return s.torque_left - w * (p + 3) <= 0 && s.torque_right - w * (p + 1) >= 0;
}
bool stable_after_remove(const GameState& s, int p) {
    for (const auto& b : s.board) if (b.position == p)
        return s.torque_left + b.weight * (p + 3) <= 0 && s.torque_right + b.weight * (p + 1) >= 0;
    return false;
}
namespace {
using U64 = std::uint64_t;
using Clock = std::chrono::steady_clock;
constexpr int WIN = 100000;
constexpr U64 SLOTS = (U64(1) << 61) - 1;
struct Interrupted {};
struct Budget {
    Clock::time_point end;
    std::uint64_t ticks = 0;
    explicit Budget(double seconds) : end(Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds))) {}
    bool expired() const { return Clock::now() >= end; }
    void check() { if ((++ticks & 31) == 0 && expired()) throw Interrupted{}; }
};
struct Random {
    U64 state = 0x6a09e667f3bcc909ULL;
    U64 next() { state ^= state >> 12; state ^= state << 25; state ^= state >> 27; return state * 2685821657736338717ULL; }
    int below(int n) { return int(next() % unsigned(n)); }
    double unit() { return (next() >> 11) * (1.0 / 9007199254740992.0); }
};
Random rng;
int bits(U64 x) { return __builtin_popcountll(x); }
int first(U64 x) { return __builtin_ctzll(x); }
int floor_div(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
int ceil_div(int a, int b) { return -floor_div(-a, b); }
U64 interval(int lo, int hi) {
    lo = std::max(0, lo); hi = std::min(60, hi);
    if (lo > hi) return 0;
    return ((U64(1) << (hi + 1)) - 1) & ~((U64(1) << lo) - 1);
}
struct Action { int slot = 0, weight = 0; double rank = 0; };
struct State {
    std::array<int, 61> weight{};
    U64 occupied = 0, inventory[2] = {0, 0};
    int a = 6, b = 6, turn = 0, k = 24;
    bool adding = true;
    U64 places(int w) const { return interval(ceil_div(-a, w) + 27, floor_div(b, w) + 29) & ~occupied & SLOTS; }
    U64 removals() const {
        U64 legal = 0, rest = occupied;
        while (rest) {
            int i = first(rest); rest &= rest - 1;
            if (a - weight[i] * (i - 27) >= 0 && b + weight[i] * (i - 29) >= 0) legal |= U64(1) << i;
        }
        return legal;
    }
    bool can_add(int p) const {
        U64 ws = inventory[p];
        while (ws) { int w = first(ws) + 1; ws &= ws - 1; if (places(w)) return true; }
        return false;
    }
    void apply(Action m) {
        int w = adding ? m.weight : weight[m.slot];
        if (adding) {
            weight[m.slot] = w; occupied |= U64(1) << m.slot;
            inventory[turn] &= ~(U64(1) << (w - 1));
            a += w * (m.slot - 27); b -= w * (m.slot - 29);
        } else {
            occupied &= ~(U64(1) << m.slot);
            a -= w * (m.slot - 27); b += w * (m.slot - 29);
        }
        turn ^= 1;
        if (adding && !(inventory[0] | inventory[1])) { adding = false; turn = 0; }
    }
};
// Count the exact legal interval for every weight, including stranded heavy weights.
double mobility(const State& s, int player) {
    U64 ws = s.inventory[player]; int count = bits(ws), total = 0, stranded = 0; double score = 0;
    while (ws) {
        int w = first(ws) + 1; ws &= ws - 1;
        int n = bits(s.places(w)); total += n; stranded += n == 0;
        score += std::log1p(double(n));
    }
    return count ? 26.0 * score / count + 9.0 * std::log1p(double(total)) - 35.0 * stranded / count : 85.0;
}
double evaluate_add(const State& s, int p) { return mobility(s, p) - mobility(s, p ^ 1); }
std::vector<Action> add_actions(const State& s) {
    std::vector<Action> out; U64 ws = s.inventory[s.turn];
    while (ws) {
        int w = first(ws) + 1; ws &= ws - 1; U64 ps = s.places(w);
        while (ps) {
            int i = first(ps); ps &= ps - 1;
            State t = s; t.apply({i, w, 0});
            double score = evaluate_add(t, s.turn) + 8.0 * w / std::max(1, s.k);
            if (t.adding && !t.can_add(t.turn)) score = WIN;
            score += 0.3 * std::abs(i - 28) / 30.0;
            out.push_back({i, w, score});
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const Action& x, const Action& y) { return x.rank > y.rank; });
    return out;
}
// Exact impartial game: winning iff at least one legal child is losing.
// Cache keys are collision-free subsets of ONE fixed board universe. Only fully
// proven values are stored; an interrupted search never manufactures a loss.
struct Proof { signed char value = 0; signed char slot = -1; };
struct RemovalSolver {
    std::array<int, 61> weights{};
    std::unordered_map<U64, Proof> table;
    Budget* budget = nullptr;
    std::uint64_t nodes = 0, limit = std::numeric_limits<std::uint64_t>::max();
    explicit RemovalSolver(const State& s, Budget* b = nullptr) : weights(s.weight), budget(b) { table.reserve(1024); }
    U64 legal(U64 mask, int a, int b) const {
        U64 result = 0;
        while (mask) {
            int i = first(mask); mask &= mask - 1;
            if (a - weights[i] * (i - 27) >= 0 && b + weights[i] * (i - 29) >= 0) result |= U64(1) << i;
        }
        return result;
    }
    void store(U64 mask, Proof p) { if (table.size() < 1500000) table.emplace(mask, p); }
    Proof solve(U64 mask, int a, int b) {
        if (++nodes > limit) throw Interrupted{};
        if (budget) budget->check();
        auto found = table.find(mask); if (found != table.end()) return found->second;
        U64 moves = legal(mask, a, b);
        if (!moves) { Proof p{-1, -1}; store(mask, p); return p; }
        struct Ordered { int slot, count; };
        std::array<Ordered, 61> order{}; int n = 0;
        while (moves) {
            int i = first(moves); moves &= moves - 1; U64 child = mask ^ (U64(1) << i);
            auto cached = table.find(child);
            if (cached != table.end() && cached->second.value < 0) {
                Proof p{1, static_cast<signed char>(i)}; store(mask, p); return p;
            }
            if (cached != table.end() && cached->second.value > 0) continue;
            int na = a - weights[i] * (i - 27), nb = b + weights[i] * (i - 29);
            int c = bits(legal(child, na, nb));
            if (!c) { Proof p{1, static_cast<signed char>(i)}; store(mask, p); return p; }
            order[n++] = {i, c};
        }
        std::sort(order.begin(), order.begin() + n, [](const Ordered& x, const Ordered& y) { return x.count < y.count; });
        for (int j = 0; j < n; ++j) {
            int i = order[j].slot;
            Proof child = solve(mask ^ (U64(1) << i), a - weights[i] * (i - 27), b + weights[i] * (i - 29));
            if (child.value < 0) { Proof p{1, static_cast<signed char>(i)}; store(mask, p); return p; }
        }
        Proof p{-1, -1}; store(mask, p); return p;
    }
};
Action removal_policy(const State& s, Random& random, bool greedy) {
    U64 moves = s.removals(); std::array<Action, 61> actions{}; int n = 0;
    while (moves) {
        int i = first(moves); moves &= moves - 1; State t = s; t.apply({i, 0, 0});
        int replies = bits(t.removals()); if (!replies) return {i, 0, WIN};
        actions[n++] = {i, 0, -replies + random.unit() * (greedy ? 2.0 : 15.0)};
    }
    if (!n) return {};
    return *std::max_element(actions.begin(), actions.begin() + n, [](const Action& x, const Action& y) { return x.rank < y.rank; });
}
Action placement_policy(const State& s, Random& random, int policy) {
    std::array<int, 61> ws{}; int nw = 0; U64 inv = s.inventory[s.turn];
    while (inv) { int w = first(inv) + 1; inv &= inv - 1; if (s.places(w)) ws[nw++] = w; }
    if (!nw) return {};
    Action best{}; best.rank = -1e100;
    for (int sample = 0; sample < 10; ++sample) {
        int wi = random.below(nw); if (policy == 1) wi = std::max(wi, random.below(nw));
        int w = ws[wi]; U64 places = s.places(w); int i;
        if (sample % 4 == 0) i = first(places);
        else if (sample % 4 == 1) i = 63 - __builtin_clzll(places);
        else { int r = random.below(bits(places)); while (r--) places &= places - 1; i = first(places); }
        State t = s; t.apply({i, w, 0});
        if (t.adding && !t.can_add(t.turn)) return {i, w, WIN};
        double score = evaluate_add(t, s.turn) + (policy == 1 ? 18.0 : 5.0) * w / std::max(1, s.k);
        score += random.unit() * (policy == 2 ? 35.0 : 8.0);
        if (score > best.rank) best = {i, w, score};
    }
    return best;
}
int rollout(State s, Budget& budget, RemovalSolver* persistent = nullptr) {
    int policies[2] = {rng.below(3), rng.below(3)};
    for (int ply = 0; ply < 125; ++ply) {
        budget.check();
        if (s.adding) {
            if (!s.can_add(s.turn)) return s.turn ^ 1;
            s.apply(placement_policy(s, rng, policies[s.turn]));
        } else {
            if (!s.removals()) return s.turn ^ 1;
            if (bits(s.occupied) <= (persistent ? 18 : 10)) {
                if (persistent) {
                    auto old_limit = persistent->limit; persistent->limit = persistent->nodes + 12000;
                    try {
                        Proof p = persistent->solve(s.occupied, s.a, s.b); persistent->limit = old_limit;
                        return p.value > 0 ? s.turn : s.turn ^ 1;
                    } catch (const Interrupted&) { persistent->limit = old_limit; if (budget.expired()) throw; }
                } else {
                    RemovalSolver exact(s, &budget); exact.limit = 3000;
                    try { Proof p = exact.solve(s.occupied, s.a, s.b); return p.value > 0 ? s.turn : s.turn ^ 1; }
                    catch (const Interrupted&) { if (budget.expired()) throw; }
                }
            }
            s.apply(removal_policy(s, rng, policies[s.turn] != 2));
        }
    }
    return s.turn ^ 1;
}
struct Edge { Action action; int child = -1; };
struct Node {
    int turn = 0, visits = 0, solved = 0; double wins = 0;
    bool initialized = false, complete = true;
    std::vector<Edge> edges;
};
std::vector<Action> removal_actions(const State& s, RemovalSolver* solver) {
    std::vector<Action> out; U64 moves = s.removals();
    while (moves) {
        int i = first(moves); moves &= moves - 1; State t = s; t.apply({i, 0, 0});
        double score = -bits(t.removals());
        if (solver) {
            auto it = solver->table.find(t.occupied);
            if (it != solver->table.end()) score = it->second.value < 0 ? WIN : -WIN;
        }
        out.push_back({i, 0, score});
    }
    std::sort(out.begin(), out.end(), [](const Action& x, const Action& y) { return x.rank > y.rank; }); return out;
}
std::vector<Action> shortlist(const std::vector<Action>& all, int top = 32) {
    if (int(all.size()) <= top + 16) return all;
    std::vector<Action> out(all.begin(), all.begin() + top); U64 seen = 0;
    for (const auto& m : all) if (!(seen & (U64(1) << (m.weight - 1)))) {
        seen |= U64(1) << (m.weight - 1); bool exists = false;
        for (const auto& c : out) if (c.slot == m.slot && c.weight == m.weight) exists = true;
        if (!exists) out.push_back(m);
    }
    return out;
}
void initialize(Node& node, const State& s, RemovalSolver* solver, const std::vector<Action>* roots = nullptr) {
    node.initialized = true; node.turn = s.turn;
    std::vector<Action> actions;
    if (roots) { actions = *roots; node.complete = !s.adding; }
    else if (s.adding) {
        auto all = add_actions(s); actions = shortlist(all, 20);
        node.complete = actions.size() == all.size();
    } else actions = removal_actions(s, solver);
    if (actions.empty()) { node.solved = -1; return; }
    for (const auto& m : actions) node.edges.push_back({m, -1});
}
Action monte_carlo(const State& root, Budget& budget, const std::vector<Action>& candidates, RemovalSolver* solver = nullptr) {
    if (candidates.size() == 1) return candidates.front();
    std::vector<Node> tree; tree.reserve(20000); tree.emplace_back(); initialize(tree[0], root, solver, &candidates);
    std::vector<int> path; path.reserve(125); Action fallback = candidates.front();
    while (!budget.expired() && tree.size() < 140000) {
        State s = root; int current = 0, winner = -1; path.clear();
        try {
            while (true) {
                budget.check(); path.push_back(current);
                if (!tree[current].initialized) initialize(tree[current], s, solver);
                if (solver && !s.adding) {
                    auto it = solver->table.find(s.occupied);
                    if (it != solver->table.end()) tree[current].solved = it->second.value;
                }
                if (tree[current].solved) { winner = tree[current].solved > 0 ? s.turn : s.turn ^ 1; break; }
                Node& node = tree[current];
                int width = s.adding ? std::min<int>(node.edges.size(), 3 + int(2.5 * std::sqrt(node.visits + 1.0))) : int(node.edges.size());
                int selected = -1; double highest = -1e100, logv = std::log(node.visits + 2.0);
                for (int e = 0; e < width; ++e) {
                    const Edge& edge = node.edges[e]; double score;
                    if (edge.child < 0) score = 10.0 + 0.001 * (width - e);
                    else {
                        const Node& child = tree[edge.child];
                        if (child.solved) {
                            bool won = (child.solved > 0 ? child.turn : child.turn ^ 1) == node.turn;
                            if (won) { selected = e; highest = 1e50; break; }
                            score = -1e20;
                        } else {
                            double q = (child.wins + 0.5) / (child.visits + 1.0); if (child.turn != node.turn) q = 1.0 - q;
                            score = q + 0.75 * std::sqrt(logv / (child.visits + 1.0)) + 0.12 / (1.0 + e) / std::sqrt(child.visits + 1.0);
                        }
                    }
                    if (score > highest) { highest = score; selected = e; }
                }
                if (selected < 0) { node.solved = -1; winner = s.turn ^ 1; break; }
                Action action = node.edges[selected].action; int child = node.edges[selected].child; s.apply(action);
                if (child < 0) {
                    child = int(tree.size()); tree[current].edges[selected].child = child;
                    tree.emplace_back(); tree[child].turn = s.turn; path.push_back(child);
                    winner = rollout(s, budget, solver); break;
                }
                current = child;
            }
        } catch (const Interrupted&) { break; }
        for (int j = int(path.size()) - 1; j >= 0; --j) {
            Node& node = tree[path[j]]; ++node.visits; if (winner == node.turn) ++node.wins;
            if (!node.initialized || node.solved) continue;
            bool all_solved = true;
            for (const auto& edge : node.edges) {
                if (edge.child < 0 || !tree[edge.child].solved) { all_solved = false; continue; }
                const Node& child = tree[edge.child]; int cw = child.solved > 0 ? child.turn : child.turn ^ 1;
                if (cw == node.turn) { node.solved = 1; all_solved = false; break; }
            }
            if (!node.solved && node.complete && all_solved) node.solved = -1;
        }
        if (tree[0].solved > 0) break;
    }
    int best_visits = -1; double best_mean = -1;
    for (const auto& edge : tree[0].edges) {
        if (edge.child < 0) continue; const Node& child = tree[edge.child];
        if (child.solved && (child.solved > 0 ? child.turn : child.turn ^ 1) == root.turn) return edge.action;
        bool lost = child.solved && (child.solved > 0 ? child.turn : child.turn ^ 1) != root.turn;
        int visits = lost ? -2 : child.visits;
        double mean = (child.wins + 0.5) / (child.visits + 1.0); if (child.turn != root.turn) mean = 1.0 - mean;
        if (visits > best_visits || (visits == best_visits && mean > best_mean)) {
            best_visits = visits; best_mean = mean; fallback = edge.action;
        }
    }
    return fallback;
}
// Beam alpha-beta supplies priors. Its placement results are heuristic: pruning
// candidate actions is NEVER a proof against every legal opponent response.
double search_add(const State& s, int depth, double alpha, double beta, int perspective, Budget& budget) {
    budget.check();
    if (!s.adding) {
        if (bits(s.occupied) <= 17) {
            RemovalSolver exact(s, &budget); exact.limit = 180000;
            try { Proof p = exact.solve(s.occupied, s.a, s.b); int winner = p.value > 0 ? s.turn : s.turn ^ 1; return winner == perspective ? WIN : -WIN; }
            catch (const Interrupted&) { if (budget.expired()) throw; }
        }
        return 0;
    }
    if (!s.can_add(s.turn)) return s.turn == perspective ? -WIN - depth : WIN + depth;
    if (!depth) return evaluate_add(s, perspective);
    auto actions = add_actions(s); bool maximize = s.turn == perspective;
    double result = maximize ? -1e100 : 1e100; int width = std::min<int>(actions.size(), depth == 1 ? 16 : 12);
    for (int j = 0; j < width; ++j) {
        State t = s; t.apply(actions[j]); double value = search_add(t, depth - 1, alpha, beta, perspective, budget);
        if (maximize) { result = std::max(result, value); alpha = std::max(alpha, value); }
        else { result = std::min(result, value); beta = std::min(beta, value); }
        if (alpha >= beta) break;
    }
    return result;
}
State from_game(const GameState& g) {
    State s; s.a = -g.torque_left; s.b = g.torque_right; s.turn = g.player; s.k = g.k; s.adding = g.phase == "add";
    for (const auto& block : g.board) { int i = block.position + 30; s.weight[i] = block.weight; s.occupied |= U64(1) << i; }
    for (int p = 0; p < 2; ++p) for (int w : g.remaining[p]) if (w >= 1 && w <= 61) s.inventory[p] |= U64(1) << (w - 1);
    return s;
}
std::array<int, 61> removal_universe{};
RemovalSolver* cached_solver = nullptr;
std::string cached_game;
double seconds_for(const GameState& g, const State& s) {
    double clock = g.clocks[g.player]; if (!(clock > 0)) clock = 120;
    double usable = std::max(0.0, clock - std::max(0.08, clock * 0.035));
    int future = s.adding ? bits(s.inventory[g.player]) + (bits(s.occupied) + bits(s.inventory[0]) + bits(s.inventory[1])) / 2 + 2 : bits(s.occupied) / 2 + 1;
    double base = usable / std::max(1, future);
    double allocation = s.adding ? std::min(2.0, base * 0.65) : std::min(12.0, base * 1.6);
    return std::max(0.0, std::min(usable, allocation));
}
} // namespace
Move choose_move(const GameState& g) {
    State s = from_game(g); rng.state ^= s.occupied + U64(s.a + 65536) * 0x9e3779b97f4a7c15ULL + U64(g.player);
    double seconds = seconds_for(g, s);
    if (s.adding) {
        auto all = add_actions(s);
        if (all.empty()) {
            U64 empty = ~s.occupied & SLOTS;
            return {empty ? first(empty) - 30 : -30, g.remaining[g.player].empty() ? 1 : g.remaining[g.player].front(), true};
        }
        for (const auto& m : all) if (m.rank >= WIN) return {m.slot - 30, m.weight, true};
        auto candidates = shortlist(all); Action chosen = candidates.front(); Budget total(seconds);
        if (seconds < 0.012) return {chosen.slot - 30, chosen.weight, true};
        // On the final placement, retain a certified winning removal position.
        if (bits(s.inventory[0]) + bits(s.inventory[1]) == 1) {
            Budget endings(seconds * 0.7);
            for (const auto& m : all) {
                State t = s; t.apply(m); RemovalSolver exact(t, &endings);
                exact.limit = 250000;
                try {
                    Proof p = exact.solve(t.occupied, t.a, t.b);
                    int winner = p.value > 0 ? t.turn : t.turn ^ 1;
                    if (winner == s.turn) return {m.slot - 30, m.weight, true};
                } catch (const Interrupted&) { if (endings.expired()) break; }
            }
        }
        Budget tactical(std::min(seconds * 0.28, 0.45));
        for (int depth = 1; depth <= 4 && !tactical.expired(); ++depth) {
            std::vector<Action> ranked = candidates; bool complete = true;
            try {
                for (auto& m : ranked) { State t = s; t.apply(m); m.rank = search_add(t, depth, -1e100, 1e100, s.turn, tactical); }
            } catch (const Interrupted&) { complete = false; }
            if (!complete) break;
            if (depth == 1) {
                // At one reply, a witnessed placement trap or exact lost endgame
                // is a certificate. Exclude it if another root move survives.
                bool survives = false;
                for (const auto& m : ranked) if (m.rank > -WIN) survives = true;
                if (survives) ranked.erase(std::remove_if(ranked.begin(), ranked.end(),
                    [](const Action& m) { return m.rank <= -WIN; }), ranked.end());
            }
            std::stable_sort(ranked.begin(), ranked.end(), [](const Action& a, const Action& b) { return a.rank > b.rank; });
            candidates = std::move(ranked); chosen = candidates.front();
            if (chosen.rank >= WIN) break;
        }
        if (!total.expired()) chosen = monte_carlo(s, total, candidates);
        return {chosen.slot - 30, chosen.weight, true};
    }
    bool reset = !cached_solver || cached_game != g.game_id;
    for (int i = 0; i < 61; ++i) if ((s.occupied & (U64(1) << i)) && removal_universe[i] != s.weight[i]) reset = true;
    if (reset) { delete cached_solver; cached_solver = new RemovalSolver(s); removal_universe = s.weight; cached_game = g.game_id; }
    auto actions = removal_actions(s, cached_solver);
    if (actions.empty()) return {g.board.empty() ? -4 : g.board.front().position, 0, false};
    if (actions.size() == 1) return {actions.front().slot - 30, 0, false};
    Budget total(seconds); Budget proof(seconds * (bits(s.occupied) <= 26 ? 0.92 : 0.38));
    cached_solver->budget = &proof; cached_solver->limit = std::numeric_limits<std::uint64_t>::max();
    try {
        Proof answer = cached_solver->solve(s.occupied, s.a, s.b);
        if (answer.value > 0) { cached_solver->budget = nullptr; return {int(answer.slot) - 30, 0, false}; }
    } catch (const Interrupted&) {}
    cached_solver->budget = &total; Action chosen = actions.front();
    auto root_fact = cached_solver->table.find(s.occupied);
    if (root_fact != cached_solver->table.end() && root_fact->second.value < 0) {
        // In a forced loss, prefer positions with more losing opponent replies.
        double best = -1;
        try {
            for (const auto& m : actions) {
                State t = s; t.apply(m); U64 replies = t.removals();
                int n = bits(replies), mistakes = 0;
                while (replies) {
                    int i = first(replies); replies &= replies - 1; State u = t; u.apply({i, 0, 0});
                    Proof p = cached_solver->solve(u.occupied, u.a, u.b); mistakes += p.value > 0;
                }
                double score = n ? double(mistakes) / n + 0.0001 * n : 1.0;
                if (score > best) { best = score; chosen = m; }
            }
        } catch (const Interrupted&) {}
        cached_solver->budget = nullptr;
        return {chosen.slot - 30, 0, false};
    }
    if (!total.expired()) chosen = monte_carlo(s, total, actions, cached_solver);
    cached_solver->budget = nullptr; return {chosen.slot - 30, 0, false};
}
