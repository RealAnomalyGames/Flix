import * as fs from "fs";

import { Lexer } from "./lexer/Lexer";
import { Parser } from "./parser/Parser";
import { Interpreter } from "./interpreter/Interpreter";
import { FlixError } from "./runtime/Errors";

const args = process.argv.slice(2);

if (args.length !== 1) {

    console.error("Usage:");
    console.error("npm start <program.flx>");

    process.exit(1);

}

const filename = args[0];

if (!filename.endsWith(".flx")) {

    console.error("Error:");
    console.error("Input file must have the .flx extension.");

    process.exit(1);

}

if (!fs.existsSync(filename)) {

    console.error(`Error: File '${filename}' does not exist.`);

    process.exit(1);

}

try {

    const source = fs.readFileSync(
        filename,
        "utf8"
    );

    const lexer = new Lexer(source);

    const tokens = lexer.tokenize();

    const parser = new Parser(tokens);

    const program = parser.parse();

    const interpreter = new Interpreter();

    interpreter.interpret(program);

}
catch (error) {

    if (error instanceof FlixError) {

        console.error("");

        console.error(`Flix ${error.name}`);

        console.error(
            `Line ${error.line}, Column ${error.column}`
        );

        console.error("");

        console.error(error.message);

    }
    else {

        console.error(error);

    }

}