
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>

namespace flix {

enum class ValueType {
    Integer,
    Float,
    String,
    Boolean
};

struct Variable {
    std::string name;
    ValueType type;
    std::string initial_value;
    int line;
};

struct Instruction {
    std::string opcode;
    std::string operand;
    int line;
};

struct Program {
    std::vector<Instruction> instructions;

    std::unordered_map<std::string, size_t> labels;

    std::unordered_map<std::string, Variable> variables;

    size_t entry_point = 0;
};

bool parseProgram(
    const std::string& source,
    Program& program,
    std::string& error
);

bool loadProgram(
    const std::string& path,
    Program& program,
    std::string& error
);

std::string valueTypeName(ValueType type);

}
