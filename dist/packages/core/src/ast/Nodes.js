"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Program = exports.BinaryExpression = exports.Identifier = exports.StringLiteral = exports.NumberLiteral = exports.Expression = exports.PrintStatement = exports.LetStatement = exports.Statement = exports.Node = void 0;
class Node {
    kind;
    constructor(kind) {
        this.kind = kind;
    }
}
exports.Node = Node;
// -------------------------
// Statements
// -------------------------
class Statement extends Node {
}
exports.Statement = Statement;
class LetStatement extends Statement {
    name;
    value;
    constructor(name, value) {
        super("LetStatement");
        this.name = name;
        this.value = value;
    }
}
exports.LetStatement = LetStatement;
class PrintStatement extends Statement {
    expression;
    constructor(expression) {
        super("PrintStatement");
        this.expression = expression;
    }
}
exports.PrintStatement = PrintStatement;
// -------------------------
// Expressions
// -------------------------
class Expression extends Node {
}
exports.Expression = Expression;
class NumberLiteral extends Expression {
    value;
    constructor(value) {
        super("NumberLiteral");
        this.value = value;
    }
}
exports.NumberLiteral = NumberLiteral;
class StringLiteral extends Expression {
    value;
    constructor(value) {
        super("StringLiteral");
        this.value = value;
    }
}
exports.StringLiteral = StringLiteral;
class Identifier extends Expression {
    name;
    constructor(name) {
        super("Identifier");
        this.name = name;
    }
}
exports.Identifier = Identifier;
class BinaryExpression extends Expression {
    left;
    operator;
    right;
    constructor(left, operator, right) {
        super("BinaryExpression");
        this.left = left;
        this.operator = operator;
        this.right = right;
    }
}
exports.BinaryExpression = BinaryExpression;
class Program extends Node {
    statements;
    constructor(statements) {
        super("Program");
        this.statements = statements;
    }
}
exports.Program = Program;
