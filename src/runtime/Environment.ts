export class Environment {

    private variables = new Map<string, unknown>();

    public define(name: string, value: unknown): void {

        if (this.variables.has(name)) {
            throw new Error(
                `Variable '${name}' already exists.`
            );
        }

        this.variables.set(name, value);

    }

    public assign(name: string, value: unknown): void {

        if (!this.variables.has(name)) {
            throw new Error(
                `Undefined variable '${name}'.`
            );
        }

        this.variables.set(name, value);

    }

    public get(name: string): unknown {

        if (!this.variables.has(name)) {
            throw new Error(
                `Undefined variable '${name}'.`
            );
        }

        return this.variables.get(name);

    }

    public exists(name: string): boolean {
        return this.variables.has(name);
    }

}