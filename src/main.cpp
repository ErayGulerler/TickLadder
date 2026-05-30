#include "tickladder/replay.hpp"

#include <exception>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: replay FILE.csv\n";
        return 2;
    }
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open " << argv[1] << '\n';
        return 1;
    }
    try {
        tickladder::replay(file, std::cout);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
