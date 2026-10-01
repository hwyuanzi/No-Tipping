// Starter template: implement chooseMove and return one JSON move per input line.
#include <iostream>
#include <string>

std::string chooseMove(const std::string& state) {
    // TODO: Parse state and return, for example, R"({"position":-3,"weight":2})".
    return "{}";
}

int main() {
    std::string state;
    while (std::getline(std::cin, state)) {
        std::cout << chooseMove(state) << std::endl;
    }
}
