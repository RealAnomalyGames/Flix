
#include "flix/program.hpp"
#include "flix/version.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

namespace {

std::string quote(const std::string& value) {
    std::string result = "\"";

    for (char c : value) {
        switch (c) {
        case '\\': result += "\\\\"; break;
        case '"':  result += "\\\""; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:   result += c; break;
        }
    }

    result += "\"";
    return result;
}

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string decodeString(const std::string& value) {
    std::string result;

    if (value.size() < 2) {
        return result;
    }

    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        char c = value[i];

        if (c == '\\' && i + 2 < value.size()) {
            ++i;

            switch (value[i]) {
            case 'n': result += '\n'; break;
            case 't': result += '\t'; break;
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            default: result += value[i]; break;
            }
        } else {
            result += c;
        }
    }

    return result;
}

std::string cppValue(
    const std::string& value,
    flix::ValueType expected
) {
    if (!value.empty() && value.front() == '"') {
        return "V::str(" + quote(decodeString(value)) + ")";
    }

    if (value == "true" || value == "false") {
        return "V::boolean(" + value + ")";
    }

    if (expected == flix::ValueType::String) {
        return "V::str(" + quote(value) + ")";
    }

    return "V::number(" + value + "L)";
}

std::string cppExpression(
    const std::string& expression,
    const flix::Program& program
) {
    std::string result;

    bool inString = false;
    bool escaped = false;

    for (std::size_t i = 0; i < expression.size();) {
        const char c = expression[i];

        if (inString) {
            result += c;

            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }

            ++i;
            continue;
        }

        if (c == '"') {
            inString = true;
            result += c;
            ++i;
            continue;
        }

        // Flix's ^^ is exclusive OR.
        // Since comparisons produce bool values, C++ !=
        // provides the required exclusive-or behavior.
        if (c == '^' &&
            i + 1 < expression.size() &&
            expression[i + 1] == '^') {
            result += "!=";
            i += 2;
            continue;
        }

        if (std::isalpha(
                static_cast<unsigned char>(c)) ||
            c == '_') {
            std::size_t end = i + 1;

            while (end < expression.size()) {
                const char next = expression[end];

                if (!std::isalnum(
                        static_cast<unsigned char>(next)) &&
                    next != '_') {
                    break;
                }

                ++end;
            }

            const std::string token =
                expression.substr(i, end - i);

            if (token == "true" || token == "false") {
                result += token;
            } else if (program.variables.contains(token)) {
                result += "vars[" + quote(token) + "]";
            } else {
                result += token;
            }

            i = end;
            continue;
        }

        result += c;
        ++i;
    }

    return result;
}

std::string conditionExpression(
    const flix::Instruction& instruction,
    const flix::Program& program
) {
    std::string expression = instruction.operand;

    if (instruction.opcode == "el") {
        if (expression.rfind("cmp ", 0) != 0) {
            return "true";
        }

        expression = trim(expression.substr(4));
    }

    return cppExpression(expression, program);
}

std::string emitRuntime() {
    return R"CPP(
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct V {
    long double n = 0;
    std::string s;
    bool b = false;
    int type = 0;

    V() = default;

    V(long double value) : n(value), type(0) {}
    V(int value) : n(value), type(0) {}
    V(bool value) : b(value), type(2) {}
    V(const char* value) : s(value), type(1) {}
    V(const std::string& value) : s(value), type(1) {}

    explicit operator bool() const {
        if (type == 1) return !s.empty();
        if (type == 2) return b;
        return n != 0;
    }

    static V number(long double value) {
        return V(value);
    }

    static V str(const std::string& value) {
        return V(value);
    }

    static V boolean(bool value) {
        return V(value);
    }

    std::string text() const {
        if (type == 1) return s;
        if (type == 2) return b ? "true" : "false";

        std::ostringstream stream;
        stream << std::setprecision(18) << n;
        return stream.str();
    }
};

static V operator+(const V& a, const V& b) {
    if (a.type == 1 || b.type == 1) {
        return V::str(a.text() + b.text());
    }

    return V::number(a.n + b.n);
}

static V operator-(const V& a, const V& b) {
    return V::number(a.n - b.n);
}

static V operator*(const V& a, const V& b) {
    return V::number(a.n * b.n);
}

static V operator/(const V& a, const V& b) {
    if (b.n == 0) {
        std::cerr << "Flix runtime error: division by zero\n";
        std::exit(1);
    }

    return V::number(a.n / b.n);
}

static V operator%(const V& a, const V& b) {
    if (b.n == 0) {
        std::cerr << "Flix runtime error: modulo by zero\n";
        std::exit(1);
    }

    return V::number(std::fmod(a.n, b.n));
}

static V operator-(const V& value) {
    return V::number(-value.n);
}

static bool operator==(const V& a, const V& b) {
    if (a.type == 1 || b.type == 1) {
        return a.text() == b.text();
    }

    if (a.type == 2 && b.type == 2) {
        return a.b == b.b;
    }

    return a.n == b.n;
}

static bool operator!=(const V& a, const V& b) {
    return !(a == b);
}

static bool operator<(const V& a, const V& b) {
    if (a.type == 1 && b.type == 1) {
        return a.s < b.s;
    }

    return a.n < b.n;
}

static bool operator>(const V& a, const V& b) {
    return b < a;
}

static bool operator<=(const V& a, const V& b) {
    return !(a > b);
}

static bool operator>=(const V& a, const V& b) {
    return !(a < b);
}
)CPP";
}

bool writeGeneratedSource(
    const fs::path& output,
    const flix::Program& program
) {
    std::ofstream file(output);

    if (!file) {
        std::cerr << "Unable to write generated C++ source.\n";
        return false;
    }

    file << emitRuntime();

    int branchCount = 0;

    for (const auto& instruction : program.instructions) {
        if (instruction.branch_id >= 0) {
            branchCount = std::max(
                branchCount,
                instruction.branch_id + 1
            );
        }
    }

    file << "\nint main() {\n";
    file << "    std::unordered_map<std::string, V> vars;\n";
    file << "    std::vector<bool> branchTaken("
         << branchCount << ", false);\n";

    for (const auto& [name, variable] : program.variables) {
        file << "    vars[" << quote(name) << "] = "
             << cppValue(
                    variable.initial_value,
                    variable.type
                )
             << ";\n";
    }

    file << "    int pc = "
         << program.entry_point
         << ";\n";

    file << "    while (true) {\n";
    file << "        switch (pc) {\n";

    for (std::size_t i = 0;
         i < program.instructions.size();
         ++i) {
        const auto& instruction = program.instructions[i];

        const std::string& op = instruction.opcode;
        const std::string& arg = instruction.operand;

        file << "        case " << i << ": {\n";

        if (op == "out" || op == "outln") {
            if (!arg.empty() && arg.front() == '"') {
                file << "            std::cout << "
                     << cppValue(
                            arg,
                            flix::ValueType::String
                        )
                     << ".text();\n";
            } else if (program.variables.contains(arg)) {
                file << "            std::cout << vars["
                     << quote(arg)
                     << "].text();\n";
            } else {
                file << "            std::cout << "
                     << cppValue(
                            arg,
                            flix::ValueType::Integer
                        )
                     << ".text();\n";
            }

            if (op == "outln") {
                file << "            std::cout << '\\n';\n";
            }

            file << "            ++pc;\n";
        }

        else if (op == "mov") {
            const auto comma = arg.find(',');

            if (comma == std::string::npos) {
                std::cerr << "Line " << instruction.line
                          << ": mov requires destination, source.\n";
                return false;
            }

            const std::string destination =
                trim(arg.substr(0, comma));

            const std::string source =
                trim(arg.substr(comma + 1));

            if (!program.variables.contains(destination)) {
                std::cerr << "Line " << instruction.line
                          << ": Unknown destination variable '"
                          << destination << "'.\n";
                return false;
            }

            if (program.variables.contains(source)) {
                file << "            vars[" << quote(destination)
                     << "] = vars[" << quote(source) << "];\n";
            } else {
                file << "            vars[" << quote(destination)
                     << "] = "
                     << cppValue(
                            source,
                            program.variables.at(destination).type
                        )
                     << ";\n";
            }

            file << "            ++pc;\n";
        }

        else if (
            op == "add" || op == "sub" ||
            op == "mul" || op == "div" ||
            op == "mod"
        ) {
            const auto equals = arg.find('=');

            if (equals == std::string::npos) {
                std::cerr << "Line " << instruction.line
                          << ": Expected name = value.\n";
                return false;
            }

            const std::string destination =
                trim(arg.substr(0, equals));

            const std::string source =
                trim(arg.substr(equals + 1));

            if (!program.variables.contains(destination)) {
                std::cerr << "Line " << instruction.line
                          << ": Unknown arithmetic variable '"
                          << destination << "'.\n";
                return false;
            }

            const std::string rhs =
                program.variables.contains(source)
                    ? "vars[" + quote(source) + "]"
                    : cppValue(
                        source,
                        program.variables.at(destination).type
                    );

            const char* symbol =
                op == "add" ? "+" :
                op == "sub" ? "-" :
                op == "mul" ? "*" :
                op == "div" ? "/" : "%";

            file << "            vars[" << quote(destination)
                 << "] = vars[" << quote(destination) << "] "
                 << symbol << " (" << rhs << ");\n";

            file << "            ++pc;\n";
        }

        else if (op == "inc" || op == "dec" || op == "neg") {
            if (!program.variables.contains(arg)) {
                std::cerr << "Line " << instruction.line
                          << ": Unknown variable '" << arg << "'.\n";
                return false;
            }

            if (op == "inc") {
                file << "            vars[" << quote(arg)
                     << "] = vars[" << quote(arg)
                     << "] + V::number(1);\n";
            } else if (op == "dec") {
                file << "            vars[" << quote(arg)
                     << "] = vars[" << quote(arg)
                     << "] - V::number(1);\n";
            } else {
                file << "            vars[" << quote(arg)
                     << "] = -vars[" << quote(arg) << "];\n";
            }

            file << "            ++pc;\n";
        }

        else if (op == "jmp") {
            const std::string label = arg.substr(1);

            file << "            pc = "
                 << program.labels.at(label)
                 << ";\n";
        }

        else if (op == "halt") {
            file << "            return "
                 << (arg.empty() ? "0" : arg)
                 << ";\n";
        }

        else if (op == "cmp" || op == ) {
            if (instruction.branch_id < 0) {
                std::cerr << "Line " << instruction.line
                          << ": Invalid conditional branch metadata.\n";
                return false;
            }

            const int id = instruction.branch_id;

            if (instruction.branch_first) {
                file << "            branchTaken[" << id
                     << "] = false;\n";
            }

            file << "            if (branchTaken[" << id << "]) {\n";
            file << "                pc = "
                 << instruction.branch_end << ";\n";
            file << "            } else if (("
                 << conditionExpression(instruction, program)
                 << ")) {\n";

            file << "                branchTaken[" << id
                 << "] = true;\n";
            file << "                ++pc;\n";

            file << "            } else {\n";
            file << "                pc = "
                 << instruction.branch_next << ";\n";
            file << "            }\n";
        }

        else {
            std::cerr << "Line " << instruction.line
                      << ": Unsupported instruction '"
                      << op << "'.\n";
            return false;
        }

        file << "            break;\n";
        file << "        }\n";
    }

    file << "        default:\n";
    file << "            return 0;\n";
    file << "        }\n";
    file << "    }\n";
    file << "}\n";

    return true;
}

std::string shellQuote(const std::string& value) {
    return "\"" + value + "\"";
}

bool runCommand(const std::string& command) {
    std::cout << "> " << command << "\n";
    return std::system(command.c_str()) == 0;
}

void usage() {
    std::cout
        << "Flix Compiler " << flix::version() << "\n\n"
        << "Usage:\n"
        << "  flixc version\n"
        << "  flixc check <file.flix>\n"
        << "  flixc asm <file.flix> -o <file.asm>\n"
        << "  flixc build <file.flix> -o <file.exe>\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 &&
        std::string(argv[1]) == "version") {
        std::cout << "Flix Compiler "
                  << flix::version() << "\n";
        return 0;
    }

    if (argc < 3) {
        usage();
        return 1;
    }

    const std::string command = argv[1];
    const fs::path sourcePath = argv[2];

    if (command != "check" &&
        command != "asm" &&
        command != "build") {
        usage();
        return 1;
    }

    flix::Program program;
    std::string error;

    if (!flix::loadProgram(
        sourcePath.string(),
        program,
        error
    )) {
        std::cerr << "Flix Error: " << error << "\n";
        return 1;
    }

    if (command == "check") {
        std::cout << "Flix check successful.\n";
        std::cout << "Instructions: "
                  << program.instructions.size() << "\n";
        std::cout << "Variables: "
                  << program.variables.size() << "\n";
        std::cout << "Labels: "
                  << program.labels.size() << "\n";
        return 0;
    }

    if (argc != 5 || std::string(argv[3]) != "-o") {
        usage();
        return 1;
    }

    const fs::path outputPath = argv[4];

    if (outputPath.extension() !=
        (command == "asm" ? ".asm" : ".exe")) {
        std::cerr << "Incorrect output extension.\n";
        return 1;
    }

    if (!outputPath.parent_path().empty()) {
        fs::create_directories(outputPath.parent_path());
    }

    const fs::path generatedCpp =
        outputPath.parent_path() /
        (outputPath.stem().string() + ".generated.cpp");

    if (!writeGeneratedSource(generatedCpp, program)) {
        return 1;
    }

    std::string compileCommand =
        "cl.exe /nologo /std:c++20 /EHsc /O2 ";

    if (command == "asm") {
        compileCommand += "/c /FA /Fa";
        compileCommand += shellQuote(outputPath.string());
        compileCommand += " ";
        compileCommand += shellQuote(generatedCpp.string());
    } else {
        compileCommand += "/Fe";
        compileCommand += shellQuote(outputPath.string());
        compileCommand += " ";
        compileCommand += shellQuote(generatedCpp.string());
    }

    if (!runCommand(compileCommand)) {
        std::cerr << "Native compilation failed.\n";
        return 1;
    }

    if (command == "asm") {
        std::cout << "Assembly listing generated: "
                  << outputPath.string() << "\n";
    } else {
        std::cout << "Native executable generated: "
                  << outputPath.string() << "\n";
    }

    return 0;
}
