// Organizer-owned wrapper. Compile with strategy.cpp.
#include <iostream>
#include <string>
#include "strategy.hpp"
int main() { std::string state; while (std::getline(std::cin, state)) { std::string move = choose_move(state); if (move.empty()) { std::cerr << "strategy returned no move\n"; return 1; } std::cout << move << '\n' << std::flush; } }
