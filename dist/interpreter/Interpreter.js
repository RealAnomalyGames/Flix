"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Interpreter = void 0;
const Nodes_1 = require("../ast/Nodes");
const Environment_1 = require("../runtime/Environment");
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
        throw new Error(`Unknown statement '${statement.kind}'.`);
    }
    executeLet(statement) {
        const value = this.evaluate(statement.value);
        this.environment.define(statement.name, value);
    }
    executePrint(statement) {
        const value = this.evaluate(statement.expression);
        console.log(value);
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
            throw new Error("Binary expressions are not implemented yet.");
        }
        throw new Error(`Unknown expression '${expression.kind}'.`);
    }
}
exports.Interpreter = Interpreter;
