import { Token, TokenType } from "./Token";

export class Lexer {

    private position = 0;
    private line = 1;
    private column = 1;

    constructor(private source: string) {}

    public tokenize(): Token[] {

        const tokens: Token[] = [];

        while (!this.isAtEnd()) {

            const token = this.nextToken();

            if (token !== null) {
                tokens.push(token);
            }

        }

        tokens.push(new Token(
            TokenType.EOF,
            "",
            this.line,
            this.column
        ));

        return tokens;

    }

    private nextToken(): Token | null {

        this.skipWhitespace();

        if (this.isAtEnd()) {
            return null;
        }

        const startLine = this.line;
        const startColumn = this.column;

        const c = this.advance();

        switch (c) {

            case '=':
                return new Token(TokenType.Equals, c, startLine, startColumn);

            case '+':
                return new Token(TokenType.Plus, c, startLine, startColumn);

            case '-':
                return new Token(TokenType.Minus, c, startLine, startColumn);

            case '*':
                return new Token(TokenType.Star, c, startLine, startColumn);

            case '/':
                return new Token(TokenType.Slash, c, startLine, startColumn);

            case '(':
                return new Token(TokenType.LeftParen, c, startLine, startColumn);

            case ')':
                return new Token(TokenType.RightParen, c, startLine, startColumn);

            case ',':
                return new Token(TokenType.Comma, c, startLine, startColumn);

        }

        if (this.isDigit(c)) {
            return this.readNumber(c, startLine, startColumn);
        }

        if (this.isLetter(c)) {
            return this.readIdentifier(c, startLine, startColumn);
        }

        return new Token(
            TokenType.Unknown,
            c,
            startLine,
            startColumn
        );

    }

    private readNumber(first: string, line: number, column: number): Token {

        let value = first;

        while (!this.isAtEnd() && this.isDigit(this.peek())) {
            value += this.advance();
        }

        return new Token(TokenType.Number, value, line, column);

    }

    private readIdentifier(first: string, line: number, column: number): Token {

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
                return new Token(TokenType.Let, value, line, column);

            case "prt":
                return new Token(TokenType.Prt, value, line, column);

            default:
                return new Token(TokenType.Identifier, value, line, column);

        }

    }

    private skipWhitespace(): void {

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

    private peek(): string {
        return this.source[this.position];
    }

    private advance(): string {

        const c = this.source[this.position++];

        if (c === '\n') {
            this.line++;
            this.column = 1;
        } else {
            this.column++;
        }

        return c;

    }

    private isAtEnd(): boolean {
        return this.position >= this.source.length;
    }

    private isDigit(c: string): boolean {
        return c >= '0' && c <= '9';
    }

    private isLetter(c: string): boolean {
        return /[A-Za-z]/.test(c);
    }

}