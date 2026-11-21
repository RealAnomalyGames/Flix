
#include "parser.hpp"

#include <sstream>

namespace flix {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens) {}

const Token& Parser::current() const {
    return tokens[position];
}

void Parser::advance() {
    if (current().type != TokenType::EndOfFile) {
        ++position;
    }
}

void Parser::skipNewLines() {
    while (current().type == TokenType::NewLine) {
        advance();
    }
}

void Parser::setError(
    const std::string& description,
    const Token& token
) {
    if (error)
        return;

    error = true;

    std::ostringstream stream;

    stream << "Line " << token.line
           << ", Column " << token.column
           << ": " << description;

    message = stream.str();
}

Statement Parser::parseStatement() {
    Statement statement;

    const Token& first = current();

    statement.line = first.line;
    statement.column = first.column;

    if (first.type == TokenType::Invalid) {
        setError(first.lexeme, first);
        return statement;
    }

    if (first.type == TokenType::Label) {
        statement.isLabel = true;
        statement.name = first.lexeme;

        advance();

        if (current().type != TokenType::NewLine
            && current().type != TokenType::EndOfFile) {

            setError("Unexpected content after label", current());
        }

        return statement;
    }

    if (first.type != TokenType::Identifier) {
        setError("Expected an instruction or label", first);
        return statement;
    }

    statement.name = first.lexeme;

    advance();

    while (current().type != TokenType::NewLine
        && current().type != TokenType::EndOfFile) {

        if (current().type == TokenType::Invalid) {
            setError(current().lexeme, current());
            break;
        }

        statement.operands.push_back(current());
        advance();
    }

    return statement;
}

std::vector<Statement> Parser::parse() {
    std::vector<Statement> statements;

    skipNewLines();

    while (current().type != TokenType::EndOfFile && !error) {
        statements.push_back(parseStatement());

        skipNewLines();
    }

    return statements;
}

bool Parser::hasError() const {
    return error;
}

const std::string& Parser::errorMessage() const {
    return message;
}

}
