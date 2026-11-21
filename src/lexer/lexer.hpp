
#pragma once

#include "token.hpp"

#include <string>
#include <vector>

namespace flix {

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();

private:
    const std::string& source;

    std::vector<Token> tokens;

    std::size_t position = 0;

    int line = 1;
    int column = 1;

    char current() const;
    char peek(std::size_t offset = 1) const;

    void advance();
    void addToken(TokenType type, const std::string& value);

    void scanToken();
    void scanIdentifier();
    void scanNumber();
    void scanString();
    void scanLabel();

    void skipComment();
};

}
