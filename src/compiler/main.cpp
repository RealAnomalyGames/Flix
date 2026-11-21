
#include <iostream>
#include <string>

#include "flix/version.hpp"
#include "flix/program.hpp"

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Flix Compiler "
                  << flix::version() << "\n";

        std::cout << "Usage: flixc check <file.flix>\n";
        std::cout << "       flixc version\n";

        return 0;
    }

    std::string command = argv[1];

    if (command == "version") {
        std::cout << "Flix Compiler "
                  << flix::version() << "\n";

        return 0;
    }

    if (command != "check" || argc < 3) {
        std::cerr << "Usage: flixc check <file.flix>\n";
        return 1;
    }

    flix::Program program;
    std::string error;

    if (!flix::loadProgram(argv[2], program, error)) {
        std::cerr << "Flix Error: " << error << "\n";
        return 1;
    }

    std::cout << "Flix program structure is valid.\n";
    std::cout << "Labels: " << program.labels.size() << "\n";
    std::cout << "Instructions: "
              << program.instructions.size() << "\n";

    return 0;
}
