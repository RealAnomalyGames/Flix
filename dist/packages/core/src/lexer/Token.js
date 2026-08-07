"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Token = exports.TokenType = void 0;
var TokenType;
(function (TokenType) {
    // Special
    TokenType[TokenType["EOF"] = 0] = "EOF";
    // Literals
    TokenType[TokenType["Identifier"] = 1] = "Identifier";
    TokenType[TokenType["Number"] = 2] = "Number";
    TokenType[TokenType["String"] = 3] = "String";
    // Keywords
    TokenType[TokenType["Let"] = 4] = "Let";
    TokenType[TokenType["Prt"] = 5] = "Prt";
    // Operators
    TokenType[TokenType["Equals"] = 6] = "Equals";
    TokenType[TokenType["Plus"] = 7] = "Plus";
    TokenType[TokenType["Minus"] = 8] = "Minus";
    TokenType[TokenType["Star"] = 9] = "Star";
    TokenType[TokenType["Slash"] = 10] = "Slash";
    // Symbols
    TokenType[TokenType["LeftParen"] = 11] = "LeftParen";
    TokenType[TokenType["RightParen"] = 12] = "RightParen";
    TokenType[TokenType["Comma"] = 13] = "Comma";
    // Unknown
    TokenType[TokenType["Unknown"] = 14] = "Unknown";
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
    toString() {
        return `${TokenType[this.type]}("${this.value}") @ ${this.line}:${this.column}`;
    }
}
exports.Token = Token;
