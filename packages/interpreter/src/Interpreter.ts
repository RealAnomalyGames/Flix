import {
    Program,
    Statement,
    Expression,
    LetStatement,
    PrintStatement,
    NumberLiteral,
    StringLiteral,
    Identifier,
    BinaryExpression
} from "../../core/src/ast/Nodes";

import { Environment } from "../../core/src/runtime/Environment";
import { RuntimeError } from "../../core/src/runtime/Errors";

export class Interpreter {

    private environment = new Environment();

    public interpret(program: Program): void {

        for (const statement of program.statements) {
            this.execute(statement);
        }

    }

    private execute(statement: Statement): void {

        if (statement instanceof LetStatement) {
            this.executeLet(statement);
            return;
        }

        if (statement instanceof PrintStatement) {
            this.executePrint(statement);
            return;
        }

        throw new Error("Unknown statement.");

    }

    private executeLet(statement: LetStatement): void {

        const value = this.evaluate(statement.value);

        this.environment.define(
            statement.name,
            value
        );

    }

    private executePrint(statement: PrintStatement): void {

        console.log(
            this.evaluate(statement.expression)
        );

    }

    private evaluate(expression: Expression): unknown {

        if (expression instanceof NumberLiteral) {
            return expression.value;
        }

        if (expression instanceof StringLiteral) {
            return expression.value;
        }

        if (expression instanceof Identifier) {
            return this.environment.get(
                expression.name
            );
        }

        if (expression instanceof BinaryExpression) {

            const left = this.evaluate(expression.left);
            const right = this.evaluate(expression.right);

            if (
                typeof left !== "number" ||
                typeof right !== "number"
            ) {
                throw new RuntimeError(
                    "Arithmetic requires numeric operands.",
                    0,
                    0
                );
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

        throw new Error(
            "Unknown expression."
        );

    }

}