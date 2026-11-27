
#include "flix/program.hpp"
#include "flix/version.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

// -----------------------------------------------------------------------------
// General helpers
// -----------------------------------------------------------------------------

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return {};
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
            case 'n':  result += '\n'; break;
            case 't':  result += '\t'; break;
            case '"':  result += '"'; break;
            case '\\': result += '\\'; break;
            default:   result += value[i]; break;
            }
        } else {
            result += c;
        }
    }

    return result;
}

bool isIntegerText(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    std::size_t i = 0;

    if (value[i] == '+' || value[i] == '-') {
        ++i;
    }

    if (i == value.size()) {
        return false;
    }

    for (; i < value.size(); ++i) {
        if (value[i] < '0' || value[i] > '9') {
            return false;
        }
    }

    return true;
}

bool isFloatText(const std::string& value) {
    const auto dot = value.find('.');

    if (dot == std::string::npos ||
        dot == 0 ||
        dot + 1 == value.size() ||
        value.find('.', dot + 1) != std::string::npos) {
        return false;
    }

    return isIntegerText(value.substr(0, dot) + value.substr(dot + 1));
}

std::string normalizeInteger(const std::string& input) {
    if (!isIntegerText(input)) {
        throw std::runtime_error("Invalid integer: " + input);
    }

    std::size_t position = 0;
    bool negative = false;

    if (input[position] == '+' || input[position] == '-') {
        negative = input[position] == '-';
        ++position;
    }

    while (position < input.size() && input[position] == '0') {
        ++position;
    }

    if (position == input.size()) {
        return "0";
    }

    std::string result = input.substr(position);

    if (negative) {
        result.insert(result.begin(), '-');
    }

    return result;
}

std::string absInteger(const std::string& value) {
    return !value.empty() && value.front() == '-'
        ? value.substr(1)
        : value;
}

int compareAbsolute(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return a.size() < b.size() ? -1 : 1;
    }

    if (a == b) {
        return 0;
    }

    return a < b ? -1 : 1;
}

std::string addAbsolute(
    const std::string& a,
    const std::string& b
) {
    std::string result;
    int carry = 0;

    std::size_t i = a.size();
    std::size_t j = b.size();

    while (i > 0 || j > 0 || carry) {
        int sum = carry;

        if (i > 0) {
            sum += a[--i] - '0';
        }

        if (j > 0) {
            sum += b[--j] - '0';
        }

        result.push_back(static_cast<char>('0' + sum % 10));
        carry = sum / 10;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

// Requires a >= b.
std::string subtractAbsolute(
    const std::string& a,
    const std::string& b
) {
    std::string result;
    int borrow = 0;

    std::size_t i = a.size();
    std::size_t j = b.size();

    while (i > 0) {
        int digit = (a[--i] - '0') - borrow;

        if (j > 0) {
            digit -= b[--j] - '0';
        }

        borrow = 0;

        if (digit < 0) {
            digit += 10;
            borrow = 1;
        }

        result.push_back(static_cast<char>('0' + digit));
    }

    std::reverse(result.begin(), result.end());

    const auto first = result.find_first_not_of('0');

    if (first == std::string::npos) {
        return "0";
    }

    return result.substr(first);
}

int compareInteger(const std::string& left, const std::string& right) {
    const std::string a = normalizeInteger(left);
    const std::string b = normalizeInteger(right);

    const bool negativeA = a.front() == '-';
    const bool negativeB = b.front() == '-';

    if (negativeA != negativeB) {
        return negativeA ? -1 : 1;
    }

    const int absolute = compareAbsolute(
        absInteger(a),
        absInteger(b)
    );

    return negativeA ? -absolute : absolute;
}

std::string addInteger(
    const std::string& left,
    const std::string& right
) {
    const std::string a = normalizeInteger(left);
    const std::string b = normalizeInteger(right);

    const bool negativeA = a.front() == '-';
    const bool negativeB = b.front() == '-';

    const std::string absA = absInteger(a);
    const std::string absB = absInteger(b);

    if (negativeA == negativeB) {
        std::string result = addAbsolute(absA, absB);

        if (negativeA && result != "0") {
            result.insert(result.begin(), '-');
        }

        return result;
    }

    const int comparison = compareAbsolute(absA, absB);

    if (comparison == 0) {
        return "0";
    }

    const bool resultNegative =
        comparison > 0 ? negativeA : negativeB;

    std::string result = comparison > 0
        ? subtractAbsolute(absA, absB)
        : subtractAbsolute(absB, absA);

    if (resultNegative && result != "0") {
        result.insert(result.begin(), '-');
    }

    return result;
}

std::string negateInteger(const std::string& value) {
    const std::string normalized = normalizeInteger(value);

    if (normalized == "0") {
        return "0";
    }

    if (normalized.front() == '-') {
        return normalized.substr(1);
    }

    return "-" + normalized;
}

std::string subtractInteger(
    const std::string& a,
    const std::string& b
) {
    return addInteger(a, negateInteger(b));
}

std::string multiplyAbsolute(
    const std::string& a,
    const std::string& b
) {
    if (a == "0" || b == "0") {
        return "0";
    }

    std::vector<int> digits(a.size() + b.size(), 0);

    for (std::size_t i = a.size(); i > 0; --i) {
        for (std::size_t j = b.size(); j > 0; --j) {
            const int product =
                (a[i - 1] - '0') * (b[j - 1] - '0');

            const std::size_t position = i + j - 1;

            digits[position] += product;
            digits[position - 1] += digits[position] / 10;
            digits[position] %= 10;
        }
    }

    std::string result;

    for (int digit : digits) {
        if (result.empty() && digit == 0) {
            continue;
        }

        result.push_back(static_cast<char>('0' + digit));
    }

    return result.empty() ? "0" : result;
}

std::string multiplyInteger(
    const std::string& left,
    const std::string& right
) {
    const std::string a = normalizeInteger(left);
    const std::string b = normalizeInteger(right);

    const bool negative =
        (a.front() == '-') != (b.front() == '-');

    std::string result = multiplyAbsolute(
        absInteger(a),
        absInteger(b)
    );

    if (negative && result != "0") {
        result.insert(result.begin(), '-');
    }

    return result;
}

std::pair<std::string, std::string> divideAbsolute(
    const std::string& numerator,
    const std::string& denominator
) {
    if (denominator == "0") {
        throw std::runtime_error("Division by zero.");
    }

    std::string quotient;
    std::string remainder = "0";

    for (char digit : numerator) {
        if (remainder == "0") {
            remainder = std::string(1, digit);
        } else {
            remainder.push_back(digit);
        }

        int count = 0;

        while (compareAbsolute(remainder, denominator) >= 0) {
            remainder = subtractAbsolute(remainder, denominator);
            ++count;
        }

        quotient.push_back(static_cast<char>('0' + count));
    }

    const auto first = quotient.find_first_not_of('0');

    if (first == std::string::npos) {
        quotient = "0";
    } else {
        quotient = quotient.substr(first);
    }

    return { quotient, remainder };
}

std::pair<std::string, std::string> divideInteger(
    const std::string& left,
    const std::string& right
) {
    const std::string a = normalizeInteger(left);
    const std::string b = normalizeInteger(right);

    if (b == "0") {
        throw std::runtime_error("Division by zero.");
    }

    auto result = divideAbsolute(absInteger(a), absInteger(b));

    const bool negativeQuotient =
        (a.front() == '-') != (b.front() == '-');

    if (negativeQuotient && result.first != "0") {
        result.first.insert(result.first.begin(), '-');
    }

    // Remainder follows the sign of the dividend.
    if (a.front() == '-' && result.second != "0") {
        result.second.insert(result.second.begin(), '-');
    }

    return result;
}

// -----------------------------------------------------------------------------
// Runtime values
// -----------------------------------------------------------------------------

struct Value {
    flix::ValueType type = flix::ValueType::Integer;
    std::string integer = "0";
    long double floating = 0.0L;
    std::string string;
    bool boolean = false;

    static Value makeInteger(const std::string& value) {
        Value result;
        result.type = flix::ValueType::Integer;
        result.integer = normalizeInteger(value);
        return result;
    }

    static Value makeFloat(long double value) {
        Value result;
        result.type = flix::ValueType::Float;
        result.floating = value;
        return result;
    }

    static Value makeString(const std::string& value) {
        Value result;
        result.type = flix::ValueType::String;
        result.string = value;
        return result;
    }

    static Value makeBoolean(bool value) {
        Value result;
        result.type = flix::ValueType::Boolean;
        result.boolean = value;
        return result;
    }

    std::string text() const {
        switch (type) {
        case flix::ValueType::Integer:
            return integer;

        case flix::ValueType::Float: {
            std::ostringstream stream;
            stream << std::setprecision(18) << floating;
            return stream.str();
        }

        case flix::ValueType::String:
            return string;

        case flix::ValueType::Boolean:
            return boolean ? "true" : "false";
        }

        return {};
    }
};

Value parseLiteral(
    const std::string& source,
    flix::ValueType expected
) {
    const std::string value = trim(source);

    if (expected == flix::ValueType::String) {
        if (value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"') {
            return Value::makeString(decodeString(value));
        }

        return Value::makeString(value);
    }

    if (expected == flix::ValueType::Boolean) {
        if (value == "true") {
            return Value::makeBoolean(true);
        }

        if (value == "false") {
            return Value::makeBoolean(false);
        }

        throw std::runtime_error("Expected a boolean value.");
    }

    if (expected == flix::ValueType::Integer) {
        if (!isIntegerText(value)) {
            throw std::runtime_error("Expected an integer value.");
        }

        return Value::makeInteger(value);
    }

    if (!isFloatText(value)) {
        throw std::runtime_error("Expected a floating-point value.");
    }

    try {
        return Value::makeFloat(std::stold(value));
    } catch (...) {
        throw std::runtime_error("Invalid floating-point value.");
    }
}

int compareValues(const Value& a, const Value& b) {
    if (a.type != b.type) {
        throw std::runtime_error(
            "Cannot compare values of different types."
        );
    }

    switch (a.type) {
    case flix::ValueType::Integer:
        return compareInteger(a.integer, b.integer);

    case flix::ValueType::Float:
        if (a.floating < b.floating) return -1;
        if (a.floating > b.floating) return 1;
        return 0;

    case flix::ValueType::String:
        if (a.string < b.string) return -1;
        if (a.string > b.string) return 1;
        return 0;

    case flix::ValueType::Boolean:
        if (a.boolean == b.boolean) return 0;
        return a.boolean ? 1 : -1;
    }

    return 0;
}

bool requireBoolean(const Value& value) {
    if (value.type != flix::ValueType::Boolean) {
        throw std::runtime_error(
            "Logical operators require boolean values."
        );
    }

    return value.boolean;
}

// -----------------------------------------------------------------------------
// Conditional expression parser
// -----------------------------------------------------------------------------

class ExpressionParser {
public:
    ExpressionParser(
        const std::string& expression,
        const std::unordered_map<std::string, Value>& variables
    )
        : variables_(variables) {
        tokenize(expression);
    }

    bool evaluate() {
        Value result = parseOr();

        if (position_ != tokens_.size()) {
            throw std::runtime_error(
                "Unexpected token '" + tokens_[position_] +
                "' in condition."
            );
        }

        return requireBoolean(result);
    }

private:
    const std::unordered_map<std::string, Value>& variables_;
    std::vector<std::string> tokens_;
    std::size_t position_ = 0;

    void tokenize(const std::string& expression) {
        for (std::size_t i = 0; i < expression.size();) {
            const char c = expression[i];

            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++i;
                continue;
            }

            if (c == '"') {
                const std::size_t start = i++;
                bool escaped = false;

                while (i < expression.size()) {
                    const char current = expression[i++];

                    if (escaped) {
                        escaped = false;
                    } else if (current == '\\') {
                        escaped = true;
                    } else if (current == '"') {
                        break;
                    }
                }

                if (i > expression.size() ||
                    expression[i - 1] != '"') {
                    throw std::runtime_error(
                        "Unterminated string in condition."
                    );
                }

                tokens_.push_back(
                    expression.substr(start, i - start)
                );

                continue;
            }

            if (i + 1 < expression.size()) {
                const std::string pair = expression.substr(i, 2);

                if (pair == "&&" || pair == "||" ||
                    pair == "^^" || pair == "==" ||
                    pair == "!=" || pair == "<=" ||
                    pair == ">=") {
                    tokens_.push_back(pair);
                    i += 2;
                    continue;
                }
            }

            if (c == '<' || c == '>' || c == '!' ||
                c == '(' || c == ')' || c == '-') {
                tokens_.push_back(std::string(1, c));
                ++i;
                continue;
            }

            const std::size_t start = i;

            while (i < expression.size()) {
                const char current = expression[i];

                if (current == ' ' || current == '\t' ||
                    current == '\r' || current == '\n' ||
                    current == '<' || current == '>' ||
                    current == '!' || current == '(' ||
                    current == ')' || current == '-' ||
                    current == '&' || current == '|' ||
                    current == '^' || current == '=') {
                    break;
                }

                ++i;
            }

            if (i == start) {
                throw std::runtime_error(
                    "Invalid character in condition."
                );
            }

            tokens_.push_back(
                expression.substr(start, i - start)
            );
        }
    }

    bool match(const std::string& token) {
        if (position_ < tokens_.size() &&
            tokens_[position_] == token) {
            ++position_;
            return true;
        }

        return false;
    }

    Value parseOr() {
        Value left = parseXor();

        while (match("||")) {
            Value right = parseXor();

            left = Value::makeBoolean(
                requireBoolean(left) || requireBoolean(right)
            );
        }

        return left;
    }

    Value parseXor() {
        Value left = parseAnd();

        while (match("^^")) {
            Value right = parseAnd();

            left = Value::makeBoolean(
                requireBoolean(left) != requireBoolean(right)
            );
        }

        return left;
    }

    Value parseAnd() {
        Value left = parseComparison();

        while (match("&&")) {
            Value right = parseComparison();

            left = Value::makeBoolean(
                requireBoolean(left) && requireBoolean(right)
            );
        }

        return left;
    }

    Value parseComparison() {
        Value left = parseUnary();

        if (position_ >= tokens_.size()) {
            return left;
        }

        const std::string op = tokens_[position_];

        if (op != "==" && op != "!=" &&
            op != "<" && op != ">" &&
            op != "<=" && op != ">=") {
            return left;
        }

        ++position_;

        Value right = parseUnary();
        const int comparison = compareValues(left, right);

        if (op == "==") return Value::makeBoolean(comparison == 0);
        if (op == "!=") return Value::makeBoolean(comparison != 0);
        if (op == "<")  return Value::makeBoolean(comparison < 0);
        if (op == ">")  return Value::makeBoolean(comparison > 0);
        if (op == "<=") return Value::makeBoolean(comparison <= 0);
        if (op == ">=") return Value::makeBoolean(comparison >= 0);

        return Value::makeBoolean(false);
    }

    Value parseUnary() {
        if (match("!")) {
            return Value::makeBoolean(!requireBoolean(parseUnary()));
        }

        if (match("-")) {
            Value value = parseUnary();

            if (value.type == flix::ValueType::Integer) {
                return Value::makeInteger(
                    negateInteger(value.integer)
                );
            }

            if (value.type == flix::ValueType::Float) {
                return Value::makeFloat(-value.floating);
            }

            throw std::runtime_error(
                "Unary minus requires a numeric value."
            );
        }

        return parsePrimary();
    }

    Value parsePrimary() {
        if (match("(")) {
            Value value = parseOr();

            if (!match(")")) {
                throw std::runtime_error(
                    "Missing closing parenthesis in condition."
                );
            }

            return value;
        }

        if (position_ >= tokens_.size()) {
            throw std::runtime_error(
                "Unexpected end of condition."
            );
        }

        const std::string token = tokens_[position_++];

        if (token.size() >= 2 &&
            token.front() == '"' &&
            token.back() == '"') {
            return Value::makeString(decodeString(token));
        }

        if (token == "true") {
            return Value::makeBoolean(true);
        }

        if (token == "false") {
            return Value::makeBoolean(false);
        }

        const auto variable = variables_.find(token);

        if (variable != variables_.end()) {
            return variable->second;
        }

        if (isIntegerText(token)) {
            return Value::makeInteger(token);
        }

        if (isFloatText(token)) {
            return Value::makeFloat(std::stold(token));
        }

        throw std::runtime_error(
            "Unknown value '" + token + "' in condition."
        );
    }
};

// -----------------------------------------------------------------------------
// Flix execution runtime
// -----------------------------------------------------------------------------

class FlixRuntime {
public:
    explicit FlixRuntime(const flix::Program& program)
        : program_(program) {
        for (const auto& [name, variable] : program_.variables) {
            variables_[name] = parseLiteral(
                variable.initial_value,
                variable.type
            );
        }

        for (const auto& instruction : program_.instructions) {
            if (instruction.branch_id >= 0) {
                branchCount_ = std::max(
                    branchCount_,
                    instruction.branch_id + 1
                );
            }
        }

        branchTaken_.resize(
            static_cast<std::size_t>(branchCount_),
            false
        );
    }

    int run() {
        std::size_t pc = program_.entry_point;

        try {
            while (pc < program_.instructions.size()) {
                const auto& instruction = program_.instructions[pc];

                const std::string& op = instruction.opcode;
                const std::string& arg = instruction.operand;

                if (op == "out" || op == "outln") {
                    output(arg);

                    if (op == "outln") {
                        std::cout << '\n';
                    }

                    ++pc;
                }

                else if (op == "mov") {
                    const auto comma = arg.find(',');

                    if (comma == std::string::npos) {
                        fail(instruction, "mov requires destination, source.");
                    }

                    const std::string destination =
                        trim(arg.substr(0, comma));

                    const std::string source =
                        trim(arg.substr(comma + 1));

                    Value& target = variable(destination);
                    Value value = resolve(source, target.type);

                    if (target.type != value.type) {
                        fail(instruction, "Cannot assign a value of a different type.");
                    }

                    target = value;
                    ++pc;
                }

                else if (
                    op == "add" || op == "sub" ||
                    op == "mul" || op == "div" ||
                    op == "mod"
                ) {
                    const auto equals = arg.find('=');

                    if (equals == std::string::npos) {
                        fail(instruction, "Expected name = value.");
                    }

                    const std::string destination =
                        trim(arg.substr(0, equals));

                    const std::string source =
                        trim(arg.substr(equals + 1));

                    Value& target = variable(destination);
                    const Value right = resolve(source, target.type);

                    if (target.type != right.type) {
                        fail(instruction, "Arithmetic operands must have matching types.");
                    }

                    if (target.type == flix::ValueType::Integer) {
                        if (op == "add") {
                            target.integer = addInteger(
                                target.integer, right.integer
                            );
                        } else if (op == "sub") {
                            target.integer = subtractInteger(
                                target.integer, right.integer
                            );
                        } else if (op == "mul") {
                            target.integer = multiplyInteger(
                                target.integer, right.integer
                            );
                        } else {
                            const auto division = divideInteger(
                                target.integer, right.integer
                            );

                            if (op == "div") {
                                target.integer = division.first;
                            } else {
                                target.integer = division.second;
                            }
                        }
                    }

                    else if (target.type == flix::ValueType::Float) {
                        if (op == "mod") {
                            fail(instruction, "Modulo is not supported for float values.");
                        }

                        if (op == "add") {
                            target.floating += right.floating;
                        } else if (op == "sub") {
                            target.floating -= right.floating;
                        } else if (op == "mul") {
                            target.floating *= right.floating;
                        } else {
                            if (right.floating == 0.0L) {
                                fail(instruction, "Division by zero.");
                            }

                            target.floating /= right.floating;
                        }
                    }

                    else if (
                        op == "add" &&
                        target.type == flix::ValueType::String
                    ) {
                        target.string += right.string;
                    }

                    else {
                        fail(instruction, "This arithmetic operation is not valid for this type.");
                    }

                    ++pc;
                }

                else if (op == "inc" || op == "dec" || op == "neg") {
                    Value& target = variable(arg);

                    if (target.type == flix::ValueType::Integer) {
                        if (op == "inc") {
                            target.integer = addInteger(target.integer, "1");
                        } else if (op == "dec") {
                            target.integer = subtractInteger(target.integer, "1");
                        } else {
                            target.integer = negateInteger(target.integer);
                        }
                    }

                    else if (target.type == flix::ValueType::Float) {
                        if (op == "inc") {
                            target.floating += 1.0L;
                        } else if (op == "dec") {
                            target.floating -= 1.0L;
                        } else {
                            target.floating = -target.floating;
                        }
                    }

                    else {
                        fail(instruction, "inc, dec, and neg require numeric variables.");
                    }

                    ++pc;
                }

                else if (op == "jmp") {
                    const std::string label = arg.substr(1);
                    const auto found = program_.labels.find(label);

                    if (found == program_.labels.end()) {
                        fail(instruction, "Unknown jump label.");
                    }

                    pc = found->second;
                }

                else if (op == "halt") {
                    if (arg.empty()) {
                        return 0;
                    }

                    const std::string code = normalizeInteger(arg);

                    // Windows process exit codes are 32-bit.
                    // Flix integers themselves remain arbitrary precision.
                    unsigned int result = 0;

                    for (char digit : absInteger(code)) {
                        result = result * 10u +
                            static_cast<unsigned int>(digit - '0');
                    }

                    if (!code.empty() && code.front() == '-') {
                        result = 0u - result;
                    }

                    return static_cast<int>(result);
                }

                else if (op == "cmp" || op == "el") {
                    if (instruction.branch_id < 0) {
                        fail(instruction, "Invalid conditional branch metadata.");
                    }

                    const int id = instruction.branch_id;
                    const auto index = static_cast<std::size_t>(id);

                    if (instruction.branch_first) {
                        branchTaken_[index] = false;
                    }

                    if (branchTaken_[index]) {
                        pc = instruction.branch_end;
                        continue;
                    }

                    bool condition = false;

                    if (op == "el" &&
                        arg.rfind("cmp ", 0) != 0) {
                        condition = true;
                    } else {
                        std::string expression = arg;

                        if (op == "el") {
                            expression = trim(expression.substr(4));
                        }

                        condition = ExpressionParser(
                            expression,
                            variables_
                        ).evaluate();
                    }

                    if (condition) {
                        branchTaken_[index] = true;
                        ++pc;
                    } else {
                        pc = instruction.branch_next;
                    }
                }

                else {
                    fail(instruction, "Unsupported instruction '" + op + "'.");
                }
            }
        } catch (const std::exception& exception) {
            std::cerr << "Flix runtime error: "
                      << exception.what() << '\n';
            return 1;
        }

        return 0;
    }

private:
    const flix::Program& program_;
    std::unordered_map<std::string, Value> variables_;
    std::vector<bool> branchTaken_;
    int branchCount_ = 0;

    [[noreturn]] void fail(
        const flix::Instruction& instruction,
        const std::string& message
    ) const {
        throw std::runtime_error(
            "Line " + std::to_string(instruction.line) +
            ": " + message
        );
    }

    Value& variable(const std::string& name) {
        const auto found = variables_.find(name);

        if (found == variables_.end()) {
            throw std::runtime_error(
                "Unknown variable '" + name + "'."
            );
        }

        return found->second;
    }

    Value resolve(
        const std::string& source,
        flix::ValueType expected
    ) {
        const std::string value = trim(source);

        const auto found = variables_.find(value);

        if (found != variables_.end()) {
            return found->second;
        }

        return parseLiteral(value, expected);
    }

    void output(const std::string& operand) {
        const std::string value = trim(operand);

        if (value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"') {
            std::cout << decodeString(value);
            return;
        }

        const auto found = variables_.find(value);

        if (found != variables_.end()) {
            std::cout << found->second.text();
            return;
        }

        if (value == "true" || value == "false") {
            std::cout << value;
            return;
        }

        if (isIntegerText(value)) {
            std::cout << normalizeInteger(value);
            return;
        }

        if (isFloatText(value)) {
            std::cout << std::setprecision(18)
                      << std::stold(value);
            return;
        }

        // Flix supports unquoted text output.
        std::cout << operand;
    }
};

// -----------------------------------------------------------------------------
// Standalone executable payload
//
// File layout:
// [native flixc.exe]
// [embedded Flix source]
// [uint64 source length]
// [8-byte FLIXEXE1 signature]
// -----------------------------------------------------------------------------

constexpr char payloadMagic[8] = {
    'F', 'L', 'I', 'X', 'E', 'X', 'E', '1'
};

fs::path currentExecutablePath(const char* argv0) {
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);

    const DWORD length = GetModuleFileNameW(
        nullptr,
        buffer.data(),
        static_cast<DWORD>(buffer.size())
    );

    if (length > 0 && length < buffer.size()) {
        return fs::path(
            std::wstring(buffer.data(), length)
        );
    }
#endif

    return fs::absolute(fs::path(argv0));
}

bool readEmbeddedSource(
    const fs::path& executable,
    std::string& source
) {
    std::ifstream file(executable, std::ios::binary);

    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();

    if (size < 16) {
        return false;
    }

    file.seekg(size - 16);

    std::uint64_t sourceLength = 0;
    char magic[8]{};

    file.read(
        reinterpret_cast<char*>(&sourceLength),
        sizeof(sourceLength)
    );

    file.read(magic, sizeof(magic));

    if (!file || !std::equal(
        std::begin(magic),
        std::end(magic),
        std::begin(payloadMagic)
    )) {
        return false;
    }

    if (sourceLength >
        static_cast<std::uint64_t>(size - 16)) {
        throw std::runtime_error(
            "Invalid embedded Flix executable payload."
        );
    }

    file.seekg(
        size - 16 - static_cast<std::streamoff>(sourceLength)
    );

    source.resize(static_cast<std::size_t>(sourceLength));

    if (sourceLength != 0) {
        file.read(source.data(), static_cast<std::streamsize>(sourceLength));
    }

    if (!file) {
        throw std::runtime_error(
            "Unable to read embedded Flix program."
        );
    }

    return true;
}

bool buildExecutable(
    const fs::path& executable,
    const fs::path& output,
    const std::string& source
) {
    std::error_code error;

    const fs::path sourceExecutable =
        fs::weakly_canonical(executable, error);

    error.clear();

    const fs::path targetExecutable =
        fs::weakly_canonical(output, error);

    if (!error && sourceExecutable == targetExecutable) {
        std::cerr << "Output cannot overwrite flixc.exe.\n";
        return false;
    }

    if (!fs::exists(executable)) {
        std::cerr << "Unable to locate the running compiler executable.\n";
        return false;
    }

    if (!output.parent_path().empty()) {
        fs::create_directories(output.parent_path());
    }

    fs::copy_file(
        executable,
        output,
        fs::copy_options::overwrite_existing
    );

    std::ofstream file(
        output,
        std::ios::binary | std::ios::app
    );

    if (!file) {
        std::cerr << "Unable to append program to output executable.\n";
        return false;
    }

    const std::uint64_t sourceLength =
        static_cast<std::uint64_t>(source.size());

    file.write(
        source.data(),
        static_cast<std::streamsize>(source.size())
    );

    file.write(
        reinterpret_cast<const char*>(&sourceLength),
        sizeof(sourceLength)
    );

    file.write(payloadMagic, sizeof(payloadMagic));
    file.flush();

    if (!file) {
        std::cerr << "Failed to write executable payload.\n";
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Flix VM assembly listing
// -----------------------------------------------------------------------------

std::string assemblyQuote(const std::string& value) {
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

bool writeAssembly(
    const fs::path& output,
    const flix::Program& program
) {
    std::ofstream file(output);

    if (!file) {
        std::cerr << "Unable to write assembly output.\n";
        return false;
    }

    file << "; Flix VM Assembly\n";
    file << "; Flix version " << flix::version() << "\n";
    file << "; Target: Flix standalone VM\n\n";

    file << ".program\n";
    file << "    .entry " << program.entry_point << "\n\n";

    file << ".variables\n";

    for (const auto& [name, variable] : program.variables) {
        file << "    .var " << name
             << ", " << flix::valueTypeName(variable.type)
             << ", " << assemblyQuote(variable.initial_value)
             << "\n";
    }

    file << "\n.labels\n";

    for (const auto& [name, position] : program.labels) {
        file << "    ." << name << " = " << position << "\n";
    }

    file << "\n.code\n";

    for (std::size_t i = 0;
         i < program.instructions.size();
         ++i) {
        const auto& instruction = program.instructions[i];

        file << "    " << std::setw(6) << std::setfill('0')
             << i << std::setfill(' ') << ": "
             << instruction.opcode;

        if (!instruction.operand.empty()) {
            file << " " << instruction.operand;
        }

        if (instruction.branch_id >= 0) {
            file << "    ; branch=" << instruction.branch_id
                 << " next=" << instruction.branch_next
                 << " end=" << instruction.branch_end;
        }

        file << "    ; source line " << instruction.line << "\n";
    }

    file << "\n.end\n";

    return static_cast<bool>(file);
}

// -----------------------------------------------------------------------------
// Command line
// -----------------------------------------------------------------------------

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
    const fs::path executable =
        currentExecutablePath(argv[0]);

    // Packaged executables run their embedded Flix program
    // instead of entering the compiler command-line interface.
    try {
        std::string embeddedSource;

        if (readEmbeddedSource(executable, embeddedSource)) {
            flix::Program program;
            std::string error;

            if (!flix::parseProgram(
                embeddedSource,
                program,
                error
            )) {
                std::cerr << "Flix executable error: "
                          << error << '\n';
                return 1;
            }

            return FlixRuntime(program).run();
        }
    } catch (const std::exception& exception) {
        std::cerr << "Flix executable error: "
                  << exception.what() << '\n';
        return 1;
    }

    if (argc == 2 && std::string(argv[1]) == "version") {
        std::cout << "Flix Compiler "
                  << flix::version() << '\n';
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
        std::cerr << "Flix Error: " << error << '\n';
        return 1;
    }

    if (command == "check") {
        std::cout << "Flix check successful.\n";
        std::cout << "Instructions: "
                  << program.instructions.size() << '\n';
        std::cout << "Variables: "
                  << program.variables.size() << '\n';
        std::cout << "Labels: "
                  << program.labels.size() << '\n';
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

    try {
        if (command == "asm") {
            if (!writeAssembly(outputPath, program)) {
                return 1;
            }

            std::cout << "Flix VM assembly generated: "
                      << outputPath.string() << '\n';
            return 0;
        }

        std::ifstream sourceFile(
            sourcePath,
            std::ios::binary
        );

        if (!sourceFile) {
            std::cerr << "Unable to read source file.\n";
            return 1;
        }

        std::ostringstream contents;
        contents << sourceFile.rdbuf();

        if (!sourceFile.good() && !sourceFile.eof()) {
            std::cerr << "Unable to read complete source file.\n";
            return 1;
        }

        if (!buildExecutable(
            executable,
            outputPath,
            contents.str()
        )) {
            return 1;
        }

        std::cout << "Standalone Flix executable generated: "
                  << outputPath.string() << '\n';

        std::cout << "Runtime: embedded Flix VM\n";
        std::cout << "Integer support: arbitrary precision\n";

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Compiler error: "
                  << exception.what() << '\n';
        return 1;
    }
}
