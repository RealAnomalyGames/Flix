"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Token = exports.TokenType = void 0;
var TokenType;
(function (TokenType) {
    // Literals
    TokenType[TokenType["Identifier"] = 0] = "Identifier";
    TokenType[TokenType["Number"] = 1] = "Number";
    TokenType[TokenType["String"] = 2] = "String";
    TokenType[TokenType["True"] = 3] = "True";
    TokenType[TokenType["False"] = 4] = "False";
    TokenType[TokenType["Nil"] = 5] = "Nil";
    // Keywords
    TokenType[TokenType["Let"] = 6] = "Let";
    TokenType[TokenType["Prt"] = 7] = "Prt";
    // Operators
    TokenType[TokenType["Plus"] = 8] = "Plus";
    TokenType[TokenType["Minus"] = 9] = "Minus";
    TokenType[TokenType["Star"] = 10] = "Star";
    TokenType[TokenType["Slash"] = 11] = "Slash";
    TokenType[TokenType["Equals"] = 12] = "Equals";
    // Delimiters
    TokenType[TokenType["LeftParen"] = 13] = "LeftParen";
    TokenType[TokenType["RightParen"] = 14] = "RightParen";
    TokenType[TokenType["Comma"] = 15] = "Comma";
    // Special
    TokenType[TokenType["EOF"] = 16] = "EOF";
    // Unknown
    TokenType[TokenType["Unknown"] = 17] = "Unknown";
})(TokenType || (exports.TokenType = TokenType = {}));
class Token {
    type;
    value;
    line;
    column;
    constructor(type, value, line, column) {
        this.type = type;
        this.value = value;
        this.line = line;
        this.column = column;
    }
}
exports.Token = Token;
