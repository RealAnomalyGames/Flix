#pragma once

#include <string>

namespace flix {

enum class TokenType {
    Identifier,
    Label,
    Number,
    String,

    Colon,
    Comma,
    Equal,

    NewLine,
    EndOfFile,
    Invalid
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

}
