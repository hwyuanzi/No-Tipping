// Minimal C++17 example. It uses only the standard library and the runner's JSON protocol.
#include <cstdlib>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

struct Block { int position; int weight; };

int integerField(const std::string& text, const std::string& key) {
    const std::regex field("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(text, match, field)) throw std::runtime_error("missing field: " + key);
    return std::stoi(match[1].str());
}

std::vector<Block> parseBoard(const std::string& input) {
    const auto start = input.find("\"board\"");
    const auto end = input.find(']', start);
    if (start == std::string::npos || end == std::string::npos) throw std::runtime_error("missing board");
    const std::string array = input.substr(start, end - start);
    const std::regex object(R"(\{[^{}]*\})");
    std::vector<Block> board;
    for (std::sregex_iterator it(array.begin(), array.end(), object), last; it != last; ++it) {
        const auto item = it->str();
        board.push_back({integerField(item, "position"), integerField(item, "weight")});
    }
    return board;
}

std::vector<int> remainingFor(const std::string& input, int player) {
    auto start = input.find("\"remaining\"");
    if (start == std::string::npos) throw std::runtime_error("missing remaining weights");
    start = input.find('[', start);
    for (int i = 0; i <= player; ++i) {
        start = input.find('[', start + 1);
        auto end = input.find(']', start);
        if (start == std::string::npos || end == std::string::npos) throw std::runtime_error("invalid remaining weights");
        if (i == player) {
            std::vector<int> weights;
            const std::regex number(R"(-?[0-9]+)");
            const auto array = input.substr(start, end - start);
            for (std::sregex_iterator it(array.begin(), array.end(), number), last; it != last; ++it)
                weights.push_back(std::stoi(it->str()));
            return weights;
        }
        start = end;
    }
    throw std::runtime_error("invalid player index");
}

bool stable(int left, int right) { return left <= 0 && right >= 0; }

std::string chooseMove(const std::string& input) {
    const auto player = integerField(input, "player");
    const auto left = integerField(input, "left");
    const auto right = integerField(input, "right");
    const auto board = parseBoard(input);
    std::set<int> occupied;
    for (const auto& block : board) occupied.insert(block.position);

    if (std::regex_search(input, std::regex(R"("phase"\s*:\s*"add")"))) {
        const auto weights = remainingFor(input, player);
        for (const int weight : weights) {
            for (int position = -30; position <= 30; ++position) {
                if (occupied.count(position)) continue;
                if (stable(left - weight * (position + 3), right - weight * (position + 1))) {
                    return "{\"position\":" + std::to_string(position) +
                           ",\"weight\":" + std::to_string(weight) + "}";
                }
            }
        }
        for (int position = -30; position <= 30; ++position) {
            if (!occupied.count(position)) {
                return "{\"position\":" + std::to_string(position) +
                       ",\"weight\":" + std::to_string(weights.front()) + "}";
            }
        }
    } else {
        for (const auto& block : board) {
            if (stable(left + block.weight * (block.position + 3),
                       right + block.weight * (block.position + 1))) {
                return "{\"position\":" + std::to_string(block.position) + "}";
            }
        }
        return "{\"position\":" + std::to_string(board.front().position) + "}";
    }
    return {};
}

int main() {
    std::string input;
    while (std::getline(std::cin, input)) {
        const auto move = chooseMove(input);
        if (move.empty()) return 1;
        std::cout << move << std::endl;
    }
    return 0;
}
