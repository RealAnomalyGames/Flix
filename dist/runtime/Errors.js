"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.RuntimeError = exports.SyntaxError = exports.FlixError = void 0;
class FlixError extends Error {
    line;
    column;
    constructor(message, line, column) {
        super(message);
        this.line = line;
        this.column = column;
    }
}
exports.FlixError = FlixError;
class SyntaxError extends FlixError {
    constructor(message, line, column) {
        super(message, line, column);
        this.name = "SyntaxError";
    }
}
exports.SyntaxError = SyntaxError;
class RuntimeError extends FlixError {
    constructor(message, line, column) {
        super(message, line, column);
        this.name = "RuntimeError";
    }
}
exports.RuntimeError = RuntimeError;
