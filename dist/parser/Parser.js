"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Parser = void 0;
const Token_1 = require("../lexer/Token");
const Nodes_1 = require("../ast/Nodes");
class Parser {
    tokens;
    current = 0;
    constructor(tokens) {
        this.tokens = tokens;
    }
    parse() {
        const statements = [];
        while (!this.isAtEnd()) {
            statements.push(this.statement());
        }
        return new Nodes_1.Program(statements);
    }
    statement() {
        if (this.match(Token_1.TokenType.Let)) {
            return this.letStatement();
        }
        if (this.match(Token_1.TokenType.Prt)) {
            return this.printStatement();
        }
        throw new Error(`Unexpected token '${this.peek().value}'`);
    }
    letStatement() {
        const name = this.consume(Token_1.TokenType.Identifier).value;
        this.consume(Token_1.TokenType.Equals);
        const value = this.expression();
        return new Nodes_1.LetStatement(name, value);
    }
    printStatement() {
        this.consume(Token_1.TokenType.LeftParen);
        const expression = this.expression();
        this.consume(Token_1.TokenType.RightParen);
        return new Nodes_1.PrintStatement(expression);
    }
    expression() {
        if (this.match(Token_1.TokenType.Number)) {
            return new Nodes_1.NumberLiteral(Number(this.previous().value));
        }
        if (this.match(Token_1.TokenType.String)) {
            return new Nodes_1.StringLiteral(this.previous().value);
        }
        if (this.match(Token_1.TokenType.Identifier)) {
            return new Nodes_1.Identifier(this.previous().value);
        }
        throw new Error("Expected expression");
    }
    match(type) {
        if (!this.check(type)) {
            return false;
        }
        this.current++;
        return true;
    }
    consume(type) {
        if (this.check(type)) {
            return this.tokens[this.current++];
        }
        throw new Error(`Expected ${Token_1.TokenType[type]}, found ${Token_1.TokenType[this.peek().type]}`);
    }
    check(type) {
        if (this.isAtEnd()) {
            return false;
        }
        return this.peek().type === type;
    }
    previous() {
        return this.tokens[this.current - 1];
    }
    peek() {
        return this.tokens[this.current];
    }
    isAtEnd() {
        return this.peek().type === Token_1.TokenType.EOF;
    }
}
exports.Parser = Parser;
