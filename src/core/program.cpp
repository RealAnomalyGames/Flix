#include "flix/program.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace flix {

static std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = s.find_last_not_of(" \t\r\n");

    return s.substr(first, last - first + 1);
}

static int indentationOf(const std::string& s) {
    int result = 0;

    for (char c : s) {
        if (c == ' ')
            ++result;
        else if (c == '\t')
            result += 4;
        else
            break;
    }

    return result;
}

static std::string removeComment(const std::string& s) {
    bool quoted = false;
    bool escaped = false;

    for (std::size_t i = 0; i < s.size(); ++i) {
        char c = s[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (quoted && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"')
            quoted = !quoted;

        if (c == ';' && !quoted)
            return s.substr(0, i);
    }

    return s;
}

static bool identifier(const std::string& s) {
    if (s.empty())
        return false;

    if (!(std::isalpha(static_cast<unsigned char>(s[0])) ||
          s[0] == '_'))
        return false;

    for (char c : s) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_'))
            return false;
    }

    return true;
}

static bool labelName(const std::string& s) {
    return s.size() > 1 && s[0] == '.' &&
           identifier(s.substr(1));
}

static bool integerLiteral(const std::string& s) {
    if (s.empty())
        return false;

    std::size_t i = 0;

    if (s[i] == '+' || s[i] == '-')
        ++i;

    if (i == s.size())
        return false;

    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    }

    return true;
}

static bool floatLiteral(const std::string& s) {
    if (s.empty())
        return false;

    std::size_t i = 0;

    if (s[i] == '+' || s[i] == '-')
        ++i;

    bool dot = false;
    bool digit = false;

    for (; i < s.size(); ++i) {
        if (s[i] == '.' && !dot) {
            dot = true;
            continue;
        }

        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;

        digit = true;
    }

    return dot && digit;
}

static bool stringLiteral(const std::string& s) {
    if (s.size() < 2 || s.front() != '"' || s.back() != '"')
        return false;

    bool escaped = false;

    for (std::size_t i = 1; i + 1 < s.size(); ++i) {
        char c = s[i];

        if (escaped) {
            if (c != 'n' && c != 't' && c != '"' && c != '\\')
                return false;

            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            return false;
        }
    }

    return !escaped;
}

static bool booleanLiteral(const std::string& s) {
    return s == "true" || s == "false";
}

static bool parseType(const std::string& s, ValueType& type) {
    if (s == "int") {
        type = ValueType::Integer;
        return true;
    }

    if (s == "float") {
        type = ValueType::Float;
        return true;
    }

    if (s == "str") {
        type = ValueType::String;
        return true;
    }

    if (s == "bool") {
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

static std::size_t findOutsideString(
    const std::string& s,
    const std::string& target
) {
    bool quoted = false;
    bool escaped = false;

    for (std::size_t i = 0; i + target.size() <= s.size(); ++i) {
        char c = s[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (quoted && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"')
            quoted = !quoted;

        if (!quoted && s.compare(i, target.size(), target) == 0)
            return i;
    }

    return std::string::npos;
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
    const std::string rest = trim(operand.substr(colon + 1));

    if (!identifier(name)) {
        error = "Line " + std::to_string(line) +
                ": Invalid variable name '" + name + "'.";
        return false;
    }

    if (program.variables.count(name)) {
        error = "Line " + std::to_string(line) +
                ": Variable '" + name + "' is already declared.";
        return false;
    }

    const auto equals = rest.find('=');

    const std::string typeText = trim(rest.substr(0, equals));

    ValueType type;

    if (!parseType(typeText, type)) {
        error = "Line " + std::to_string(line) +
                ": Unknown type '" + typeText + "'.";
        return false;
    }

    std::string initial;

    if (equals != std::string::npos) {
        initial = trim(rest.substr(equals + 1));
    } else {
        switch (type) {
            case ValueType::Integer: initial = "0"; break;
            case ValueType::Float: initial = "0.0"; break;
            case ValueType::String: initial = "\"\""; break;
            case ValueType::Boolean: initial = "false"; break;
        }
    }

    bool valid = false;

    switch (type) {
        case ValueType::Integer: valid = integerLiteral(initial); break;
        case ValueType::Float: valid = floatLiteral(initial); break;
        case ValueType::String: valid = stringLiteral(initial); break;
        case ValueType::Boolean: valid = booleanLiteral(initial); break;
    }

    if (!valid) {
        error = "Line " + std::to_string(line) +
                ": Initial value does not match type '" + typeText + "'.";
        return false;
    }

    program.variables.emplace(
        name,
        Variable{name, type, initial, line}
    );

    return true;
}

static bool knownOpcode(const std::string& op) {
    static const std::vector<std::string> names = {
        "mov", "out", "outln", "jmp", "halt",
        "add", "sub", "mul", "div", "mod",
        "inc", "dec", "neg", "cmp", "el"
    };

    return std::find(names.begin(), names.end(), op) != names.end();
}

static bool arithmeticOpcode(const std::string& op) {
    return op == "add" || op == "sub" ||
           op == "mul" || op == "div" ||
           op == "mod" || op == "inc" ||
           op == "dec" || op == "neg";
}

bool parseProgram(
    const std::string& source,
    Program& program,
    std::string& error
) {
    program = Program{};
    error.clear();

    std::istringstream input(source);
    std::string raw;
    int lineNumber = 0;
    int starts = 0;

    while (std::getline(input, raw)) {
        ++lineNumber;

        const int indent = indentationOf(raw);
        std::string line = trim(removeComment(raw));

        if (line.empty())
            continue;

        if (line[0] == '.') {
            if (indent != 0 || line.back() != ':') {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid label declaration or indentation.";
                return false;
            }

            std::string name = line.substr(0, line.size() - 1);

            if (!labelName(name)) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid label.";
                return false;
            }

            if (program.labels.count(name)) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Duplicate label '" + name + "'.";
                return false;
            }

            program.labels[name] = program.instructions.size();

            if (name == ".start")
                ++starts;

            continue;
        }

        std::istringstream parts(line);
        std::string opcode;
        parts >> opcode;

        std::string operand;
        std::getline(parts, operand);
        operand = trim(operand);

        if (opcode == "var") {
            if (indent != 0) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Variable declarations must be unindented.";
                return false;
            }

            if (!parseVariable(operand, lineNumber, program, error))
                return false;

            continue;
        }

        if (opcode == "&&" || opcode == "||" || opcode == "^^") {
            program.instructions.push_back({
                opcode,
                operand,
                lineNumber,
                indent
            });
            continue;
        }

        if (!knownOpcode(opcode)) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Unknown instruction '" + opcode + "'.";
            return false;
        }

        if (opcode == "jmp" && !labelName(operand)) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Invalid jump target.";
            return false;
        }

        if (opcode == "halt" &&
            !operand.empty() && !integerLiteral(operand)) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Invalid halt exit code.";
            return false;
        }

        if (opcode == "cmp" && operand.empty()) {
            error = "Line " + std::to_string(lineNumber) +
                    ": cmp requires a condition.";
            return false;
        }

        if (opcode == "el" &&
            !operand.empty() && operand.rfind("cmp ", 0) != 0) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Expected 'el' or 'el cmp'.";
            return false;
        }

        if (arithmeticOpcode(opcode)) {
            if (opcode == "inc" || opcode == "dec" || opcode == "neg") {
                if (!identifier(operand)) {
                    error = "Line " + std::to_string(lineNumber) +
                            ": Expected a variable name.";
                    return false;
                }
            } else {
                const auto equals = findOutsideString(operand, "=");

                if (equals == std::string::npos ||
                    trim(operand.substr(0, equals)).empty() ||
                    trim(operand.substr(equals + 1)).empty()) {
                    error = "Line " + std::to_string(lineNumber) +
                            ": Expected destination = value.";
                    return false;
                }
            }
        }

        if ((opcode == "mov" || opcode == "out" ||
             opcode == "outln") && operand.empty()) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Missing operand.";
            return false;
        }

        program.instructions.push_back({
            opcode,
            operand,
            lineNumber,
            indent
        });

        // Logical continuation lines are attached to the preceding cmp.
        // They are represented as cmp instructions with a continuation
        // expression, so the interpreter can evaluate them together.
        if (opcode == "cmp") {
            // Continuations are merged in the second pass below.
        }
    }

    if (starts != 1) {
        error = "Program must contain exactly one '.start:' label.";
        return false;
    }

    // Merge logical continuation lines into the preceding cmp.
    std::vector<Instruction> merged;

    for (const auto& ins : program.instructions) {
        if (ins.opcode == "&&" || ins.opcode == "||" ||
            ins.opcode == "^^") {
            if (merged.empty() || merged.back().opcode != "cmp") {
                error = "Line " + std::to_string(ins.line) +
                        ": Logical continuation must follow cmp.";
                return false;
            }

            merged.back().operand += " " + ins.opcode + " " + ins.operand;
            continue;
        }

        merged.push_back(ins);
    }

    program.instructions = std::move(merged);

    for (const auto& ins : program.instructions) {
        if (ins.opcode == "jmp" &&
            !program.labels.count(ins.operand)) {
            error = "Line " + std::to_string(ins.line) +
                    ": Undefined label '" + ins.operand + "'.";
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
