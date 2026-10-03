#pragma once
#include <string>
#include <vector>
struct Block { int position; int weight; int owner; };
struct GameState {
    int protocol_version = 0, k = 0, player = 0, ply = -1, winner = -1;
    std::string phase, reason, game_id;
    std::vector<Block> board;
    std::vector<int> remaining[2];
    int torque_left = 0, torque_right = 0;
    double clocks[2] = {0, 0};
    std::string players[2];
};
struct Move { int position = 0; int weight = 0; bool has_weight = false; };
Move choose_move(const GameState& state);
bool occupied_positions(const GameState& state, int position);
bool stable_after_add(const GameState& state, int position, int weight);
bool stable_after_remove(const GameState& state, int position);
