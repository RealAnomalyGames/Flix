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
            const token = this.nextToken();
            if (token !== null) {
                tokens.push(token);
            }
        }
        tokens.push(new Token_1.Token(Token_1.TokenType.EOF, "", this.line, this.column));
        return tokens;
    }
    nextToken() {
        this.skipWhitespace();
        if (this.isAtEnd()) {
            return null;
        }
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
        }
        if (this.isDigit(c)) {
            return this.readNumber(c, startLine, startColumn);
        }
        if (this.isLetter(c)) {
            return this.readIdentifier(c, startLine, startColumn);
        }
        return new Token_1.Token(Token_1.TokenType.Unknown, c, startLine, startColumn);
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
            default:
                return new Token_1.Token(Token_1.TokenType.Identifier, value, line, column);
        }
    }
    skipWhitespace() {
        while (!this.isAtEnd()) {
            const c = this.peek();
            if (c === ' ' || c === '\t' || c === '\r') {
                this.advance();
            }
            else if (c === '\n') {
                this.advance();
            }
            else {
                break;
            }
        }
    }
    peek() {
        return this.source[this.position];
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
