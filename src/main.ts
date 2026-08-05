import * as fs from "fs";

import { Lexer } from "./lexer/Lexer";
import { Parser } from "./parser/Parser";
import { Interpreter } from "./interpreter/Interpreter";

const source = fs.readFileSync(
    "examples/variables.flx",
    "utf8"
);

const lexer = new Lexer(source);

const tokens = lexer.tokenize();

const parser = new Parser(tokens);

const program = parser.parse();

const interpreter = new Interpreter();

interpreter.interpret(program);