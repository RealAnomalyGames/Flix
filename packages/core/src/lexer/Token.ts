export enum TokenType {

    // Literals
    Identifier,
    Number,
    String,
    True,
    False,
    Nil,

    // Keywords
    Let,
    Prt,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Equals,

    // Delimiters
    LeftParen,
    RightParen,
    Comma,

    // Special
    EOF,
    
    // Unknown
    Unknown

}

export class Token {

    constructor(
        public readonly type: TokenType,
        public readonly value: string,
        public readonly line: number,
        public readonly column: number
    ) {}

}