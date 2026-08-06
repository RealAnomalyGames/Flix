export class FlixError extends Error {

    constructor(
        message: string,
        public readonly line: number,
        public readonly column: number
    ) {
        super(message);
    }

}

export class SyntaxError extends FlixError {

    constructor(
        message: string,
        line: number,
        column: number
    ) {
        super(message, line, column);
        this.name = "SyntaxError";
    }

}

export class RuntimeError extends FlixError {

    constructor(
        message: string,
        line: number,
        column: number
    ) {
        super(message, line, column);
        this.name = "RuntimeError";
    }

}