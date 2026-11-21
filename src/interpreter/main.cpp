
#include <iostream>
#include <string>
#include <unordered_map>
#include <cctype>

#include "flix/version.hpp"
#include "flix/program.hpp"

struct RuntimeValue {
    flix::ValueType type;
    std::string data;
};

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

static std::string defaultValue(flix::ValueType type) {
    switch (type) {
        case flix::ValueType::Integer: return "0";
        case flix::ValueType::Float: return "0.0";
        case flix::ValueType::String: return "";
        case flix::ValueType::Boolean: return "false";
    }

    return "";
}

static bool isInteger(const std::string& value) {
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

static bool isFloat(const std::string& value) {
    if (value.empty())
        return false;

    size_t i = 0;

    if (value[i] == '-' || value[i] == '+')
        ++i;

    bool digits = false;
    bool decimal = false;

    for (; i < value.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(value[i]))) {
            digits = true;
        }
        else if (value[i] == '.' && !decimal) {
            decimal = true;
        }
        else {
            return false;
        }
    }

    return digits && decimal;
}

static bool validLiteral(
    flix::ValueType type,
    const std::string& value
) {
    switch (type) {
        case flix::ValueType::Integer:
            return isInteger(value);

        case flix::ValueType::Float:
            return isFloat(value);

        case flix::ValueType::String:
            return value.size() >= 2 &&
                   value.front() == '"' &&
                   value.back() == '"';

        case flix::ValueType::Boolean:
            return value == "true" || value == "false";
    }

    return false;
}

static int runtimeError(
    int line,
    const std::string& message
) {
    std::cerr << "Flix Runtime Error: Line "
              << line << ": " << message << "\n";

    return 1;
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

    // Initialize global variable storage.

    std::unordered_map<std::string, RuntimeValue> variables;

    for (const auto& [name, declaration] : program.variables) {
        std::string value = declaration.initial_value;

        if (value.empty())
            value = defaultValue(declaration.type);

        if (declaration.type == flix::ValueType::String &&
            declaration.initial_value.empty()) {
            value = "";
        }
        else if (declaration.type == flix::ValueType::String) {
            value = decodeString(value);
        }

        variables.emplace(
            name,
            RuntimeValue{declaration.type, value}
        );
    }

    // Begin execution.

    size_t pc = program.entry_point;

    while (pc < program.instructions.size()) {
        const auto& instruction = program.instructions[pc];

        if (instruction.opcode == "outln") {
            const std::string& operand = instruction.operand;

            if (operand.front() == '"') {
                std::cout << decodeString(operand) << '\n';
            }
            else {
                auto it = variables.find(operand);

                if (it == variables.end()) {
                    return runtimeError(
                        instruction.line,
                        "Undefined variable '" + operand + "'."
                    );
                }

                std::cout << it->second.data << '\n';
            }

            ++pc;
        }

        else if (instruction.opcode == "mov") {
            size_t comma = instruction.operand.find(',');

            std::string destination =
                instruction.operand.substr(0, comma);

            std::string source =
                instruction.operand.substr(comma + 1);

            auto target = variables.find(destination);

            if (target == variables.end()) {
                return runtimeError(
                    instruction.line,
                    "Undefined variable '" + destination + "'."
                );
            }

            RuntimeValue newValue = target->second;

            auto sourceVariable = variables.find(source);

            if (sourceVariable != variables.end()) {
                if (sourceVariable->second.type != target->second.type) {
                    return runtimeError(
                        instruction.line,
                        "Variable type mismatch."
                    );
                }

                newValue = sourceVariable->second;
            }
            else {
                if (!validLiteral(target->second.type, source)) {
                    return runtimeError(
                        instruction.line,
                        "Invalid value for variable '" + destination + "'."
                    );
                }

                newValue.data =
                    target->second.type == flix::ValueType::String
                    ? decodeString(source)
                    : source;
            }

            target->second = newValue;

            ++pc;
        }

        else if (instruction.opcode == "jmp") {
            pc = program.labels.at(instruction.operand);
        }

        else if (instruction.opcode == "halt") {
            if (instruction.operand.empty())
                return 0;

            try {
                return std::stoi(instruction.operand);
            }
            catch (...) {
                return 1;
            }
        }

        else {
            return runtimeError(
                instruction.line,
                "Unsupported instruction '" +
                instruction.opcode + "'."
            );
        }
    }

    std::cerr << "Flix Error: Program ended without halt.\n";
    return 1;
}
