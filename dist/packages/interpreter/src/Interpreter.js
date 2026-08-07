"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Interpreter = void 0;
const Nodes_1 = require("../../core/src/ast/Nodes");
const Environment_1 = require("../../core/src/runtime/Environment");
const Errors_1 = require("../../core/src/runtime/Errors");
class Interpreter {
    environment = new Environment_1.Environment();
    interpret(program) {
        for (const statement of program.statements) {
            this.execute(statement);
        }
    }
    execute(statement) {
        if (statement instanceof Nodes_1.LetStatement) {
            this.executeLet(statement);
            return;
        }
        if (statement instanceof Nodes_1.PrintStatement) {
            this.executePrint(statement);
            return;
        }
        throw new Error("Unknown statement.");
    }
    executeLet(statement) {
        const value = this.evaluate(statement.value);
        this.environment.define(statement.name, value);
    }
    executePrint(statement) {
        console.log(this.evaluate(statement.expression));
    }
    evaluate(expression) {
        if (expression instanceof Nodes_1.NumberLiteral) {
            return expression.value;
        }
        if (expression instanceof Nodes_1.StringLiteral) {
            return expression.value;
        }
        if (expression instanceof Nodes_1.Identifier) {
            return this.environment.get(expression.name);
        }
        if (expression instanceof Nodes_1.BinaryExpression) {
            const left = this.evaluate(expression.left);
            const right = this.evaluate(expression.right);
            if (typeof left !== "number" ||
                typeof right !== "number") {
                throw new Errors_1.RuntimeError("Arithmetic requires numeric operands.", 0, 0);
            }
            switch (expression.operator) {
                case "+":
                    return left + right;
                case "-":
                    return left - right;
                case "*":
                    return left * right;
                case "/":
                    return left / right;
            }
        }
        throw new Error("Unknown expression.");
    }
}
exports.Interpreter = Interpreter;
