#include "strategy.h"
#include <limits.h>
#include <stddef.h>

int occupied_positions(const GameState *state, int position) {
    for (int i = 0; i < state->board_count; ++i)
        if (state->board[i].position == position) return 1;
    return 0;
}
int stable_after_add(const GameState *s, int p, int w) {
    return s->torque_left - w * (p + 3) <= 0 &&
           s->torque_right - w * (p + 1) >= 0;
}
int stable_after_remove(const GameState *s, int p) {
    for (int i = 0; i < s->board_count; ++i) if (s->board[i].position == p)
        return s->torque_left + s->board[i].weight * (p + 3) <= 0 &&
               s->torque_right + s->board[i].weight * (p + 1) >= 0;
    return 0;
}

/* Example student strategy: choose the first stable move. */
int choose_move(const GameState *s, Move *move) {
    if (s->phase[0] == 'a') {
        for (int wi = 0; wi < s->remaining_count[s->player]; ++wi) {
            int w = s->remaining[s->player][wi];
            for (int p = -30; p <= 30; ++p)
                if (!occupied_positions(s, p) && stable_after_add(s, p, w)) {
                    move->position = p; move->weight = w; move->has_weight = 1; return 1;
                }
        }
        for (int p = -30; p <= 30; ++p) if (!occupied_positions(s, p)) {
            move->position = p; move->weight = s->remaining[s->player][0]; move->has_weight = 1; return 1;
        }
    } else {
        for (int i = 0; i < s->board_count; ++i)
            if (stable_after_remove(s, s->board[i].position)) {
                move->position = s->board[i].position; move->has_weight = 0; return 1;
            }
        if (s->board_count) { move->position = s->board[0].position; move->has_weight = 0; return 1; }
    }
    return 0;
}
