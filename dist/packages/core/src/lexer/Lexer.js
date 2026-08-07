"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Lexer = void 0;
const Token_1 = require("./Token");
class Lexer {
    source;
    position = 0;
    line = 1;
    column = 1;
    constructor(source) {
        this.source = source;
    }
    tokenize() {
        const tokens = [];
        while (!this.isAtEnd()) {
            this.skipWhitespaceAndComments();
            if (this.isAtEnd()) {
                break;
            }
            const token = this.nextToken();
            if (token !== null) {
                tokens.push(token);
            }
        }
        tokens.push(new Token_1.Token(Token_1.TokenType.EOF, "", this.line, this.column));
        return tokens;
    }
    nextToken() {
        const startLine = this.line;
        const startColumn = this.column;
        const c = this.advance();
        switch (c) {
            case '=':
                return new Token_1.Token(Token_1.TokenType.Equals, c, startLine, startColumn);
            case '+':
                return new Token_1.Token(Token_1.TokenType.Plus, c, startLine, startColumn);
            case '-':
                return new Token_1.Token(Token_1.TokenType.Minus, c, startLine, startColumn);
            case '*':
                return new Token_1.Token(Token_1.TokenType.Star, c, startLine, startColumn);
            case '/':
                return new Token_1.Token(Token_1.TokenType.Slash, c, startLine, startColumn);
            case '(':
                return new Token_1.Token(Token_1.TokenType.LeftParen, c, startLine, startColumn);
            case ')':
                return new Token_1.Token(Token_1.TokenType.RightParen, c, startLine, startColumn);
            case ',':
                return new Token_1.Token(Token_1.TokenType.Comma, c, startLine, startColumn);
            case '"':
                return this.readString(startLine, startColumn);
        }
        if (this.isDigit(c)) {
            return this.readNumber(c, startLine, startColumn);
        }
        if (this.isLetter(c) || c === '_') {
            return this.readIdentifier(c, startLine, startColumn);
        }
        return new Token_1.Token(Token_1.TokenType.Unknown, c, startLine, startColumn);
    }
    readString(line, column) {
        let value = "";
        while (!this.isAtEnd() && this.peek() !== '"') {
            value += this.advance();
        }
        if (this.isAtEnd()) {
            return new Token_1.Token(Token_1.TokenType.Unknown, value, line, column);
        }
        // Consume closing quote
        this.advance();
        return new Token_1.Token(Token_1.TokenType.String, value, line, column);
    }
    readNumber(first, line, column) {
        let value = first;
        while (!this.isAtEnd() && this.isDigit(this.peek())) {
            value += this.advance();
        }
        return new Token_1.Token(Token_1.TokenType.Number, value, line, column);
    }
    readIdentifier(first, line, column) {
        let value = first;
        while (!this.isAtEnd()) {
            const c = this.peek();
            if (!this.isLetter(c) && !this.isDigit(c) && c !== '_') {
                break;
            }
            value += this.advance();
        }
        switch (value) {
            case "let":
                return new Token_1.Token(Token_1.TokenType.Let, value, line, column);
            case "prt":
                return new Token_1.Token(Token_1.TokenType.Prt, value, line, column);
            case "true":
                return new Token_1.Token(Token_1.TokenType.True, value, line, column);
            case "false":
                return new Token_1.Token(Token_1.TokenType.False, value, line, column);
            case "nil":
                return new Token_1.Token(Token_1.TokenType.Nil, value, line, column);
            default:
                return new Token_1.Token(Token_1.TokenType.Identifier, value, line, column);
        }
    }
    skipWhitespaceAndComments() {
        while (!this.isAtEnd()) {
            const c = this.peek();
            // Whitespace
            if (c === ' ' || c === '\t' || c === '\r' || c === '\n') {
                this.advance();
                continue;
            }
            // Single-line comment //
            if (c === '/' && this.peekNext() === '/') {
                while (!this.isAtEnd() && this.peek() !== '\n') {
                    this.advance();
                }
                continue;
            }
            break;
        }
    }
    peek() {
        return this.source[this.position];
    }
    peekNext() {
        if (this.position + 1 >= this.source.length) {
            return '\0';
        }
        return this.source[this.position + 1];
    }
    advance() {
        const c = this.source[this.position++];
        if (c === '\n') {
            this.line++;
            this.column = 1;
        }
        else {
            this.column++;
        }
        return c;
    }
    isAtEnd() {
        return this.position >= this.source.length;
    }
    isDigit(c) {
        return c >= '0' && c <= '9';
    }
    isLetter(c) {
        return /[A-Za-z]/.test(c);
    }
}
exports.Lexer = Lexer;
