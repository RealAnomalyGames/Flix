import { Token, TokenType } from "../lexer/Token";
import { SyntaxError } from "../runtime/Errors";
import {
    Program,
    Statement,
    Expression,
    LetStatement,
    PrintStatement,
    NumberLiteral,
    StringLiteral,
    Identifier,
    BinaryExpression
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

        const token = this.peek();

        throw new SyntaxError(
            `Unexpected token '${token.value}'.`,
            token.line,
            token.column
        );

    }

    private letStatement(): LetStatement {

        const name = this.consume(TokenType.Identifier).value;

        this.consume(TokenType.Equals);

        return new LetStatement(
            name,
            this.expression()
        );

    }

    private printStatement(): PrintStatement {

        this.consume(TokenType.LeftParen);

        const value = this.expression();

        this.consume(TokenType.RightParen);

        return new PrintStatement(value);

    }

    private expression(): Expression {
        return this.addition();
    }

    private addition(): Expression {

        let expression = this.multiplication();

        while (
            this.match(TokenType.Plus) ||
            this.match(TokenType.Minus)
        ) {

            const operator = this.previous().value;

            const right = this.multiplication();

            expression = new BinaryExpression(
                expression,
                operator,
                right
            );

        }

        return expression;

    }

    private multiplication(): Expression {

        let expression = this.primary();

        while (
            this.match(TokenType.Star) ||
            this.match(TokenType.Slash)
        ) {

            const operator = this.previous().value;

            const right = this.primary();

            expression = new BinaryExpression(
                expression,
                operator,
                right
            );

        }

        return expression;

    }

    private primary(): Expression {

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

        if (this.match(TokenType.LeftParen)) {

            const expression = this.expression();

            this.consume(TokenType.RightParen);

            return expression;

        }

        const token = this.peek();

        throw new SyntaxError(
            "Expected expression.",
            token.line,
            token.column
        );

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

        const token = this.peek();

        throw new SyntaxError(
            `Expected ${TokenType[type]}.`,
            token.line,
            token.column
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