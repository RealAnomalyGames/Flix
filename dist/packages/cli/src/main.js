"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
const fs = __importStar(require("fs"));
const Lexer_1 = require("../../core/src/lexer/Lexer");
const Parser_1 = require("../../core/src/parser/Parser");
const Interpreter_1 = require("../../interpreter/src/Interpreter");
const Errors_1 = require("../../core/src/runtime/Errors");
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
    const source = fs.readFileSync(filename, "utf8");
    const lexer = new Lexer_1.Lexer(source);
    const tokens = lexer.tokenize();
    const parser = new Parser_1.Parser(tokens);
    const program = parser.parse();
    const interpreter = new Interpreter_1.Interpreter();
    interpreter.interpret(program);
}
catch (error) {
    if (error instanceof Errors_1.FlixError) {
        console.error("");
        console.error(`Flix ${error.name}`);
        console.error(`Line ${error.line}, Column ${error.column}`);
        console.error("");
        console.error(error.message);
    }
    else {
        console.error(error);
    }
}
