#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "Flix Interpreter 0.1.0\n";
    std::cout << "Architecture: x86_64\n";

    if (argc < 2)
    {
        std::cout << "Usage: flix-interpreter <file.flix>\n";
        return 1;
    }

    std::cout << "Source file: " << argv[1] << '\n';

    // Lexer and parser will be connected here.

    return 0;
}
