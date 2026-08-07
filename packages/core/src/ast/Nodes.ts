export abstract class Node {

    constructor(
        public readonly kind: string
    ) {}

}

// -------------------------
// Statements
// -------------------------

export abstract class Statement extends Node {}

export class LetStatement extends Statement {

    constructor(
        public readonly name: string,
        public readonly value: Expression
    ) {
        super("LetStatement");
    }

}

export class PrintStatement extends Statement {

    constructor(
        public readonly expression: Expression
    ) {
        super("PrintStatement");
    }

}

// -------------------------
// Expressions
// -------------------------

export abstract class Expression extends Node {}

export class NumberLiteral extends Expression {

    constructor(
        public readonly value: number
    ) {
        super("NumberLiteral");
    }

}

export class StringLiteral extends Expression {

    constructor(
        public readonly value: string
    ) {
        super("StringLiteral");
    }

}

export class BooleanLiteral extends Expression {

    constructor(
        public readonly value: boolean
    ) {
        super("BooleanLiteral");
    }

}

export class NilLiteral extends Expression {

    constructor() {
        super("NilLiteral");
    }

}

export class Identifier extends Expression {

    constructor(
        public readonly name: string
    ) {
        super("Identifier");
    }

}

export class BinaryExpression extends Expression {

    constructor(
        public readonly left: Expression,
        public readonly operator: string,
        public readonly right: Expression
    ) {
        super("BinaryExpression");
    }

}

export class Program extends Node {

    constructor(
        public readonly statements: Statement[]
    ) {
        super("Program");
    }

}