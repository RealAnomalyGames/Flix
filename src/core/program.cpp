#include "flix/program.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace flix {

static std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

static int indentationOf(const std::string& line) {
    int result = 0;

    for (char c : line) {
        if (c == ' ')
            ++result;
        else if (c == '\t')
            result += 4;
        else
            break;
    }

    return result;
}

static std::string removeComment(const std::string& line) {
    bool inString = false;
    bool escaped = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (inString && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"')
            inString = !inString;

        if (c == ';' && !inString)
            return line.substr(0, i);
    }

    return line;
}

static bool validIdentifier(const std::string& value) {
    if (value.empty())
        return false;

    if (!std::isalpha(static_cast<unsigned char>(value[0])) &&
        value[0] != '_')
        return false;

    for (char c : value) {
        if (!std::isalnum(static_cast<unsigned char>(c)) &&
            c != '_')
            return false;
    }

    return true;
}

static bool validLabel(const std::string& value) {
    return value.size() > 1 &&
           value[0] == '.' &&
           validIdentifier(value.substr(1));
}

static bool validInteger(const std::string& value) {
    if (value.empty())
        return false;

    std::size_t i = 0;

    if (value[0] == '-' || value[0] == '+')
        ++i;

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

    std::size_t i = 0;

    if (value[0] == '-' || value[0] == '+')
        ++i;

    bool dot = false;
    bool digit = false;

    for (; i < value.size(); ++i) {
        const char c = value[i];

        if (c == '.' && !dot) {
            dot = true;
            continue;
        }

        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;

        digit = true;
    }

    return dot && digit;
}

static bool validString(const std::string& value) {
    if (value.size() < 2 ||
        value.front() != '"' ||
        value.back() != '"')
        return false;

    bool escaped = false;

    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        const char c = value[i];

        if (escaped) {
            if (c != 'n' && c != 't' &&
                c != '"' && c != '\\')
                return false;

            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"')
            return false;
    }

    return !escaped;
}

static bool parseType(
    const std::string& name,
    ValueType& type
) {
    if (name == "int") {
        type = ValueType::Integer;
        return true;
    }

    if (name == "float") {
        type = ValueType::Float;
        return true;
    }

    if (name == "str") {
        type = ValueType::String;
        return true;
    }

    if (name == "bool") {
        type = ValueType::Boolean;
        return true;
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

static bool validValue(
    const std::string& value,
    ValueType type
) {
    switch (type) {
        case ValueType::Integer:
            return validInteger(value);

        case ValueType::Float:
            return validFloat(value);

        case ValueType::String:
            return validString(value);

        case ValueType::Boolean:
            return value == "true" || value == "false";
    }

    return false;
}

static bool parseVariable(
    const std::string& operand,
    int line,
    Program& program,
    std::string& error
) {
    const auto colon = operand.find(':');

    if (colon == std::string::npos) {
        error = "Line " + std::to_string(line) +
                ": Variable declaration requires a type.";
        return false;
    }

    const std::string name =
        trim(operand.substr(0, colon));

    const std::string remainder =
        trim(operand.substr(colon + 1));

    if (!validIdentifier(name)) {
        error = "Line " + std::to_string(line) +
                ": Invalid variable name '" + name + "'.";
        return false;
    }

    if (program.variables.count(name)) {
        error = "Line " + std::to_string(line) +
                ": Variable '" + name + "' is already declared.";
        return false;
    }

    const auto equals = remainder.find('=');

    const std::string typeName =
        trim(remainder.substr(0, equals));

    ValueType type;

    if (!parseType(typeName, type)) {
        error = "Line " + std::to_string(line) +
                ": Unknown variable type '" + typeName + "'.";
        return false;
    }

    std::string initialValue;

    if (equals == std::string::npos) {
        switch (type) {
            case ValueType::Integer: initialValue = "0"; break;
            case ValueType::Float: initialValue = "0.0"; break;
            case ValueType::String: initialValue = "\"\""; break;
            case ValueType::Boolean: initialValue = "false"; break;
        }
    } else {
        initialValue = trim(remainder.substr(equals + 1));
    }

    if (!validValue(initialValue, type)) {
        error = "Line " + std::to_string(line) +
                ": Initial value does not match type '" +
                typeName + "'.";
        return false;
    }

    program.variables.emplace(
        name,
        Variable{name, type, initialValue, line}
    );

    return true;
}

static bool isOpcode(const std::string& opcode) {
    static const std::vector<std::string> opcodes = {
        "mov", "out", "outln",
        "jmp", "halt",
        "add", "sub", "mul", "div", "mod",
        "inc", "dec", "neg",
        "cmp", "el"
    };

    return std::find(
        opcodes.begin(),
        opcodes.end(),
        opcode
    ) != opcodes.end();
}

static bool isArithmetic(const std::string& opcode) {
    return opcode == "add" ||
           opcode == "sub" ||
           opcode == "mul" ||
           opcode == "div" ||
           opcode == "mod" ||
           opcode == "inc" ||
           opcode == "dec" ||
           opcode == "neg";
}

static bool isLogicalContinuation(const std::string& line) {
    return line.rfind("&& ", 0) == 0 ||
           line.rfind("|| ", 0) == 0 ||
           line.rfind("^^ ", 0) == 0;
}

static bool findComparisonOperator(
    const std::string& expression,
    std::size_t& position,
    std::string& operation
) {
    static const std::vector<std::string> operators = {
        "==", "!=", ">=", "<=", ">", "<"
    };

    bool inString = false;
    bool escaped = false;

    for (std::size_t i = 0; i < expression.size(); ++i) {
        const char c = expression[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (inString && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"')
            inString = !inString;

        if (inString)
            continue;

        for (const auto& op : operators) {
            if (expression.compare(i, op.size(), op) == 0) {
                position = i;
                operation = op;
                return true;
            }
        }
    }

    return false;
}

bool parseProgram(
    const std::string& source,
    Program& program,
    std::string& error
) {
    program = Program{};
    error.clear();

    std::istringstream input(source);
    std::string rawLine;

    int lineNumber = 0;
    int startCount = 0;

    while (std::getline(input, rawLine)) {
        ++lineNumber;

        const int indentation = indentationOf(rawLine);
        std::string line = trim(removeComment(rawLine));

        if (line.empty())
            continue;

        // Logical operators continue the preceding comparison.
        if (isLogicalContinuation(line)) {
            if (program.instructions.empty()) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Logical operator has no preceding comparison.";
                return false;
            }

            Instruction& previous = program.instructions.back();

            if (previous.opcode != "cmp" &&
                previous.opcode != "elcmp") {
                error = "Line " + std::to_string(lineNumber) +
                        ": Logical operators can only continue comparisons.";
                return false;
            }

            if (previous.indentation != indentation) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Logical continuation indentation does not match.";
                return false;
            }

            previous.operand += " " + line;
            continue;
        }

        if (line.front() == '.') {
            if (indentation != 0) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Labels cannot be indented.";
                return false;
            }

            if (line.back() != ':') {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid label declaration.";
                return false;
            }

            const std::string label =
                line.substr(0, line.size() - 1);

            if (!validLabel(label)) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid label '" + label + "'.";
                return false;
            }

            if (program.labels.count(label)) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Duplicate label '" + label + "'.";
                return false;
            }

            program.labels[label] = program.instructions.size();

            if (label == ".start")
                ++startCount;

            continue;
        }

        std::istringstream parts(line);

        std::string opcode;
        parts >> opcode;

        std::string operand;
        std::getline(parts, operand);
        operand = trim(operand);

        if (opcode == "var") {
            if (indentation != 0) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Variable declarations must be unindented.";
                return false;
            }

            if (!parseVariable(
                    operand,
                    lineNumber,
                    program,
                    error
                ))
                return false;

            continue;
        }

        if (opcode == "el") {
            if (operand.rfind("cmp ", 0) == 0) {
                opcode = "elcmp";
                operand = trim(operand.substr(4));
            } else if (!operand.empty()) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid else instruction.";
                return false;
            }
        }

        if (!isOpcode(opcode) && opcode != "elcmp") {
            error = "Line " + std::to_string(lineNumber) +
                    ": Unknown instruction '" + opcode + "'.";
            return false;
        }

        if (opcode == "jmp" && !validLabel(operand)) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Invalid jump target.";
            return false;
        }

        if (opcode == "halt" &&
            !operand.empty() &&
            !validInteger(operand)) {
            error = "Line " + std::to_string(lineNumber) +
                    ": halt requires an integer exit code.";
            return false;
        }

        if (isArithmetic(opcode)) {
            if (opcode == "inc" ||
                opcode == "dec" ||
                opcode == "neg") {
                if (!validIdentifier(operand)) {
                    error = "Line " + std::to_string(lineNumber) +
                            ": Expected a variable name.";
                    return false;
                }
            } else {
                const auto equals = operand.find('=');

                if (equals == std::string::npos ||
                    trim(operand.substr(0, equals)).empty() ||
                    trim(operand.substr(equals + 1)).empty()) {
                    error = "Line " + std::to_string(lineNumber) +
                            ": Invalid arithmetic instruction.";
                    return false;
                }
            }
        }

        if ((opcode == "cmp" || opcode == "elcmp") &&
            operand.empty()) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Comparison requires an expression.";
            return false;
        }

        if ((opcode == "mov" || opcode == "out" ||
             opcode == "outln") && operand.empty()) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Missing instruction operand.";
            return false;
        }

        program.instructions.push_back({
            opcode,
            operand,
            lineNumber,
            indentation
        });
    }

    if (startCount != 1) {
        error = "Program must contain exactly one '.start:' label.";
        return false;
    }

    for (const auto& instruction : program.instructions) {
        if (instruction.opcode == "jmp" &&
            !program.labels.count(instruction.operand)) {
            error = "Line " + std::to_string(instruction.line) +
                    ": Undefined label '" +
                    instruction.operand + "'.";
            return false;
        }
    }

    program.entry_point = program.labels.at(".start");

    return true;
}

bool loadProgram(
    const std::string& path,
    Program& program,
    std::string& error
) {
    std::ifstream file(path);

    if (!file) {
        error = "Unable to open file '" + path + "'.";
        return false;
    }

    std::ostringstream contents;
    contents << file.rdbuf();

    return parseProgram(contents.str(), program, error);
}

}
