import { Token, TokenType } from "../lexer/Token";
import {
    Program,
    Statement,
    Expression,
    LetStatement,
    PrintStatement,
    NumberLiteral,
    StringLiteral,
    Identifier
} from "../ast/Nodes";

export class Parser {

    private current = 0;

    constructor(private tokens: Token[]) {}

    public parse(): Program {

        const statements: Statement[] = [];

        while (!this.isAtEnd()) {
            statements.push(this.statement());
        }

        return new Program(statements);

    }

    private statement(): Statement {

        if (this.match(TokenType.Let)) {
            return this.letStatement();
        }

        if (this.match(TokenType.Prt)) {
            return this.printStatement();
        }

        throw new Error(`Unexpected token '${this.peek().value}'`);

    }

    private letStatement(): LetStatement {

        const name = this.consume(TokenType.Identifier).value;

        this.consume(TokenType.Equals);

        const value = this.expression();

        return new LetStatement(name, value);

    }

    private printStatement(): PrintStatement {

        this.consume(TokenType.LeftParen);

        const expression = this.expression();

        this.consume(TokenType.RightParen);

        return new PrintStatement(expression);

    }

    private expression(): Expression {

        if (this.match(TokenType.Number)) {
            return new NumberLiteral(
                Number(this.previous().value)
            );
        }

        if (this.match(TokenType.String)) {
            return new StringLiteral(
                this.previous().value
            );
        }

        if (this.match(TokenType.Identifier)) {
            return new Identifier(
                this.previous().value
            );
        }

        throw new Error("Expected expression");

    }

    private match(type: TokenType): boolean {

        if (!this.check(type)) {
            return false;
        }

        this.current++;

        return true;

    }

    private consume(type: TokenType): Token {

        if (this.check(type)) {
            return this.tokens[this.current++];
        }

        throw new Error(
            `Expected ${TokenType[type]}, found ${TokenType[this.peek().type]}`
        );

    }

    private check(type: TokenType): boolean {

        if (this.isAtEnd()) {
            return false;
        }

        return this.peek().type === type;

    }

    private previous(): Token {
        return this.tokens[this.current - 1];
    }

    private peek(): Token {
        return this.tokens[this.current];
    }

    private isAtEnd(): boolean {
        return this.peek().type === TokenType.EOF;
    }

}