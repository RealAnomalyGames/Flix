
#include <iostream>
#include <string>

#include "flix/version.hpp"

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Flix Interpreter "
                  << flix::version() << "\n";

        std::cout << "Usage: flix run <file.flix>\n";
        std::cout << "       flix version\n";

        return 0;
    }

    std::string command = argv[1];

    if (command == "version") {
        std::cout << "Flix Interpreter "
                  << flix::version() << "\n";

        return 0;
    }

    if (command == "run") {
        if (argc < 3) {
            std::cerr << "Error: Missing Flix source file.\n";
            return 1;
        }

        std::cout << "Flix interpreter initialized.\n";
        std::cout << "Source: " << argv[2] << "\n";

        // Source execution will be implemented
        // as the lexer, parser, and runtime develop.

        return 0;
    }

    std::cerr << "Unknown command: " << command << "\n";
    return 1;
}
