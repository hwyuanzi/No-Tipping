#ifndef NOTIPPING_STRATEGY_H
#define NOTIPPING_STRATEGY_H
#include <stddef.h>

typedef struct { int position; int weight; int owner; } Block;
typedef struct {
    int protocol_version, k, player, ply, winner;
    char phase[8], reason[64], game_id[64];
    Block board[128]; int board_count;
    int remaining[2][128], remaining_count[2];
    int torque_left, torque_right;
    double clocks[2];
    char players[2][64];
} GameState;
typedef struct { int position; int weight; int has_weight; } Move;

/* Organizer parses and serializes JSON. Students implement this function. */
int choose_move(const GameState *state, Move *move);
int occupied_positions(const GameState *state, int position);
int stable_after_add(const GameState *state, int position, int weight);
int stable_after_remove(const GameState *state, int position);
#endif
