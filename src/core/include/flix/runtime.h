#ifndef FLIX_RUNTIME_H
#define FLIX_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum FlixType {
    FLIX_INT = 0,
    FLIX_FLOAT = 1,
    FLIX_STRING = 2,
    FLIX_BOOL = 3
} FlixType;

void flix_runtime_init(void);
void flix_runtime_shutdown(void);

void flix_define(
    const char* name,
    FlixType type,
    const char* initial_value
);

void flix_output(const char* operand, int newline);

void flix_mov(
    const char* destination,
    const char* source
);

void flix_arithmetic(
    const char* operation,
    const char* destination,
    const char* operand
);

void flix_unary(
    const char* operation,
    const char* destination
);

int flix_eval_condition(const char* expression);

int flix_exit_code(const char* value);

#ifdef __cplusplus
}
#endif

#endif
