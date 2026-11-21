#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "Flix Compiler 0.1.0\n";
    std::cout << "Target: Windows x86_64\n";

    if (argc < 2)
    {
        std::cout << "Usage: flix-compiler <file.flix>\n";
        return 1;
    }

    std::cout << "Source file: " << argv[1] << '\n';

    // Lexer, parser, semantic analysis,
    // and native code generation will be connected here.

    return 0;
}
