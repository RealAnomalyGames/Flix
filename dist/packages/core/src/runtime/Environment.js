"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Environment = void 0;
class Environment {
    variables = new Map();
    define(name, value) {
        if (this.variables.has(name)) {
            throw new Error(`Variable '${name}' already exists.`);
        }
        this.variables.set(name, value);
    }
    assign(name, value) {
        if (!this.variables.has(name)) {
            throw new Error(`Undefined variable '${name}'.`);
        }
        this.variables.set(name, value);
    }
    get(name) {
        if (!this.variables.has(name)) {
            throw new Error(`Undefined variable '${name}'.`);
        }
        return this.variables.get(name);
    }
    exists(name) {
        return this.variables.has(name);
    }
}
exports.Environment = Environment;
