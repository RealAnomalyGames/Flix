
#include "flix/program.hpp"

#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace flix {
namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

int indentationOf(const std::string& line) {
    int result = 0;

    for (char c : line) {
        if (c == ' ') {
            ++result;
        } else if (c == '\t') {
            result += 4;
        } else {
            break;
        }
    }

    return result;
}

std::string removeComment(const std::string& line) {
    bool quoted = false;
    bool escaped = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (quoted && c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            quoted = !quoted;
            continue;
        }

        if (c == ';' && !quoted) {
            return line.substr(0, i);
        }
    }

    return line;
}

bool validIdentifier(const std::string& name) {
    static const std::regex pattern(
        R"([A-Za-z_][A-Za-z0-9_]*)"
    );

    return std::regex_match(name, pattern);
}

bool validString(const std::string& value) {
    if (value.size() < 2 ||
        value.front() != '"' ||
        value.back() != '"') {
        return false;
    }

    bool escaped = false;

    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        const char c = value[i];

        if (escaped) {
            if (c != 'n' && c != 't' &&
                c != '"' && c != '\\') {
                return false;
            }

            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            return false;
        }
    }

    return !escaped;
}

bool validNumber(const std::string& value, bool& isFloat) {
    static const std::regex integerPattern(
        R"([+-]?[0-9]+)"
    );

    static const std::regex floatPattern(
        R"([+-]?[0-9]+\.[0-9]+)"
    );

    if (std::regex_match(value, integerPattern)) {
        isFloat = false;
        return true;
    }

    if (std::regex_match(value, floatPattern)) {
        isFloat = true;
        return true;
    }

    return false;
}

bool parseType(const std::string& name, ValueType& type) {
    if (name == "int") {
        type = ValueType::Integer;
    } else if (name == "float") {
        type = ValueType::Float;
    } else if (name == "str") {
        type = ValueType::String;
    } else if (name == "bool") {
        type = ValueType::Boolean;
    } else {
        return false;
    }

    return true;
}

bool validLiteral(
    const std::string& value,
    ValueType expected
) {
    if (expected == ValueType::String) {
        return validString(value);
    }

    if (expected == ValueType::Boolean) {
        return value == "true" || value == "false";
    }

    bool isFloat = false;

    if (!validNumber(value, isFloat)) {
        return false;
    }

    return expected != ValueType::Integer || !isFloat;
}

bool parseVariable(
    const std::string& text,
    int line,
    Program& program,
    std::string& error
) {
    const std::string declaration = trim(text.substr(4));
    const auto colon = declaration.find(':');

    if (colon == std::string::npos) {
        error = "Line " + std::to_string(line) +
                ": Expected var name:type.";
        return false;
    }

    const std::string name =
        trim(declaration.substr(0, colon));

    const std::string remainder =
        trim(declaration.substr(colon + 1));

    const auto equals = remainder.find('=');

    const std::string typeName =
        equals == std::string::npos
            ? remainder
            : trim(remainder.substr(0, equals));

    std::string initial =
        equals == std::string::npos
            ? ""
            : trim(remainder.substr(equals + 1));

    if (!validIdentifier(name)) {
        error = "Line " + std::to_string(line) +
                ": Invalid variable name '" + name + "'.";
        return false;
    }

    if (program.variables.contains(name)) {
        error = "Line " + std::to_string(line) +
                ": Duplicate variable '" + name + "'.";
        return false;
    }

    ValueType type;

    if (!parseType(typeName, type)) {
        error = "Line " + std::to_string(line) +
                ": Unknown type '" + typeName + "'.";
        return false;
    }

    if (initial.empty()) {
        switch (type) {
        case ValueType::Integer:
            initial = "0";
            break;
        case ValueType::Float:
            initial = "0.0";
            break;
        case ValueType::String:
            initial = "\"\"";
            break;
        case ValueType::Boolean:
            initial = "false";
            break;
        }
    }

    if (!validLiteral(initial, type)) {
        error = "Line " + std::to_string(line) +
                ": Invalid initial value for '" + name + "'.";
        return false;
    }

    program.variables.emplace(
        name,
        Variable{name, type, initial, line}
    );

    return true;
}

bool isBranchMarker(const Instruction& instruction) {
    return instruction.opcode == "cmp" ||
           instruction.opcode == ;
}

bool isConditionalElse(const Instruction& instruction) {
    return instruction.opcode ==  &&
           instruction.operand.rfind("cmp ", 0) == 0;
}

void buildBranchMetadata(Program& program) {
    auto& instructions = program.instructions;

    int nextBranchId = 0;

    for (std::size_t i = 0; i < instructions.size(); ++i) {
        if (!isBranchMarker(instructions[i])) {
            continue;
        }

        // If this marker already belongs to a previously
        // discovered chain, it has already been processed.
        if (instructions[i].branch_id != -1) {
            continue;
        }

        const int indentation = instructions[i].indentation;
        const int branchId = nextBranchId++;

        std::vector<std::size_t> markers;
        markers.push_back(i);

        std::size_t cursor = i + 1;

        // Find sibling branches at this indentation level.
        // Nested branches have greater indentation and are
        // deliberately ignored by this scan.
        while (cursor < instructions.size()) {
            const auto& candidate = instructions[cursor];

            if (candidate.indentation < indentation) {
                break;
            }

            if (candidate.indentation == indentation) {
                if (isBranchMarker(candidate)) {
                    markers.push_back(cursor);
                    ++cursor;
                    continue;
                }

                // A non-branch instruction at the same
                // indentation ends the conditional chain.
                break;
            }

            ++cursor;
        }

        const std::size_t groupEnd = cursor;

        for (std::size_t m = 0; m < markers.size(); ++m) {
            auto& marker = instructions[markers[m]];

            marker.branch_id = branchId;
            marker.branch_first = (m == 0);
            marker.branch_end = groupEnd;

            marker.branch_next =
                m + 1 < markers.size()
                    ? markers[m + 1]
                    : groupEnd;
        }
    }
}

} // namespace

std::string valueTypeName(ValueType type) {
    switch (type) {
    case ValueType::Integer:
        return "int";
    case ValueType::Float:
        return "float";
    case ValueType::String:
        return "str";
    case ValueType::Boolean:
        return "bool";
    }

    return "unknown";
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

        const std::string line =
            trim(removeComment(rawLine));

        if (line.empty()) {
            continue;
        }

        // Logical continuation lines extend the previous
        // cmp or el cmp condition.
        if (line.rfind("&& ", 0) == 0 ||
            line.rfind("|| ", 0) == 0 ||
            line.rfind("^^ ", 0) == 0) {

            if (program.instructions.empty()) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Logical continuation has no preceding condition.";
                return false;
            }

            auto& previous = program.instructions.back();

            const bool validPrevious =
                previous.opcode == "cmp" ||
                isConditionalElse(previous);

            if (!validPrevious) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Logical continuation must follow cmp or el cmp.";
                return false;
            }

            if (indentation <= previous.indentation) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Logical continuation must be indented beneath its condition.";
                return false;
            }

            previous.operand += " " + line;
            continue;
        }

        if (line.rfind("var ", 0) == 0) {
            if (indentation != 0) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Variable declarations must be top-level.";
                return false;
            }

            if (!parseVariable(
                line,
                lineNumber,
                program,
                error
            )) {
                return false;
            }

            continue;
        }

        if (line.front() == '.') {
            if (indentation != 0 || line.back() != ':') {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid label declaration.";
                return false;
            }

            const std::string label =
                line.substr(1, line.size() - 2);

            if (!validIdentifier(label)) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Invalid label name.";
                return false;
            }

            if (program.labels.contains(label)) {
                error = "Line " + std::to_string(lineNumber) +
                        ": Duplicate label '" + label + "'.";
                return false;
            }

            program.labels[label] =
                program.instructions.size();

            if (label == "start") {
                ++startCount;
                program.entry_point =
                    program.instructions.size();
            }

            continue;
        }

        std::string opcode;
        std::string operand;

        const auto separator = line.find_first_of(" \t");

        if (separator == std::string::npos) {
            opcode = line;
        } else {
            opcode = line.substr(0, separator);
            operand = trim(line.substr(separator + 1));
        }

        if (opcode ==  &&
            operand.rfind("cmp ", 0) == 0) {
            // Keep the "cmp " prefix so the backend can
            // distinguish else-if from unconditional else.
        }

        static const std::unordered_set<std::string> known = {
            "out", "outln", "mov",
            "add", "sub", "mul", "div", "mod",
            "inc", "dec", "neg",
            "cmp", "jmp", "halt"
        };

        if (!known.contains(opcode)) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Unknown instruction '" + opcode + "'.";
            return false;
        }

        if ((opcode == "out" || opcode == "outln" ||
             opcode == "mov" || opcode == "add" ||
             opcode == "sub" || opcode == "mul" ||
             opcode == "div" || opcode == "mod" ||
             opcode == "inc" || opcode == "dec" ||
             opcode == "neg" || opcode == "cmp" ||
             opcode == "jmp" || opcode == "el") &&
            operand.empty()) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Missing instruction operand.";
            return false;
        }

        if (opcode == "jmp" &&
            (operand.size() < 2 || operand.front() != '.')) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Expected jmp .Label.";
            return false;
        }

        if (opcode == "el" &&
            !operand.empty() &&
            operand.rfind("cmp ", 0) != 0) {
            error = "Line " + std::to_string(lineNumber) +
                    ": Expected el cmp condition or el.";
            return false;
        }

        if (opcode == "halt" && !operand.empty()) {
            bool isFloat = false;

            if (!validNumber(operand, isFloat) || isFloat) {
                error = "Line " + std::to_string(lineNumber) +
                        ": halt requires an integer exit code.";
                return false;
            }
        }

        program.instructions.push_back({
            opcode,
            operand,
            lineNumber,
            indentation
        });
    }

    if (startCount != 1) {
        error = "Program must contain exactly one .start: label.";
        return false;
    }

    // Validate all jump targets.
    for (const auto& instruction : program.instructions) {
        if (instruction.opcode != "jmp") {
            continue;
        }

        const std::string label =
            instruction.operand.substr(1);

        if (!program.labels.contains(label)) {
            error = "Line " + std::to_string(instruction.line) +
                    ": Undefined label '" + label + "'.";
            return false;
        }
    }

    buildBranchMetadata(program);

    // Every else/else-if must belong to a chain that starts
    // with cmp. An else cannot be the first branch.
    for (std::size_t i = 0; i < program.instructions.size(); ++i) {
        const auto& instruction = program.instructions[i];

        if (instruction.opcode != "el") {
            continue;
        }

        if (instruction.branch_id < 0) {
            error = "Line " + std::to_string(instruction.line) +
                    ": el does not belong to a conditional chain.";
            return false;
        }

        if (instruction.branch_first) {
            error = "Line " + std::to_string(instruction.line) +
                    ": A conditional chain must begin with cmp.";
            return false;
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

    std::ostringstream contents;
    contents << file.rdbuf();

    return parseProgram(
        contents.str(),
        program,
        error
    );
}

}
