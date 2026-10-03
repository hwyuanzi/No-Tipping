// Organizer-owned wrapper. It parses and serializes the stable tournament protocol.
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <sstream>
#include "strategy.hpp"

static int integer(const std::string& text, const std::string& key, int fallback = 0) { std::smatch m; std::regex r("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)"); return std::regex_search(text, m, r) ? std::stoi(m[1]) : fallback; }
static std::string string_value(const std::string& text, const std::string& key) { std::smatch m; std::regex r("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\""); return std::regex_search(text, m, r) ? m[1].str() : ""; }
static GameState parse_state(const std::string& s) {
    GameState g; g.protocol_version = integer(s, "protocol_version"); g.k = integer(s, "k"); g.player = integer(s, "player"); g.ply = integer(s, "ply", -1); g.winner = integer(s, "winner", -1); g.phase = string_value(s, "phase"); g.reason = string_value(s, "reason"); g.game_id = string_value(s, "game_id");
    auto board_start = s.find("\"board\""); auto board_end = s.find(']', board_start); if (board_start == std::string::npos || board_end == std::string::npos) throw std::runtime_error("missing board");
    std::string board = s.substr(board_start, board_end - board_start); std::regex object(R"(\{[^{}]*\})"); for (std::sregex_iterator it(board.begin(), board.end(), object), end; it != end; ++it) { std::string x = it->str(); g.board.push_back({integer(x, "position"), integer(x, "weight"), integer(x, "owner", -1)}); }
    auto rem = s.find("\"remaining\""); if (rem == std::string::npos) throw std::runtime_error("missing remaining"); for (int p = 0; p < 2; ++p) { auto a = s.find('[', rem); auto e = s.find(']', a); if (a == std::string::npos || e == std::string::npos) throw std::runtime_error("invalid remaining"); std::string x = s.substr(a, e - a); std::regex n(R"(-?[0-9]+)"); for (std::sregex_iterator it(x.begin(), x.end(), n), end; it != end; ++it) g.remaining[p].push_back(std::stoi(it->str())); rem = e + 1; }
    auto torque = s.find("\"torques\""); if (torque != std::string::npos) { auto x = s.substr(torque); g.torque_left = integer(x, "left"); g.torque_right = integer(x, "right"); }
    auto clocks = s.find("\"clocks\""); if (clocks != std::string::npos) { std::string x = s.substr(clocks); std::regex number(R"(-?[0-9]+(?:\.[0-9]+)?)"); int i = 0; for (std::sregex_iterator it(x.begin(), x.end(), number), end; it != end && i < 2; ++it) g.clocks[i++] = std::stod(it->str()); }
    auto players = s.find("\"players\""); if (players != std::string::npos) { auto start = s.find('[', players); std::string x = s.substr(start); std::regex name("\\\"([^\\\"]*)\\\""); int i = 0; for (std::sregex_iterator it(x.begin(), x.end(), name), end; it != end && i < 2; ++it) g.players[i++] = (*it)[1].str(); }
    return g;
}
int main() { std::string state; while (std::getline(std::cin, state)) { try { Move m = choose_move(parse_state(state)); if (m.has_weight) std::cout << "{\"position\":" << m.position << ",\"weight\":" << m.weight << "}\n"; else std::cout << "{\"position\":" << m.position << "}\n"; std::cout << std::flush; } catch (const std::exception& e) { std::cerr << "strategy error: " << e.what() << '\n'; return 1; } } }
