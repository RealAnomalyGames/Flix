
#include "flix/program.hpp"

#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <iterator>

namespace flix {

static std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = value.find_last_not_of(" \t\r\n");

    return value.substr(0, last - first + 1);
}

static std::string removeComment(const std::string& line) {
    bool quoted = false;
    bool escaped = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"' && !escaped)
            quoted = !quoted;

        if (c == ';' && !quoted)
            return line.substr(0, i);

        if (c == '\\' && !escaped)
            escaped = true;
        else
            escaped = false;
    }

    return line;
}

static bool validIdentifier(const std::string& name) {
    if (name.empty())
        return false;

    if (!std::isalpha(static_cast<unsigned char>(name[0])) &&
        name[0] != '_')
        return false;

    for (char c : name) {
        unsigned char u = static_cast<unsigned char>(c);

        if (!std::isalnum(u) && c != '_')
            return false;
    }

    return true;
}

static bool validLabel(const std::string& label) {
    return label.size() > 1 &&
           label[0] == '.' &&
           validIdentifier(label.substr(1));
}

static bool validInteger(const std::string& value) {
    if (value.empty())
        return false;

    size_t i = (value[0] == '-' || value[0] == '+') ? 1 : 0;

    if (i == value.size())
        return false;

    for (; i < value.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
            return false;
    }

    return true;
}

static bool validFloat(const std::string& value) {
    if (value.empty())
        return false;

    size_t i = 0;

    if (value[i] == '-' || value[i] == '+')
        ++i;

    bool digits = false;
    bool decimal = false;

    for (; i < value.size(); ++i) {
        char c = value[i];

        if (std::isdigit(static_cast<unsigned char>(c))) {
            digits = true;
        }
        else if (c == '.' && !decimal) {
            decimal = true;
        }
        else {
            return false;
        }
    }

    return digits && decimal;
}

static bool validString(const std::string& value) {
    if (value.size() < 2 ||
        value.front() != '"' ||
        value.back() != '"')
        return false;

    bool escaped = false;

    for (size_t i = 1; i + 1 < value.size(); ++i) {
        char c = value[i];

        if (c == '"' && !escaped)
            return false;

        if (c == '\\' && !escaped)
            escaped = true;
        else
            escaped = false;
    }

    return !escaped;
}

static bool validBoolean(const std::string& value) {
    return value == "true" || value == "false";
}

static bool validValue(
    ValueType type,
    const std::string& value
) {
    switch (type) {
        case ValueType::Integer:
            return validInteger(value);

        case ValueType::Float:
            return validFloat(value);

        case ValueType::String:
            return validString(value);

        case ValueType::Boolean:
            return validBoolean(value);
    }

    return false;
}

std::string valueTypeName(ValueType type) {
    switch (type) {
        case ValueType::Integer: return "int";
        case ValueType::Float: return "float";
        case ValueType::String: return "str";
        case ValueType::Boolean: return "bool";
    }

    return "unknown";
}

static bool parseVariable(
    const std::string& operand,
    int line,
    Program& program,
    std::string& error
) {
    size_t colon = operand.find(':');

    if (colon == std::string::npos) {
        error = "Line " + std::to_string(line) +
                ": Variable declaration requires a type.";

        return false;
    }

    std::string name = trim(operand.substr(0, colon));
    std::string remainder = trim(operand.substr(colon + 1));

    if (!validIdentifier(name)) {
        error = "Line " + std::to_string(line) +
                ": Invalid variable name '" + name + "'.";

        return false;
    }

    if (program.variables.count(name)) {
        error = "Line " + std::to_string(line) +
                ": Duplicate variable '" + name + "'.";

        return false;
    }

    size_t equals = remainder.find('=');

    std::string typeName = trim(
        remainder.substr(0, equals)
    );

    std::string initialValue;

    if (equals != std::string::npos)
        initialValue = trim(remainder.substr(equals + 1));

    ValueType type;

    if (typeName == "int")
        type = ValueType::Integer;
    else if (typeName == "float")
        type = ValueType::Float;
    else if (typeName == "str")
        type = ValueType::String;
    else if (typeName == "bool")
        type = ValueType::Boolean;
    else {
        error = "Line " + std::to_string(line) +
                ": Unknown variable type '" + typeName + "'.";

        return false;
    }

    if (equals != std::string::npos &&
        !validValue(type, initialValue)) {

        error = "Line " + std::to_string(line) +
                ": Invalid initial value for variable '" +
                name + "'.";

        return false;
    }

    program.variables.emplace(
        name,
        Variable{name, type, initialValue, line}
    );

    return true;
}

static size_t findSeparator(const std::string& text) {
    bool quoted = false;
    bool escaped = false;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        if (c == '"' && !escaped)
            quoted = !quoted;

        if (c == ',' && !quoted)
            return i;

        if (c == '\\' && !escaped)
            escaped = true;
        else
            escaped = false;
    }

    return std::string::npos;
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
                    ": Duplicate label '" + label + "'.";

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

        std::istringstream parts(line);

        Instruction instruction;
        instruction.line = lineNumber;

        parts >> instruction.opcode;

        std::getline(parts, instruction.operand);
        instruction.operand = trim(instruction.operand);

        if (instruction.opcode == "var") {
            if (!parseVariable(
                instruction.operand,
                lineNumber,
                program,
                error
            )) {
                return false;
            }

            continue;
        }

        if (instruction.opcode == "jmp") {
            if (!validLabel(instruction.operand)) {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Invalid jump destination.";

                return false;
            }
        }
        else if (instruction.opcode == "halt") {
            if (!instruction.operand.empty() &&
                !validInteger(instruction.operand)) {

                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Invalid halt value.";

                return false;
            }
        }
        else if (instruction.opcode == "outln") {
            if (instruction.operand.empty()) {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Missing output value.";

                return false;
            }

            if (instruction.operand.front() == '"') {
                if (!validString(instruction.operand)) {
                    error = "Line " +
                        std::to_string(lineNumber) +
                        ": Invalid string literal.";

                    return false;
                }
            }
            else if (!validIdentifier(instruction.operand)) {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Invalid output operand.";

                return false;
            }
        }
        else if (instruction.opcode == "mov") {
            size_t comma = findSeparator(instruction.operand);

            if (comma == std::string::npos) {
                error = "Line " +
                    std::to_string(lineNumber) +
                    ": mov requires two operands.";

                return false;
            }

            std::string destination =
                trim(instruction.operand.substr(0, comma));

            std::string sourceValue =
                trim(instruction.operand.substr(comma + 1));

            if (!validIdentifier(destination) ||
                sourceValue.empty()) {

                error = "Line " +
                    std::to_string(lineNumber) +
                    ": Invalid mov operands.";

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

    
    // Validate variable references and assignments.

    for (const auto& instruction : program.instructions) {

        if (instruction.opcode == "outln") {
            const std::string& operand = instruction.operand;

            if (operand.front() != '"' &&
                !program.variables.count(operand)) {

                error = "Line " +
                    std::to_string(instruction.line) +
                    ": Undefined variable '" + operand + "'.";

                return false;
            }
        }

        if (instruction.opcode == "mov") {
            size_t comma = findSeparator(instruction.operand);

            std::string destination = trim(
                instruction.operand.substr(0, comma)
            );

            std::string source = trim(
                instruction.operand.substr(comma + 1)
            );

            auto target = program.variables.find(destination);

            if (target == program.variables.end()) {
                error = "Line " +
                    std::to_string(instruction.line) +
                    ": Undefined variable '" + destination + "'.";

                return false;
            }

            auto sourceVariable = program.variables.find(source);

            if (sourceVariable != program.variables.end()) {
                if (sourceVariable->second.type != target->second.type) {
                    error = "Line " +
                        std::to_string(instruction.line) +
                        ": Variable type mismatch.";

                    return false;
                }
            }
            else if (!validValue(target->second.type, source)) {
                error = "Line " +
                    std::to_string(instruction.line) +
                    ": Invalid value for variable '" +
                    destination + "'.";

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
