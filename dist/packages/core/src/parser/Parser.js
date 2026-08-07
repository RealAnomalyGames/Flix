"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Parser = void 0;
const Token_1 = require("../lexer/Token");
const Nodes_1 = require("../ast/Nodes");
const Errors_1 = require("../runtime/Errors");
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
        const token = this.peek();
        throw new Errors_1.SyntaxError(`Unexpected token '${token.value}'.`, token.line, token.column);
    }
    letStatement() {
        const name = this.consume(Token_1.TokenType.Identifier).value;
        this.consume(Token_1.TokenType.Equals);
        return new Nodes_1.LetStatement(name, this.expression());
    }
    printStatement() {
        this.consume(Token_1.TokenType.LeftParen);
        const value = this.expression();
        this.consume(Token_1.TokenType.RightParen);
        return new Nodes_1.PrintStatement(value);
    }
    expression() {
        return this.addition();
    }
    addition() {
        let expression = this.multiplication();
        while (this.match(Token_1.TokenType.Plus) ||
            this.match(Token_1.TokenType.Minus)) {
            const operator = this.previous().value;
            const right = this.multiplication();
            expression = new Nodes_1.BinaryExpression(expression, operator, right);
        }
        return expression;
    }
    multiplication() {
        let expression = this.primary();
        while (this.match(Token_1.TokenType.Star) ||
            this.match(Token_1.TokenType.Slash)) {
            const operator = this.previous().value;
            const right = this.primary();
            expression = new Nodes_1.BinaryExpression(expression, operator, right);
        }
        return expression;
    }
    primary() {
        if (this.match(Token_1.TokenType.Number)) {
            return new Nodes_1.NumberLiteral(Number(this.previous().value));
        }
        if (this.match(Token_1.TokenType.String)) {
            return new Nodes_1.StringLiteral(this.previous().value);
        }
        if (this.match(Token_1.TokenType.True)) {
            return new Nodes_1.BooleanLiteral(true);
        }
        if (this.match(Token_1.TokenType.False)) {
            return new Nodes_1.BooleanLiteral(false);
        }
        if (this.match(Token_1.TokenType.Nil)) {
            return new Nodes_1.NilLiteral();
        }
        if (this.match(Token_1.TokenType.Identifier)) {
            return new Nodes_1.Identifier(this.previous().value);
        }
        if (this.match(Token_1.TokenType.LeftParen)) {
            const expression = this.expression();
            this.consume(Token_1.TokenType.RightParen);
            return expression;
        }
        const token = this.peek();
        throw new Errors_1.SyntaxError("Expected expression.", token.line, token.column);
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
        const token = this.peek();
        throw new Errors_1.SyntaxError(`Expected ${Token_1.TokenType[type]}.`, token.line, token.column);
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
