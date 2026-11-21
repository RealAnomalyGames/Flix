
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace flix {

struct Instruction {
    std::string opcode;
    std::string operand;
    int line;
};

struct Program {
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, size_t> labels;
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

}
