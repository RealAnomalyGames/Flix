
#pragma once

#include "../lexer/token.hpp"

#include <string>
#include <vector>

namespace flix {

struct Statement {
    bool isLabel = false;

    std::string name;

    std::vector<Token> operands;

    int line = 1;
    int column = 1;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    std::vector<Statement> parse();

    bool hasError() const;

    const std::string& errorMessage() const;

private:
    const std::vector<Token>& tokens;

    std::size_t position = 0;

    bool error = false;
    std::string message;

    const Token& current() const;

    void advance();
    void skipNewLines();

    Statement parseStatement();

    void setError(
        const std::string& description,
        const Token& token
    );
};

}
