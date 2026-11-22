#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "flix/program.hpp"
#include "flix/version.hpp"

namespace {

struct Value {
    flix::ValueType type;
    std::string data;
};

using Variables = std::unordered_map<std::string, Value>;

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = s.find_last_not_of(" \t\r\n");

    return s.substr(first, last - first + 1);
}

std::string decodeString(const std::string& s) {
    std::string result;

    for (std::size_t i = 1; i + 1 < s.size(); ++i) {
        char c = s[i];

        if (c == '\\' && i + 2 < s.size()) {
            char next = s[++i];

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

std::string defaultValue(flix::ValueType type) {
    switch (type) {
        case flix::ValueType::Integer: return "0";
        case flix::ValueType::Float: return "0.0";
        case flix::ValueType::String: return "";
        case flix::ValueType::Boolean: return "false";
    }

    return "";
}

int signOf(const std::string& s) {
    return !s.empty() && s[0] == '-' ? -1 : 1;
}

std::string absDigits(std::string s) {
    if (!s.empty() && (s[0] == '-' || s[0] == '+'))
        s.erase(s.begin());

    while (s.size() > 1 && s[0] == '0')
        s.erase(s.begin());

    return s.empty() ? "0" : s;
}

int compareAbs(const std::string& a, const std::string& b) {
    const std::string x = absDigits(a);
    const std::string y = absDigits(b);

    if (x.size() != y.size())
        return x.size() < y.size() ? -1 : 1;

    if (x == y)
        return 0;

    return x < y ? -1 : 1;
}

std::string addAbs(const std::string& a, const std::string& b) {
    int carry = 0;
    int i = static_cast<int>(a.size()) - 1;
    int j = static_cast<int>(b.size()) - 1;
    std::string result;

    while (i >= 0 || j >= 0 || carry) {
        int sum = carry;

        if (i >= 0) sum += a[i--] - '0';
        if (j >= 0) sum += b[j--] - '0';

        result.push_back(static_cast<char>('0' + sum % 10));
        carry = sum / 10;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

std::string subAbs(const std::string& a, const std::string& b) {
    int borrow = 0;
    int i = static_cast<int>(a.size()) - 1;
    int j = static_cast<int>(b.size()) - 1;
    std::string result;

    while (i >= 0) {
        int digit = (a[i--] - '0') - borrow;

        if (j >= 0)
            digit -= b[j--] - '0';

        borrow = digit < 0 ? 1 : 0;

        if (borrow)
            digit += 10;

        result.push_back(static_cast<char>('0' + digit));
    }

    while (result.size() > 1 && result.back() == '0')
        result.pop_back();

    std::reverse(result.begin(), result.end());
    return result;
}

std::string bigAdd(const std::string& a, const std::string& b) {
    int sa = signOf(a);
    int sb = signOf(b);

    std::string x = absDigits(a);
    std::string y = absDigits(b);

    if (sa == sb) {
        std::string result = addAbs(x, y);
        return sa < 0 && result != "0" ? "-" + result : result;
    }

    int cmp = compareAbs(x, y);

    if (cmp == 0)
        return "0";

    std::string result;

    if (cmp > 0) {
        result = subAbs(x, y);
        return sa < 0 ? "-" + result : result;
    }

    result = subAbs(y, x);
    return sb < 0 ? "-" + result : result;
}

std::string bigNeg(const std::string& s) {
    if (absDigits(s) == "0")
        return "0";

    return signOf(s) < 0 ? absDigits(s) : "-" + absDigits(s);
}

std::string bigSub(const std::string& a, const std::string& b) {
    return bigAdd(a, bigNeg(b));
}

std::string bigMul(const std::string& a, const std::string& b) {
    const std::string x = absDigits(a);
    const std::string y = absDigits(b);

    std::vector<int> digits(x.size() + y.size(), 0);

    for (int i = static_cast<int>(x.size()) - 1; i >= 0; --i) {
        for (int j = static_cast<int>(y.size()) - 1; j >= 0; --j) {
            int pos = i + j + 1;

            int product =
                (x[i] - '0') * (y[j] - '0') + digits[pos];

            digits[pos] = product % 10;
            digits[pos - 1] += product / 10;
        }
    }

    std::string result;

    for (int d : digits)
        result.push_back(static_cast<char>('0' + d));

    result = absDigits(result);

    if (signOf(a) != signOf(b) && result != "0")
        result = "-" + result;

    return result;
}

std::pair<std::string, std::string> bigDivMod(
    const std::string& dividend,
    const std::string& divisor
) {
    std::string a = absDigits(dividend);
    std::string b = absDigits(divisor);

    if (b == "0")
        throw std::runtime_error("Division by zero.");

    std::string quotient;
    std::string remainder = "0";

    for (char digit : a) {
        remainder = absDigits(remainder + digit);

        int count = 0;

        while (compareAbs(remainder, b) >= 0) {
            remainder = subAbs(remainder, b);
            ++count;
        }

        quotient.push_back(static_cast<char>('0' + count));
    }

    quotient = absDigits(quotient);

    if (signOf(dividend) != signOf(divisor) && quotient != "0")
        quotient = "-" + quotient;

    if (signOf(dividend) < 0 && remainder != "0")
        remainder = "-" + remainder;

    return {quotient, remainder};
}

long double toFloat(const std::string& s) {
    std::size_t used = 0;
    long double value = std::stold(s, &used);

    if (used != s.size() || !std::isfinite(value))
        throw std::runtime_error("Invalid floating-point value.");

    return value;
}

std::string fromFloat(long double value) {
    if (!std::isfinite(value))
        throw std::runtime_error("Floating-point result is not finite.");

    std::string result = std::to_string(value);

    while (result.size() > 1 && result.back() == '0')
        result.pop_back();

    if (!result.empty() && result.back() == '.')
        result.pop_back();

    return result;
}

bool isNumber(const std::string& s) {
    if (s.empty())
        return false;

    bool dot = false;
    bool digit = false;

    for (std::size_t i = 0; i < s.size(); ++i) {
        if (i == 0 && (s[i] == '-' || s[i] == '+'))
            continue;

        if (s[i] == '.' && !dot) {
            dot = true;
            continue;
        }

        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;

        digit = true;
    }

    return digit;
}

Value resolve(const std::string& token, const Variables& vars) {
    const std::string s = trim(token);

    auto found = vars.find(s);

    if (found != vars.end())
        return found->second;

    if (s == "true" || s == "false")
        return {flix::ValueType::Boolean, s};

    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return {flix::ValueType::String, decodeString(s)};

    if (isNumber(s)) {
        if (s.find('.') != std::string::npos)
            return {flix::ValueType::Float, s};

        return {flix::ValueType::Integer, s};
    }

    throw std::runtime_error("Unknown value or variable '" + s + "'.");
}

std::string resolveOutput(
    const std::string& operand,
    const Variables& vars
) {
    const std::string value = trim(operand);

    // Variable output.
    const auto found = vars.find(value);

    if (found != vars.end()) {
        return found->second.data;
    }

    // Quoted string output.
    if (value.size() >= 2 &&
        value.front() == '"' &&
        value.back() == '"') {
        return decodeString(value);
    }

    // Boolean and numeric output.
    if (value == "true" || value == "false" ||
        isNumber(value)) {
        return resolve(value, vars).data;
    }

    // Allow unquoted text in output instructions.
    return value;
}

std::size_t findOperator(
    const std::string& s,
    const std::vector<std::string>& operators
) {
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

        if (quoted)
            continue;

        for (const auto& op : operators) {
            if (s.compare(i, op.size(), op) == 0)
                return i;
        }
    }

    return std::string::npos;
}

bool evaluateCondition(
    const std::string& expression,
    const Variables& vars
) {
    const std::string s = trim(expression);

    auto split = [&](const std::vector<std::string>& ops)
        -> std::pair<std::string, std::string> {
        const auto pos = findOperator(s, ops);

        if (pos == std::string::npos)
            return {"", ""};

        std::string op;

        for (const auto& candidate : ops) {
            if (s.compare(pos, candidate.size(), candidate) == 0) {
                op = candidate;
                break;
            }
        }

        return {
            op,
            s.substr(0, pos) + "\n" + s.substr(pos + op.size())
        };
    };

    // OR
    {
        auto result = split({"||"});

        if (!result.first.empty()) {
            const auto pos = findOperator(s, {"||"});

            return evaluateCondition(s.substr(0, pos), vars) ||
                   evaluateCondition(s.substr(pos + 2), vars);
        }
    }

    // XOR
    {
        const auto pos = findOperator(s, {"^^"});

        if (pos != std::string::npos) {
            return evaluateCondition(s.substr(0, pos), vars) !=
                   evaluateCondition(s.substr(pos + 2), vars);
        }
    }

    // AND
    {
        const auto pos = findOperator(s, {"&&"});

        if (pos != std::string::npos) {
            return evaluateCondition(s.substr(0, pos), vars) &&
                   evaluateCondition(s.substr(pos + 2), vars);
        }
    }

    // NOT
    if (!s.empty() && s[0] == '!')
        return !evaluateCondition(s.substr(1), vars);

    // Comparison
    const auto pos = findOperator(
        s,
        {"==", "!=", ">=", "<=", ">", "<"}
    );

    if (pos == std::string::npos)
        throw std::runtime_error("Invalid comparison '" + s + "'.");

    std::string op;

    for (const auto& candidate :
         {"==", "!=", ">=", "<=", ">", "<"}) {
        if (s.compare(pos, std::string(candidate).size(), candidate) == 0) {
            op = candidate;
            break;
        }
    }

    const std::string leftText = trim(s.substr(0, pos));
    const std::string rightText = trim(s.substr(pos + op.size()));

    Value a = resolve(leftText, vars);
    Value b = resolve(rightText, vars);

    if (a.type != b.type)
        throw std::runtime_error("Comparison type mismatch.");

    int comparison = 0;

    if (a.type == flix::ValueType::Integer) {
        if (signOf(a.data) != signOf(b.data))
            comparison = signOf(a.data) < signOf(b.data) ? -1 : 1;
        else {
            comparison = compareAbs(a.data, b.data);

            if (signOf(a.data) < 0)
                comparison = -comparison;
        }
    } else if (a.type == flix::ValueType::Float) {
        const long double x = toFloat(a.data);
        const long double y = toFloat(b.data);

        comparison = x < y ? -1 : (x > y ? 1 : 0);
    } else if (a.type == flix::ValueType::String) {
        comparison = a.data < b.data ? -1 :
                     (a.data > b.data ? 1 : 0);
    } else {
        comparison = a.data == b.data ? 0 :
                     (a.data == "false" ? -1 : 1);
    }

    if (op == "==") return comparison == 0;
    if (op == "!=") return comparison != 0;
    if (op == ">") return comparison > 0;
    if (op == "<") return comparison < 0;
    if (op == ">=") return comparison >= 0;
    if (op == "<=") return comparison <= 0;

    return false;
}

void arithmetic(
    const flix::Instruction& instruction,
    Variables& vars
) {
    std::string destination;
    std::string operand;

    if (instruction.opcode == "inc" ||
        instruction.opcode == "dec" ||
        instruction.opcode == "neg") {
        destination = trim(instruction.operand);
    } else {
        const auto equals = instruction.operand.find('=');

        destination = trim(instruction.operand.substr(0, equals));
        operand = trim(instruction.operand.substr(equals + 1));
    }

    auto target = vars.find(destination);

    if (target == vars.end())
        throw std::runtime_error("Undefined variable '" + destination + "'.");

    Value& a = target->second;

    if (a.type != flix::ValueType::Integer &&
        a.type != flix::ValueType::Float) {
        throw std::runtime_error("Arithmetic requires a numeric variable.");
    }

    Value b{a.type, "1"};

    if (!operand.empty()) {
        b = resolve(operand, vars);

        if (b.type != a.type)
            throw std::runtime_error("Arithmetic type mismatch.");
    }

    const std::string& op = instruction.opcode;

    if (a.type == flix::ValueType::Integer) {
        if (op == "add") a.data = bigAdd(a.data, b.data);
        else if (op == "sub") a.data = bigSub(a.data, b.data);
        else if (op == "mul") a.data = bigMul(a.data, b.data);
        else if (op == "div") a.data = bigDivMod(a.data, b.data).first;
        else if (op == "mod") a.data = bigDivMod(a.data, b.data).second;
        else if (op == "inc") a.data = bigAdd(a.data, "1");
        else if (op == "dec") a.data = bigSub(a.data, "1");
        else if (op == "neg") a.data = bigNeg(a.data);
    } else {
        long double x = toFloat(a.data);
        long double y = toFloat(b.data);

        if (op == "add") x += y;
        else if (op == "sub") x -= y;
        else if (op == "mul") x *= y;
        else if (op == "div") {
            if (y == 0)
                throw std::runtime_error("Division by zero.");

            x /= y;
        } else if (op == "mod") {
            throw std::runtime_error("mod requires integer variables.");
        } else if (op == "inc") x += 1;
        else if (op == "dec") x -= 1;
        else if (op == "neg") x = -x;

        a.data = fromFloat(x);
    }
}

bool active(const std::vector<bool>& stack) {
    for (bool item : stack) {
        if (!item)
            return false;
    }

    return true;
}

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Flix Interpreter\n";
        std::cout << "Usage: flix run <file.flix>\n";
        std::cout << "Usage: flix version\n";
        std::cout << "       flix \n";
        return 0;
    }

    const std::string command = argv[1];

    if (command == "") {
        std::cout << "Flix " << flix::version() << "\n";
        return 0;
    }

    if (argc == 2 && std::string(argv[1]) == "version") {
        std::cout << "Flix " << flix::version() << "\n";
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

    Variables vars;

    for (const auto& [name, declaration] : program.variables) {
        std::string value = declaration.initial_value;

        if (declaration.type == flix::ValueType::String)
            value = decodeString(value);

        vars[name] = {declaration.type, value};
    }

    std::vector<bool> conditionalActive;
    std::vector<bool> branchTaken;
    std::vector<int> conditionalIndent;

    std::size_t pc = program.entry_point;

    try {
        while (pc < program.instructions.size()) {
            const auto& ins = program.instructions[pc];

            const bool isElse = ins.opcode == "el";

            // Close blocks when leaving their indentation level.
            while (!conditionalIndent.empty() &&
                   ins.indentation < conditionalIndent.back()) {
                conditionalIndent.pop_back();
                conditionalActive.pop_back();
                branchTaken.pop_back();
            }

            if (ins.opcode == "cmp") {
                while (!conditionalIndent.empty() &&
                       ins.indentation <= conditionalIndent.back()) {
                    conditionalIndent.pop_back();
                    conditionalActive.pop_back();
                    branchTaken.pop_back();
                }

                bool parent = active(conditionalActive);
                bool result = parent &&
                    evaluateCondition(ins.operand, vars);

                conditionalIndent.push_back(ins.indentation);
                conditionalActive.push_back(result);
                branchTaken.push_back(result);

                ++pc;
                continue;
            }

            if (isElse) {
                if (conditionalIndent.empty() ||
                    conditionalIndent.back() != ins.indentation) {
                    throw std::runtime_error(
                        "Line " + std::to_string(ins.line) +
                        ": el has no matching cmp."
                    );
                }

                const bool parent = conditionalActive.size() == 1
                    ? true
                    : [&]() {
                        for (std::size_t i = 0;
                             i + 1 < conditionalActive.size(); ++i) {
                            if (!conditionalActive[i])
                                return false;
                        }
                        return true;
                    }();

                const bool alreadyTaken = branchTaken.back();

                if (ins.operand.rfind("cmp ", 0) == 0) {
                    const bool condition = evaluateCondition(
                        ins.operand.substr(4),
                        vars
                    );

                    conditionalActive.back() =
                        parent && !alreadyTaken && condition;

                    branchTaken.back() =
                        alreadyTaken || conditionalActive.back();
                } else {
                    conditionalActive.back() =
                        parent && !alreadyTaken;

                    branchTaken.back() = true;
                }

                ++pc;
                continue;
            }

            if (!active(conditionalActive)) {
                ++pc;
                continue;
            }

            const std::string& op = ins.opcode;

            if (op == "out" || op == "outln") {
                std::cout << resolveOutput(ins.operand, vars);

                if (op == "outln") {
                    std::cout << '\n';
                }

                std::cout.flush();

            } else if (op == "mov") {
                const auto comma = ins.operand.find(',');

                const std::string destination =
                    trim(ins.operand.substr(0, comma));

                const std::string source =
                    trim(ins.operand.substr(comma + 1));

                auto target = vars.find(destination);

                if (target == vars.end())
                    throw std::runtime_error(
                        "Undefined variable '" + destination + "'."
                    );

                Value value = resolve(source, vars);

                if (target->second.type != value.type)
                    throw std::runtime_error("mov type mismatch.");

                target->second.data = value.data;

            } else if (op == "jmp") {
                const std::string label = ins.operand.substr(1);
                pc = program.labels.at(label);
                continue;

            } else if (op == "halt") {
                if (ins.operand.empty())
                    return 0;

                return std::stoi(ins.operand);

            } else if (op == "add" || op == "sub" ||
                       op == "mul" || op == "div" ||
                       op == "mod" || op == "inc" ||
                       op == "dec" || op == "neg") {
                arithmetic(ins, vars);

            } else {
                throw std::runtime_error(
                    "Unsupported instruction '" + op + "'."
                );
            }

            ++pc;
        }
    } catch (const std::exception& e) {
        std::cerr << "Flix Runtime Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
