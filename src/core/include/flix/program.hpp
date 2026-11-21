#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

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
    int line = 0;
};

struct Instruction {
    std::string opcode;
    std::string operand;
    int line = 0;

    // Leading indentation, with tabs normalized to four spaces.
    int indentation = 0;
};

struct Program {
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, std::size_t> labels;
    std::unordered_map<std::string, Variable> variables;
    std::size_t entry_point = 0;
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
