import * as fs from "fs";
import { Lexer } from "./lexer/Lexer";

const source = fs.readFileSync(
    "examples/hello.flx",
    "utf8"
);

const lexer = new Lexer(source);

const tokens = lexer.tokenize();

for (const token of tokens) {
    console.log(token.toString());
}