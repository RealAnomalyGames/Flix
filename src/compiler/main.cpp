
#include <iostream>
#include <string>

#include "flix/version.hpp"

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Flix Compiler "
                  << flix::version() << "\n";

        std::cout << "Usage: flixc build <file.flix>\n";
        std::cout << "       flixc asm <file.flix>\n";
        std::cout << "       flixc version\n";

        return 0;
    }

    std::string command = argv[1];

    if (command == "version") {
        std::cout << "Flix Compiler "
                  << flix::version() << "\n";

        return 0;
    }

    if (command == "build" || command == "asm") {
        if (argc < 3) {
            std::cerr << "Error: Missing Flix source file.\n";
            return 1;
        }

        std::cout << "Flix compiler initialized.\n";
        std::cout << "Mode: " << command << "\n";
        std::cout << "Source: " << argv[2] << "\n";

        // Native compilation and assembly generation
        // will be implemented in later development steps.

        return 0;
    }

    std::cerr << "Unknown command: " << command << "\n";
    return 1;
}
