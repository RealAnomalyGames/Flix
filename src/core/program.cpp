
#include "flix/program.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace flix {

static std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

static std::string removeComment(const std::string& line) {
    bool quoted = false;

    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '"' && (i == 0 || line[i - 1] != '\\'))
            quoted = !quoted;

        if (line[i] == ';' && !quoted)
            return line.substr(0, i);
    }

    return line;
}

static bool validLabel(const std::string& label) {
    if (label.size() < 2 || label[0] != '.')
        return false;

    if (!std::isalpha(
        static_cast<unsigned char>(label[1])) &&
        label[1] != '_')
        return false;

    for (size_t i = 2; i < label.size(); ++i) {
        unsigned char c =
            static_cast<unsigned char>(label[i]);

        if (!std::isalnum(c) && c != '_')
            return false;
    }

    return true;
}

bool parseProgram(
    const std::string& source,
    Program& program,
    std::string& error
) {
    program = Program{};
    error.clear();

    std::istringstream input(source);
    std::string line;

    int lineNumber = 0;
    int startCount = 0;

    while (std::getline(input, line)) {
        ++lineNumber;

        line = trim(removeComment(line));

        if (line.empty())
            continue;

        // Label declaration
        if (line.front() == '.') {
            if (line.back() != ':') {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Invalid label declaration.";

                return false;
            }

            std::string label =
                line.substr(0, line.size() - 1);

            if (!validLabel(label)) {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Invalid label name.";

                return false;
            }

            if (program.labels.count(label)) {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Duplicate label '" +
                    label + "'.";

                return false;
            }

            program.labels[label] =
                program.instructions.size();

            if (label == ".start") {
                ++startCount;

                program.entry_point =
                    program.instructions.size();
            }

            continue;
        }

        // Instruction parsing
        std::istringstream parts(line);

        Instruction instruction;
        instruction.line = lineNumber;

        parts >> instruction.opcode;

        std::getline(parts, instruction.operand);

        instruction.operand = trim(instruction.operand);

        if (instruction.opcode == "jmp") {
            if (!validLabel(instruction.operand)) {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Invalid jump destination.";

                return false;
            }
        }
        else if (instruction.opcode == "halt") {
            if (!instruction.operand.empty()) {
                try {
                    size_t parsed = 0;

                    std::stoll(
                        instruction.operand,
                        &parsed
                    );

                    if (parsed != instruction.operand.size())
                        throw std::invalid_argument("Invalid value");
                }
                catch (...) {
                    error = "Line " +
                        std::to_string(lineNumber) +
                        ": Invalid halt value.";

                    return false;
                }
            }
        }
        else if (instruction.opcode == "outln") {
            const std::string& value = instruction.operand;

            if (value.size() < 2 ||
                value.front() != '"' ||
                value.back() != '"') {

                error = "Line " +
                    std::to_string(lineNumber) +
                    ": outln requires a quoted string.";

                return false;
            }
        }
        else {
            error = "Line " +
                std::to_string(lineNumber) +
                ": Unknown instruction '" +
                instruction.opcode + "'.";

            return false;
        }

        program.instructions.push_back(instruction);
    }

    if (startCount != 1) {
        error = startCount == 0
            ? "Program is missing the .start: entry point."
            : "Program contains multiple .start: entry points.";

        return false;
    }

    // Validate jump destinations after all labels
    // have been registered.

    for (const auto& instruction : program.instructions) {
        if (instruction.opcode == "jmp") {
            if (!program.labels.count(instruction.operand)) {
                error = "Line " +
                    std::to_string(instruction.line) +
                    ": Undefined label '" +
                    instruction.operand + "'.";

                return false;
            }
        }
    }

    return true;
}

bool loadProgram(
    const std::string& path,
    Program& program,
    std::string& error
) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        error = "Unable to open source file: " + path;
        return false;
    }

    std::string source(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return parseProgram(source, program, error);
}

}
