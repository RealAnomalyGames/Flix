export enum TokenType {

    // Special
    EOF,

    // Literals
    Identifier,
    Number,
    String,

    // Keywords
    Let,
    Prt,

    // Operators
    Equals,
    Plus,
    Minus,
    Star,
    Slash,

    // Symbols
    LeftParen,
    RightParen,
    Comma,

    // Unknown
    Unknown
}

export class Token {

    constructor(
        public type: TokenType,
        public value: string,
        public line: number,
        public column: number
    ) {}

    public toString(): string {

        return `${TokenType[this.type]}("${this.value}") @ ${this.line}:${this.column}`;

    }

}