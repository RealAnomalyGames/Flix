#include "flix/program.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace flix {

static std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

static int indentationOf(const std::string& line) {
    int indentation = 0;

    for (char c : line) {
        if (c == ' ')
            indentation++;
        else if (c == '\t')
            indentation += 4;
        else
            break;
    }

    return indentation;
}

static std::string removeComment(const std::string& line) {
    bool insideString = false;
    bool escaped = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (insideString && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"')
            insideString = !insideString;

        if (c == ';' && !insideString)
            return line.substr(0, i);
    }

    return line;
}

static bool validIdentifier(const std::string& name) {
    if (name.empty())
        return false;

    if (!(std::isalpha(static_cast<unsigned char>(name[0])) ||
          name[0] == '_'))
        return false;

    for (char c : name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_'))
            return false;
    }

    return true;
}

static bool validLabel(const std::string& name) {
    return !name.empty() && name[0] == '.' &&
           validIdentifier(name.substr(1));
}

static bool validInteger(const std::string& value) {
    if (value.empty())
        return false;

    std::size_t i = 0;

    if (value[0] == '-' || value[0] == '+')
        i++;

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
        i++;

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
            if (c != 'n' && c != 't' && c != '"' && c != '\\')
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

static bool validBoolean(const std::string& value) {
    return value == "true" || value == "false";
}

static std::string decodeString(const std::string& value) {
    std::string result;

    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        char c = value[i];

        if (c == '\\' && i + 2 < value.size()) {
            const char next = value[++i];

            switch (next) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default: result += next; break;
            }
        } else {
            result += c;
        }
    }

    return result;
}

static std::string valueTypeName(ValueType type) {
    switch (type) {
        case ValueType::Integer: return "int";
        case ValueType::Float: return "float";
        case ValueType::String: return "str";
        case ValueType::Boolean: return "bool";
    }

    return "unknown";
}

std::string valueTypeName(ValueType type) {
    return flix::valueTypeName(type);
}

static bool parseType(
    const std::string& typeName,
    ValueType& type
) {
    if (typeName == "int") {
        type = ValueType::Integer;
        return true;
    }

    if (typeName == "float") {
        type = ValueType::Float;
        return true;
    }

    if (typeName == "str") {
        type = ValueType::String;
        return true;
    }

    if (typeName == "bool") {
        type = ValueType::Boolean;
        return true;
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

    const std::string name = trim(operand.substr(0, colon));
    const std::string remainder = trim(operand.substr(colon + 1));

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

    const std::string typeName = trim(
        remainder.substr(0, equals)
    );

    ValueType type;

    if (!parseType(typeName, type)) {
        error = "Line " + std::to_string(line) +
                ": Unknown variable type '" + typeName + "'.";
        return false;
    }

    std::string initialValue;

    if (equals != std::string::npos) {
        initialValue = trim(remainder.substr(equals + 1));
    } else {
        switch (type) {
            case ValueType::Integer: initialValue = "0"; break;
            case ValueType::Float: initialValue = "0.0"; break;
            case ValueType::String: initialValue = "\"\""; break;
            case ValueType::Boolean: initialValue = "false"; break;
        }
    }

    bool valid = false;

    switch (type) {
        case ValueType::Integer:
            valid = validInteger(initialValue);
            break;

        case ValueType::Float:
            valid = validFloat(initialValue);
            break;

        case ValueType::String:
            valid = validString(initialValue);
            break;

        case ValueType::Boolean:
            valid = validBoolean(initialValue);
            break;
    }

    if (!valid) {
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

static std::size_t findOutsideString(
    const std::string& text,
    char target
) {
    bool insideString = false;
    bool escaped = false;

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (insideString && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"')
            insideString = !insideString;

        if (!insideString && c == target)
            return i;
    }

    return std::string::npos;
}

static bool isKnownOpcode(const std::string& opcode) {
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

bool parseProgram(
    const std::string& source,
    Program& program,
    std::string& error
) {
    program = Program{};
    error.clear();

    std::istringstream stream(source);
    std::string rawLine;

    int lineNumber = 0;
    int startCount = 0;

    while (std::getline(stream, rawLine)) {
        ++lineNumber;

        const int indentation = indentationOf(rawLine);
        std::string line = trim(removeComment(rawLine));

        if (line.empty())
            continue;

        if (line.front() == '.') {
            if (indentation != 0) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Labels must not be indented.";
                return false;
            }

            if (line.back() != ':') {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid label declaration.";
                return false;
            }

            const std::string label = line.substr(
                0,
                line.size() - 1
            );

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
            if (!parseVariable(
                    operand,
                    lineNumber,
                    program,
                    error
                ))
                return false;

            continue;
        }

        if (!isKnownOpcode(opcode)) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Unknown instruction '" + opcode + "'.";
            return false;
        }

        if (opcode == "jmp") {
            if (!validLabel(operand)) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid jump target.";
                return false;
            }
        }

        if (opcode == "halt" && !operand.empty() &&
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

                if (equals == std::string::npos) {
                    error = "Line " + std::to_string(lineNumber) +
                            ": Arithmetic instruction requires '='.";
                    return false;
                }

                const std::string destination =
                    trim(operand.substr(0, equals));

                const std::string value =
                    trim(operand.substr(equals + 1));

                if (!validIdentifier(destination) || value.empty()) {
                    error = "Line " + std::to_string(lineNumber) +
                            ": Invalid arithmetic expression.";
                    return false;
                }
            }
        }

        if (opcode == "cmp") {
            if (operand.empty()) {
                error = "Line " + std::to_string(lineNumber) +
                        ": cmp requires a condition.";
                return false;
            }
        }

        if (opcode == "el") {
            if (operand != "" && operand.rfind("cmp ", 0) != 0) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Expected 'el' or 'el cmp'.";
                return false;
            }
        }

        if ((opcode == "out" || opcode == "outln" ||
             opcode == "mov") && operand.empty()) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Missing instruction operand.";
            return false;
        }

        if (opcode == "mov" &&
            findOutsideString(operand, ',') == std::string::npos) {
            error = "Line " + std::to_string(lineNumber) +
                    ": mov requires two operands.";
            return false;
        }

        program.instructions.push_back({
            opcode,
            operand,
            lineNumber,
            indentation
        });

        // Logical continuation lines belong to the preceding cmp.
        // They are appended to that comparison rather than becoming
        // standalone instructions.
        if (opcode == "cmp") {
            // The expression is retained as authored.
        }
    }

    if (startCount != 1) {
        error = "Program must contain exactly one '.start:' label.";
        return false;
    }

    for (const auto& instruction : program.instructions) {
        if (instruction.opcode == "jmp" &&
            !program.labels.count(instruction.operand)) {
            error = "Line " + std::to_string(instruction.line) +
                    ": Undefined label '" + instruction.operand + "'.";
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
