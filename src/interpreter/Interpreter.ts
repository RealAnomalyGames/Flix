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
} from "../ast/Nodes";

import { Environment } from "../runtime/Environment";

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

        throw new Error(
            `Unknown statement '${statement.kind}'.`
        );

    }

    private executeLet(statement: LetStatement): void {

        const value = this.evaluate(statement.value);

        this.environment.define(
            statement.name,
            value
        );

    }

    private executePrint(statement: PrintStatement): void {

        const value = this.evaluate(
            statement.expression
        );

        console.log(value);

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
            throw new Error(
                "Binary expressions are not implemented yet."
            );
        }

        throw new Error(
            `Unknown expression '${expression.kind}'.`
        );

    }

}