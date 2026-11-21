
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Flix Programming Language 0.1.0\n";
        std::cout << "Usage: flix <source.flix>\n";
        return 0;
    }

    std::string filename = argv[1];

    if (filename.size() < 5
        || filename.substr(filename.size() - 5) != ".flix") {

        std::cerr << "Error: Source files must use .flix extension.\n";
        return 1;
    }

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << "\n";
        return 1;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    std::string source = buffer.str();

    flix::Lexer lexer(source);

    auto tokens = lexer.tokenize();

    flix::Parser parser(tokens);

    auto statements = parser.parse();

    if (parser.hasError()) {
        std::cerr << "Flix Syntax Error: "
                  << parser.errorMessage() << "\n";

        return 1;
    }

    std::cout << "Flix 0.1.0\n";
    std::cout << "Source: " << filename << "\n";
    std::cout << "Syntax validation successful.\n";
    std::cout << "Statements: " << statements.size() << "\n";

    return 0;
}
