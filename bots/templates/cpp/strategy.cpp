#include "strategy.hpp"
bool occupied_positions(const GameState& s, int p) { for (const auto& b : s.board) if (b.position == p) return true; return false; }
bool stable_after_add(const GameState& s, int p, int w) { return s.torque_left - w * (p + 3) <= 0 && s.torque_right - w * (p + 1) >= 0; }
bool stable_after_remove(const GameState& s, int p) { for (const auto& b : s.board) if (b.position == p) return s.torque_left + b.weight * (p + 3) <= 0 && s.torque_right + b.weight * (p + 1) >= 0; return false; }
/* Student-owned strategy; runner.cpp handles parsing, serialization, and I/O. */
Move choose_move(const GameState& s) {
    Move m;
    if (s.phase == "add") {
        for (int w : s.remaining[s.player]) for (int p = -30; p <= 30; ++p)
            if (!occupied_positions(s, p) && stable_after_add(s, p, w)) return {p, w, true};
        for (int p = -30; p <= 30; ++p) if (!occupied_positions(s, p) && !s.remaining[s.player].empty()) return {p, s.remaining[s.player].front(), true};
    } else {
        for (const auto& b : s.board) if (stable_after_remove(s, b.position)) return {b.position, 0, false};
        if (!s.board.empty()) return {s.board.front().position, 0, false};
    }
    return m;
}
