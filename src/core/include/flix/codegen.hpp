#pragma once

#include "flix/program.hpp"

#include <string>

namespace flix {

// Generates a standalone C translation unit from a validated Flix program.
// The generated C uses the Flix runtime API.
bool generateC(
    const Program& program,
    std::string& output,
    std::string& error
);

}
