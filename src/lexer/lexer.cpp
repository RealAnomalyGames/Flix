
#include "lexer.hpp"

#include <cctype>
#include <utility>

namespace flix {

Lexer::Lexer(const std::string& source)
    : source(source) {}

char Lexer::current() const {
    if (position >= source.size())
        return '\0';

    return source[position];
}

char Lexer::peek(std::size_t offset) const {
    if (position + offset >= source.size())
        return '\0';

    return source[position + offset];
}

void Lexer::advance() {
    if (current() == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }

    ++position;
}

void Lexer::addToken(TokenType type, const std::string& value) {
    tokens.push_back({
        type,
        value,
        line,
        column
    });
}

void Lexer::skipComment() {
    while (current() != '\n' && current() != '\0') {
        advance();
    }
}

void Lexer::scanIdentifier() {
    std::string value;

    int startColumn = column;

    while (std::isalnum(static_cast<unsigned char>(current()))
        || current() == '_') {

        value += current();
        advance();
    }

    tokens.push_back({
        TokenType::Identifier,
        value,
        line,
        startColumn
    });
}

void Lexer::scanNumber() {
    std::string value;

    int startColumn = column;
    bool decimal = false;

    while (std::isdigit(static_cast<unsigned char>(current()))
        || (current() == '.' && !decimal
            && std::isdigit(static_cast<unsigned char>(peek())))) {

        if (current() == '.') {
            decimal = true;
        }

        value += current();
        advance();
    }

    tokens.push_back({
        TokenType::Number,
        value,
        line,
        startColumn
    });
}

void Lexer::scanString() {
    int startLine = line;
    int startColumn = column;

    advance(); // Opening quote

    std::string value;
    bool terminated = false;

    while (current() != '\0') {
        if (current() == '"') {
            advance();
            terminated = true;
            break;
        }

        if (current() == '\n')
            break;

        if (current() == '\\') {
            advance();

            switch (current()) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;

                default:
                    value += '\\';
                    value += current();
                    break;
            }

            advance();
            continue;
        }

        value += current();
        advance();
    }

    if (!terminated) {
        tokens.push_back({
            TokenType::Invalid,
            "Unterminated string",
            startLine,
            startColumn
        });

        return;
    }

    tokens.push_back({
        TokenType::String,
        value,
        startLine,
        startColumn
    });
}

void Lexer::scanLabel() {
    int startLine = line;
    int startColumn = column;

    advance(); // Dot

    std::string value = ".";

    while (std::isalnum(static_cast<unsigned char>(current()))
        || current() == '_') {

        value += current();
        advance();
    }

    if (current() == ':') {
        advance();

        tokens.push_back({
            TokenType::Label,
            value,
            startLine,
            startColumn
        });
    } else {
        tokens.push_back({
            TokenType::Invalid,
            "Invalid label declaration",
            startLine,
            startColumn
        });
    }
}

void Lexer::scanToken() {
    char c = current();

    if (c == ' ' || c == '\t' || c == '\r') {
        advance();
        return;
    }

    if (c == ';') {
        skipComment();
        return;
    }

    if (c == '\n') {
        tokens.push_back({
            TokenType::NewLine,
            "\\n",
            line,
            column
        });

        advance();
        return;
    }

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        scanIdentifier();
        return;
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        scanNumber();
        return;
    }

    if (c == '"') {
        scanString();
        return;
    }

    if (c == '.') {
        scanLabel();
        return;
    }

    int startLine = line;
    int startColumn = column;

    switch (c) {
        case ':':
            advance();
            tokens.push_back({TokenType::Colon, ":", startLine, startColumn});
            return;

        case ',':
            advance();
            tokens.push_back({TokenType::Comma, ",", startLine, startColumn});
            return;

        case '=':
            advance();
            tokens.push_back({TokenType::Equal, "=", startLine, startColumn});
            return;

        default:
            advance();
            tokens.push_back({
                TokenType::Invalid,
                std::string("Unexpected character: ") + c,
                startLine,
                startColumn
            });
            return;
    }
}

std::vector<Token> Lexer::tokenize() {
    while (current() != '\0') {
        scanToken();
    }

    tokens.push_back({
        TokenType::EndOfFile,
        "",
        line,
        column
    });

    return std::move(tokens);
}

}
