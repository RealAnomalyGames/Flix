
#include "flix/program.hpp"
#include "flix/version.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

namespace {

void printUsage() {
    std::cout
        << "Flix Compiler " << flix::version() << "\n\n"
        << "Usage:\n"
        << "  flixc version\n"
        << "  flixc check <file.flix>\n"
        << "  flixc asm <file.flix> -o <file.asm>\n"
        << "  flixc build <file.flix> -o <file.exe>\n";
}

bool writeFile(
    const fs::path& path,
    const std::string& contents,
    std::string& error
) {
    std::ofstream output(path, std::ios::binary);

    if (!output) {
        error = "Could not create output file: " + path.string();
        return false;
    }

    output << contents;

    if (!output.good()) {
        error = "Failed while writing output file: " + path.string();
        return false;
    }

    return true;
}

bool loadAndValidate(
    const fs::path& sourcePath,
    flix::Program& program
) {
    std::string error;

    if (!flix::loadProgram(sourcePath.string(), program, error)) {
        std::cerr << "Compilation failed:\n"
                  << error << "\n";
        return false;
    }

    return true;
}

std::string serializeVariable(const flix::Variable& variable) {
    std::ostringstream output;

    output << ".var "
           << variable.name << " "
           << flix::valueTypeName(variable.type)
           << " ";

    if (variable.type == flix::ValueType::String) {
        output << '"';

        for (char c : variable.initial_value) {
            switch (c) {
                case '\\':
                    output << "\\\\";
                    break;
                case '"':
                    output << "\\\"";
                    break;
                case '\n':
                    output << "\\n";
                    break;
                case '\t':
                    output << "\\t";
                    break;
                default:
                    output << c;
                    break;
            }
        }

        output << '"';
    } else {
        output << variable.initial_value;
    }

    return output.str();
}

std::string generateAssembly(const flix::Program& program) {
    std::ostringstream output;

    std::unordered_map<std::size_t, std::string> labelsAtPosition;

    for (const auto& [name, position] : program.labels) {
        labelsAtPosition[position] = name;
    }

    output
        << "; Flix Intermediate Assembly\n"
        << "; Compiler version: " << flix::version() << "\n"
        << "; This file is a readable Flix IR listing.\n"
        << "; It is not a native CPU assembly file.\n\n";

    output << ".flixasm 1\n";
    output << ".entry " << program.entry_point << "\n\n";

    output << "; Variable declarations\n";

    for (const auto& [name, variable] : program.variables) {
        output << serializeVariable(variable) << "\n";
    }

    output << "\n; Program instructions\n";

    for (std::size_t i = 0; i < program.instructions.size(); ++i) {
        const auto label = labelsAtPosition.find(i);

        if (label != labelsAtPosition.end()) {
            output << "\n." << label->second << ":\n";
        }

        const flix::Instruction& instruction =
            program.instructions[i];

        output << "    "
               << instruction.opcode;

        if (!instruction.operand.empty()) {
            output << " " << instruction.operand;
        }

        output
            << "    ; source line "
            << instruction.line
            << "\n";

        if (instruction.branch_id >= 0) {
            output
                << "    ; branch_id="
                << instruction.branch_id
                << " branch_first="
                << (instruction.branch_first ? "true" : "false")
                << " branch_next="
                << instruction.branch_next
                << " branch_end="
                << instruction.branch_end
                << "\n";
        }
    }

    const auto endLabel = labelsAtPosition.find(
        program.instructions.size()
    );

    if (endLabel != labelsAtPosition.end()) {
        output << "\n." << endLabel->second << ":\n";
    }

    output << "\n; End of Flix program\n";

    return output.str();
}

int commandCheck(const fs::path& sourcePath) {
    flix::Program program;

    if (!loadAndValidate(sourcePath, program)) {
        return 1;
    }

    std::cout
        << "Successfully checked "
        << sourcePath.filename().string()
        << "\n";

    std::cout
        << "Instructions: "
        << program.instructions.size()
        << "\n";

    std::cout
        << "Variables: "
        << program.variables.size()
        << "\n";

    std::cout
        << "Labels: "
        << program.labels.size()
        << "\n";

    return 0;
}

int commandAssembly(
    const fs::path& sourcePath,
    const fs::path& outputPath
) {
    flix::Program program;

    if (!loadAndValidate(sourcePath, program)) {
        return 1;
    }

    const std::string assembly = generateAssembly(program);
    std::string error;

    if (!writeFile(outputPath, assembly, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::cout
        << "Generated Flix intermediate assembly:\n"
        << outputPath.string()
        << "\n";

    return 0;
}

int commandBuild() {
    std::cerr
        << "Native executable generation is temporarily unavailable.\n\n"
        << "The previous executable-generation implementation has "
           "been removed because it copied the compiler executable "
           "and appended program data to it.\n\n"
        << "That design can trigger antivirus detections and is not "
           "suitable for distributing Flix programs.\n\n"
        << "No executable was created.\n";

    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    const std::string command = argv[1];

    if (command == "version") {
        std::cout << "Flix Compiler " << flix::version() << "\n";
        return 0;
    }

    if (command == "check") {
        if (argc != 3) {
            printUsage();
            return 1;
        }

        return commandCheck(argv[2]);
    }

    if (command == "asm") {
        if (argc != 5 || std::string(argv[3]) != "-o") {
            printUsage();
            return 1;
        }

        return commandAssembly(argv[2], argv[4]);
    }

    if (command == "build") {
        if (argc != 5 || std::string(argv[3]) != "-o") {
            printUsage();
            return 1;
        }

        return commandBuild();
    }

    std::cerr << "Unknown command: " << command << "\n\n";
    printUsage();

    return 1;
}
