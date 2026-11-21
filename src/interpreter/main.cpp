
#include <iostream>
#include <string>

#include "flix/version.hpp"
#include "flix/program.hpp"

static std::string decodeString(const std::string& value) {
    std::string result;

    for (size_t i = 1; i + 1 < value.size(); ++i) {
        if (value[i] == '\\' && i + 2 < value.size()) {
            ++i;

            switch (value[i]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default:
                    result += '\\';
                    result += value[i];
                    break;
            }
        }
        else {
            result += value[i];
        }
    }

    return result;
}

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

    if (command != "run" || argc < 3) {
        std::cerr << "Usage: flix run <file.flix>\n";
        return 1;
    }

    flix::Program program;
    std::string error;

    if (!flix::loadProgram(argv[2], program, error)) {
        std::cerr << "Flix Error: " << error << "\n";
        return 1;
    }

    size_t pc = program.entry_point;

    while (pc < program.instructions.size()) {
        const auto& instruction =
            program.instructions[pc];

        if (instruction.opcode == "outln") {
            std::cout
                << decodeString(instruction.operand)
                << '\n';

            ++pc;
        }
        else if (instruction.opcode == "jmp") {
            pc = program.labels.at(instruction.operand);
        }
        else if (instruction.opcode == "halt") {
            if (instruction.operand.empty())
                return 0;

            return static_cast<int>(
                std::stoll(instruction.operand)
            );
        }
        else {
            std::cerr
                << "Flix Error: Unsupported instruction at line "
                << instruction.line << "\n";

            return 1;
        }
    }

    // Reaching the end of the program without
    // an explicit halt is an error in 0.1.0.

    std::cerr << "Flix Error: Program ended without halt.\n";

    return 1;
}
