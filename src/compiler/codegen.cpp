#include "flix/codegen.hpp"
#include "flix/version.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <unordered_map>

namespace flix {
namespace {

std::string cString(const std::string& value) {
    std::ostringstream out;

    for (unsigned char c : value) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"':  out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            case '\0': out << "\\0"; break;
            default:
                if (c < 32 || c >= 127) {
                    static const char* hex = "0123456789ABCDEF";
                    out << "\\x"
                        << hex[(c >> 4) & 0x0F]
                        << hex[c & 0x0F]
                        << "\"\"";
                } else {
                    out << static_cast<char>(c);
                }
        }
    }

    return out.str();
}

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(
        value.begin(), value.end(),
        [](unsigned char c) { return std::isspace(c) != 0; }
    );

    const auto last = std::find_if_not(
        value.rbegin(), value.rend(),
        [](unsigned char c) { return std::isspace(c) != 0; }
    ).base();

    if (first >= last) {
        return {};
    }

    return std::string(first, last);
}

bool splitAssignment(
    const std::string& operand,
    std::string& destination,
    std::string& source
) {
    bool quoted = false;
    bool escaped = false;

    for (std::size_t i = 0; i < operand.size(); ++i) {
        const char c = operand[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (c == '\\' && quoted) {
            escaped = true;
            continue;
        }

        if (c == '"') {
            quoted = !quoted;
            continue;
        }

        if (c == '=' && !quoted) {
            destination = trim(operand.substr(0, i));
            source = trim(operand.substr(i + 1));
            return !destination.empty() && !source.empty();
        }
    }

    return false;
}

bool splitComma(
    const std::string& operand,
    std::string& first,
    std::string& second
) {
    bool quoted = false;
    bool escaped = false;

    for (std::size_t i = 0; i < operand.size(); ++i) {
        const char c = operand[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (c == '\\' && quoted) {
            escaped = true;
            continue;
        }

        if (c == '"') {
            quoted = !quoted;
            continue;
        }

        if (c == ',' && !quoted) {
            first = trim(operand.substr(0, i));
            second = trim(operand.substr(i + 1));
            return !first.empty() && !second.empty();
        }
    }

    return false;
}

std::string labelName(const std::string& name) {
    std::string result = "flix_label_";

    for (unsigned char c : name) {
        result += std::isalnum(c) || c == '_'
            ? static_cast<char>(c)
            : '_';
    }

    return result;
}

std::string pcLabel(std::size_t position) {
    return "flix_pc_" + std::to_string(position);
}

std::string branchTakenName(int branchId) {
    return "flix_branch_taken_" + std::to_string(branchId);
}

bool getType(
    const Program& program,
    const std::string& name,
    ValueType& type
) {
    const auto it = program.variables.find(name);

    if (it == program.variables.end()) {
        return false;
    }

    type = it->second.type;
    return true;
}

bool emitInstruction(
    const Program& program,
    std::size_t index,
    const Instruction& instruction,
    std::ostringstream& out,
    std::string& error
) {
    const std::string& op = instruction.opcode;
    const std::string& arg = instruction.operand;

    if (op == "out" || op == "outln") {
        out << "    flix_output("
            << "\"" << cString(arg) << "\", "
            << (op == "outln" ? "1" : "0")
            << ");\n";
        return true;
    }

    if (op == "mov") {
        std::string destination;
        std::string source;

        if (!splitComma(arg, destination, source)) {
            error = "Invalid mov instruction at source line "
                + std::to_string(instruction.line);
            return false;
        }

        out << "    flix_mov("
            << "\"" << cString(destination) << "\", "
            << "\"" << cString(source) << "\");\n";
        return true;
    }

    if (op == "add" || op == "sub" ||
        op == "mul" || op == "div" || op == "mod") {
        std::string destination;
        std::string source;

        if (!splitAssignment(arg, destination, source)) {
            error = "Invalid arithmetic instruction at source line "
                + std::to_string(instruction.line);
            return false;
        }

        out << "    flix_arithmetic("
            << "\"" << op << "\", "
            << "\"" << cString(destination) << "\", "
            << "\"" << cString(source) << "\");\n";
        return true;
    }

    if (op == "inc" || op == "dec" || op == "neg") {
        out << "    flix_unary("
            << "\"" << op << "\", "
            << "\"" << cString(arg) << "\");\n";
        return true;
    }

    if (op == "cmp") {
        if (instruction.branch_id < 0) {
            error = "Missing branch metadata at source line "
                + std::to_string(instruction.line);
            return false;
        }

        const std::string taken =
            branchTakenName(instruction.branch_id);

        if (instruction.branch_first) {
            out << "    " << taken << " = 0;\n";
        }

        out << "    if (flix_eval_condition(\""
            << cString(arg)
            << "\")) {\n"
            << "        " << taken << " = 1;\n"
            << "    } else {\n"
            << "        goto " << pcLabel(instruction.branch_next)
            << ";\n"
            << "    }\n";

        return true;
    }

    if (op == "el") {
        if (instruction.branch_id < 0) {
            error = "Missing else-branch metadata at source line "
                + std::to_string(instruction.line);
            return false;
        }

        const std::string taken =
            branchTakenName(instruction.branch_id);

        if (arg.rfind("cmp ", 0) == 0) {
            const std::string expression = trim(arg.substr(4));

            out << "    if (" << taken << ") {\n"
                << "        goto " << pcLabel(instruction.branch_end)
                << ";\n"
                << "    }\n"
                << "    if (flix_eval_condition(\""
                << cString(expression)
                << "\")) {\n"
                << "        " << taken << " = 1;\n"
                << "    } else {\n"
                << "        goto " << pcLabel(instruction.branch_next)
                << ";\n"
                << "    }\n";
        } else {
            out << "    if (" << taken << ") {\n"
                << "        goto " << pcLabel(instruction.branch_end)
                << ";\n"
                << "    }\n"
                << "    " << taken << " = 1;\n";
        }

        return true;
    }

    if (op == "jmp") {
        if (arg.size() < 2 || arg.front() != '.') {
            error = "Invalid jump at source line "
                + std::to_string(instruction.line);
            return false;
        }

        const std::string name = arg.substr(1);
        const auto target = program.labels.find(name);

        if (target == program.labels.end()) {
            error = "Unknown jump target at source line "
                + std::to_string(instruction.line);
            return false;
        }

        out << "    goto " << pcLabel(target->second) << ";\n";
        return true;
    }

    if (op == "halt") {
        out << "    return ";

        if (arg.empty()) {
            out << "0";
        } else {
            out << "flix_exit_code(\"" << cString(arg) << "\")";
        }

        out << ";\n";
        return true;
    }

    error = "Unsupported instruction '" + op
        + "' at source line " + std::to_string(instruction.line);

    (void)index;
    return false;
}

} // namespace

bool generateC(
    const Program& program,
    std::string& output,
    std::string& error
) {
    output.clear();
    error.clear();

    std::ostringstream out;

    out
        << "/* Generated by Flix " << version() << ". */\n"
        << "/* Native C backend translation unit. */\n\n"
        << "#include <stddef.h>\n"
        << "#include \"runtime.h\"\n\n"
        << "int main(void) {\n"
        << "    flix_runtime_init();\n";

    for (const auto& [name, variable] : program.variables) {
        const char* type = "FLIX_INT";

        switch (variable.type) {
            case ValueType::Integer: type = "FLIX_INT"; break;
            case ValueType::Float: type = "FLIX_FLOAT"; break;
            case ValueType::String: type = "FLIX_STRING"; break;
            case ValueType::Boolean: type = "FLIX_BOOL"; break;
        }

        out << "    flix_define(\""
            << cString(name) << "\", "
            << type << ", \""
            << cString(variable.initial_value)
            << "\");\n";
    }

    std::unordered_map<std::size_t, std::string> labelsAtPosition;

    for (const auto& [name, position] : program.labels) {
        labelsAtPosition[position] = name;
    }

    for (const auto& instruction : program.instructions) {
        if (instruction.branch_first && instruction.branch_id >= 0) {
            out << "    int "
                << branchTakenName(instruction.branch_id)
                << " = 0;\n";
        }
    }

    out << "\n    goto " << pcLabel(program.entry_point) << ";\n\n";

    for (std::size_t i = 0; i <= program.instructions.size(); ++i) {
        out << pcLabel(i) << ":\n";

        const auto label = labelsAtPosition.find(i);

        if (label != labelsAtPosition.end()) {
            out << "    " << labelName(label->second) << ": ;\n";
        }

        if (i == program.instructions.size()) {
            out << "    return 0;\n";
            continue;
        }

        if (!emitInstruction(
                program,
                i,
                program.instructions[i],
                out,
                error)) {
            return false;
        }

        out << "\n";
    }

    out
        << "}\n";

    output = out.str();
    return true;
}

} // namespace flix
