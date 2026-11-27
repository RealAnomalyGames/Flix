#include "runtime.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLIX_MAX_VARIABLES 4096

typedef struct FlixVariable {
    char* name;
    char* value;
    FlixType type;
} FlixVariable;

static FlixVariable variables[FLIX_MAX_VARIABLES];
static size_t variable_count = 0;

static char* copy_text(const char* text) {
    if (!text) {
        text = "";
    }

    size_t length = strlen(text);
    char* result = (char*)malloc(length + 1);

    if (!result) {
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    memcpy(result, text, length + 1);
    return result;
}

static char* trim_copy(const char* text) {
    while (*text && isspace((unsigned char)*text)) {
        ++text;
    }

    size_t length = strlen(text);

    while (length > 0 &&
           isspace((unsigned char)text[length - 1])) {
        --length;
    }

    char* result = (char*)malloc(length + 1);

    if (!result) {
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    memcpy(result, text, length);
    result[length] = '\0';

    return result;
}

static int is_integer_text(const char* value) {
    if (!value || !*value) {
        return 0;
    }

    if (*value == '-' || *value == '+') {
        ++value;
    }

    if (!*value) {
        return 0;
    }

    while (*value) {
        if (!isdigit((unsigned char)*value)) {
            return 0;
        }

        ++value;
    }

    return 1;
}

static char* normalize_integer(const char* input) {
    const char* p = input;
    int negative = 0;

    if (*p == '-') {
        negative = 1;
        ++p;
    } else if (*p == '+') {
        ++p;
    }

    while (*p == '0' && p[1] != '\0') {
        ++p;
    }

    if (strcmp(p, "0") == 0) {
        negative = 0;
    }

    size_t length = strlen(p);
    char* result = (char*)malloc(length + (negative ? 2 : 1));

    if (!result) {
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    size_t offset = 0;

    if (negative) {
        result[offset++] = '-';
    }

    memcpy(result + offset, p, length + 1);
    return result;
}

static void split_sign(
    const char* input,
    int* negative,
    const char** digits
) {
    *negative = 0;
    *digits = input;

    if (**digits == '-') {
        *negative = 1;
        ++(*digits);
    } else if (**digits == '+') {
        ++(*digits);
    }
}

static int compare_abs(const char* a, const char* b) {
    while (*a == '0' && a[1]) ++a;
    while (*b == '0' && b[1]) ++b;

    size_t al = strlen(a);
    size_t bl = strlen(b);

    if (al != bl) {
        return al < bl ? -1 : 1;
    }

    int result = strcmp(a, b);
    return result < 0 ? -1 : result > 0 ? 1 : 0;
}

static char* add_abs(const char* a, const char* b) {
    size_t al = strlen(a);
    size_t bl = strlen(b);
    size_t capacity = (al > bl ? al : bl) + 2;

    char* result = (char*)malloc(capacity);

    if (!result) {
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    result[capacity - 1] = '\0';

    size_t ai = al;
    size_t bi = bl;
    size_t ri = capacity - 2;
    int carry = 0;

    while (ai || bi || carry) {
        int sum = carry;

        if (ai) sum += a[--ai] - '0';
        if (bi) sum += b[--bi] - '0';

        result[ri--] = (char)('0' + (sum % 10));
        carry = sum / 10;
    }

    char* normalized = copy_text(result + ri + 1);
    free(result);

    return normalized;
}

static char* subtract_abs(const char* a, const char* b) {
    size_t al = strlen(a);
    size_t bl = strlen(b);

    char* result = (char*)malloc(al + 1);

    if (!result) {
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    result[al] = '\0';

    size_t ai = al;
    size_t bi = bl;
    int borrow = 0;

    while (ai) {
        int digit = (a[--ai] - '0') - borrow;

        if (bi) {
            digit -= b[--bi] - '0';
        }

        borrow = digit < 0;

        if (borrow) {
            digit += 10;
        }

        result[ai] = (char)('0' + digit);
    }

    char* normalized = normalize_integer(result);
    free(result);

    return normalized;
}

static char* signed_add(const char* a, const char* b) {
    int an, bn;
    const char *ad, *bd;

    split_sign(a, &an, &ad);
    split_sign(b, &bn, &bd);

    char* result;

    if (an == bn) {
        char* magnitude = add_abs(ad, bd);

        if (an && strcmp(magnitude, "0") != 0) {
            size_t length = strlen(magnitude);
            result = (char*)malloc(length + 2);
            result[0] = '-';
            memcpy(result + 1, magnitude, length + 1);
            free(magnitude);
        } else {
            result = magnitude;
        }
    } else {
        int comparison = compare_abs(ad, bd);

        if (comparison == 0) {
            result = copy_text("0");
        } else if (comparison > 0) {
            char* magnitude = subtract_abs(ad, bd);

            if (an) {
                size_t length = strlen(magnitude);
                result = (char*)malloc(length + 2);
                result[0] = '-';
                memcpy(result + 1, magnitude, length + 1);
                free(magnitude);
            } else {
                result = magnitude;
            }
        } else {
            char* magnitude = subtract_abs(bd, ad);

            if (bn) {
                size_t length = strlen(magnitude);
                result = (char*)malloc(length + 2);
                result[0] = '-';
                memcpy(result + 1, magnitude, length + 1);
                free(magnitude);
            } else {
                result = magnitude;
            }
        }
    }

    return result;
}

static char* signed_negate(const char* value) {
    if (strcmp(value, "0") == 0) {
        return copy_text("0");
    }

    if (*value == '-') {
        return copy_text(value + 1);
    }

    size_t length = strlen(value);
    char* result = (char*)malloc(length + 2);

    result[0] = '-';
    memcpy(result + 1, value, length + 1);

    return result;
}

static char* multiply_abs(const char* a, const char* b) {
    size_t al = strlen(a);
    size_t bl = strlen(b);

    size_t count = al + bl;
    int* digits = (int*)calloc(count, sizeof(int));

    if (!digits) {
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    for (size_t i = al; i > 0; --i) {
        for (size_t j = bl; j > 0; --j) {
            digits[(i - 1) + (j - 1) + 1] +=
                (a[i - 1] - '0') * (b[j - 1] - '0');
        }
    }

    for (size_t i = count; i > 1; --i) {
        digits[i - 2] += digits[i - 1] / 10;
        digits[i - 1] %= 10;
    }

    size_t start = 0;

    while (start + 1 < count && digits[start] == 0) {
        ++start;
    }

    char* result = (char*)malloc(count - start + 1);

    if (!result) {
        free(digits);
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    size_t offset = 0;

    for (size_t i = start; i < count; ++i) {
        result[offset++] = (char)('0' + digits[i]);
    }

    result[offset] = '\0';

    free(digits);
    return result;
}

static char* signed_multiply(const char* a, const char* b) {
    int an, bn;
    const char *ad, *bd;

    split_sign(a, &an, &ad);
    split_sign(b, &bn, &bd);

    char* magnitude = multiply_abs(ad, bd);

    if ((an != bn) && strcmp(magnitude, "0") != 0) {
        size_t length = strlen(magnitude);
        char* result = (char*)malloc(length + 2);

        result[0] = '-';
        memcpy(result + 1, magnitude, length + 1);

        free(magnitude);
        return result;
    }

    return magnitude;
}

static char* divide_abs(
    const char* dividend,
    const char* divisor,
    char** remainder_out
) {
    if (strcmp(divisor, "0") == 0) {
        fprintf(stderr, "Flix runtime: division by zero\n");
        exit(1);
    }

    size_t length = strlen(dividend);
    char* quotient = (char*)malloc(length + 1);
    char* remainder = copy_text("0");

    if (!quotient) {
        fprintf(stderr, "Flix runtime: out of memory\n");
        exit(1);
    }

    for (size_t i = 0; i < length; ++i) {
        size_t rlen = strlen(remainder);
        char* expanded = (char*)malloc(rlen + 2);

        memcpy(expanded, remainder, rlen);
        expanded[rlen] = dividend[i];
        expanded[rlen + 1] = '\0';

        free(remainder);
        remainder = normalize_integer(expanded);
        free(expanded);

        int digit = 0;

        while (compare_abs(remainder, divisor) >= 0) {
            char* next = subtract_abs(remainder, divisor);
            free(remainder);
            remainder = next;
            ++digit;
        }

        quotient[i] = (char)('0' + digit);
    }

    quotient[length] = '\0';

    char* normalized = normalize_integer(quotient);
    free(quotient);

    *remainder_out = remainder;
    return normalized;
}

static char* signed_divide(
    const char* a,
    const char* b,
    int want_remainder
) {
    int an, bn;
    const char *ad, *bd;

    split_sign(a, &an, &ad);
    split_sign(b, &bn, &bd);

    char* remainder = NULL;
    char* quotient = divide_abs(ad, bd, &remainder);

    char* selected = want_remainder ? remainder : quotient;

    if (want_remainder) {
        free(quotient);
    } else {
        free(remainder);
    }

    int negative = want_remainder ? an : (an != bn);

    if (negative && strcmp(selected, "0") != 0) {
        size_t length = strlen(selected);
        char* result = (char*)malloc(length + 2);

        result[0] = '-';
        memcpy(result + 1, selected, length + 1);

        free(selected);
        return result;
    }

    return selected;
}

static FlixVariable* find_variable(const char* name) {
    for (size_t i = 0; i < variable_count; ++i) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i];
        }
    }

    return NULL;
}

static char* unquote(const char* input) {
    size_t length = strlen(input);

    if (length < 2 || input[0] != '"' ||
        input[length - 1] != '"') {
        return copy_text(input);
    }

    char* result = (char*)malloc(length);
    size_t offset = 0;

    for (size_t i = 1; i < length - 1; ++i) {
        if (input[i] == '\\' && i + 1 < length - 1) {
            ++i;

            switch (input[i]) {
                case 'n': result[offset++] = '\n'; break;
                case 't': result[offset++] = '\t'; break;
                case 'r': result[offset++] = '\r'; break;
                case '\\': result[offset++] = '\\'; break;
                case '"': result[offset++] = '"'; break;
                default: result[offset++] = input[i]; break;
            }
        } else {
            result[offset++] = input[i];
        }
    }

    result[offset] = '\0';
    return result;
}

static char* resolve_value(const char* operand) {
    char* trimmed = trim_copy(operand);

    FlixVariable* variable = find_variable(trimmed);

    if (variable) {
        char* result = copy_text(variable->value);
        free(trimmed);
        return result;
    }

    char* result = unquote(trimmed);
    free(trimmed);

    return result;
}

void flix_runtime_init(void) {
    variable_count = 0;
}

void flix_runtime_shutdown(void) {
    for (size_t i = 0; i < variable_count; ++i) {
        free(variables[i].name);
        free(variables[i].value);
    }

    variable_count = 0;
}

void flix_define(
    const char* name,
    FlixType type,
    const char* initial_value
) {
    if (variable_count >= FLIX_MAX_VARIABLES) {
        fprintf(stderr, "Flix runtime: too many variables\n");
        exit(1);
    }

    if (find_variable(name)) {
        fprintf(stderr, "Flix runtime: duplicate variable '%s'\n", name);
        exit(1);
    }

    FlixVariable* variable = &variables[variable_count++];

    variable->name = copy_text(name);
    variable->type = type;

    if (type == FLIX_STRING) {
        variable->value = unquote(initial_value);
    } else if (type == FLIX_INT) {
        variable->value = normalize_integer(initial_value);
    } else {
        variable->value = copy_text(initial_value);
    }
}

void flix_output(const char* operand, int newline) {
    char* value = resolve_value(operand);

    fputs(value, stdout);

    if (newline) {
        fputc('\n', stdout);
    }

    free(value);
}

void flix_mov(const char* destination, const char* source) {
    FlixVariable* variable = find_variable(destination);

    if (!variable) {
        fprintf(stderr, "Flix runtime: unknown variable '%s'\n", destination);
        exit(1);
    }

    char* value = resolve_value(source);

    if (variable->type == FLIX_INT) {
        if (!is_integer_text(value)) {
            fprintf(stderr, "Flix runtime: invalid integer assignment\n");
            free(value);
            exit(1);
        }

        char* normalized = normalize_integer(value);
        free(value);
        value = normalized;
    }

    free(variable->value);
    variable->value = value;
}

void flix_arithmetic(
    const char* operation,
    const char* destination,
    const char* operand
) {
    FlixVariable* variable = find_variable(destination);

    if (!variable || variable->type != FLIX_INT) {
        fprintf(stderr, "Flix runtime: arithmetic requires an integer variable\n");
        exit(1);
    }

    char* right = resolve_value(operand);

    if (!is_integer_text(right)) {
        fprintf(stderr, "Flix runtime: invalid integer operand\n");
        free(right);
        exit(1);
    }

    char* result = NULL;

    if (strcmp(operation, "add") == 0) {
        result = signed_add(variable->value, right);
    } else if (strcmp(operation, "sub") == 0) {
        char* negative = signed_negate(right);
        result = signed_add(variable->value, negative);
        free(negative);
    } else if (strcmp(operation, "mul") == 0) {
        result = signed_multiply(variable->value, right);
    } else if (strcmp(operation, "div") == 0) {
        result = signed_divide(variable->value, right, 0);
    } else if (strcmp(operation, "mod") == 0) {
        result = signed_divide(variable->value, right, 1);
    } else {
        fprintf(stderr, "Flix runtime: unknown arithmetic operation\n");
        free(right);
        exit(1);
    }

    free(right);
    free(variable->value);
    variable->value = result;
}

void flix_unary(
    const char* operation,
    const char* destination
) {
    FlixVariable* variable = find_variable(destination);

    if (!variable || variable->type != FLIX_INT) {
        fprintf(stderr, "Flix runtime: unary operation requires an integer variable\n");
        exit(1);
    }

    char* result = NULL;

    if (strcmp(operation, "inc") == 0) {
        result = signed_add(variable->value, "1");
    } else if (strcmp(operation, "dec") == 0) {
        result = signed_add(variable->value, "-1");
    } else if (strcmp(operation, "neg") == 0) {
        result = signed_negate(variable->value);
    } else {
        fprintf(stderr, "Flix runtime: unknown unary operation\n");
        exit(1);
    }

    free(variable->value);
    variable->value = result;
}

/*
 * Expression evaluation is intentionally kept separate from code
 * generation. The initial backend supports a single comparison here.
 * Logical-expression parsing is added in the next runtime increment.
 */

static int compare_values(const char* left, const char* right) {
    if (is_integer_text(left) && is_integer_text(right)) {
        int ln, rn;
        const char *ld, *rd;

        split_sign(left, &ln, &ld);
        split_sign(right, &rn, &rd);

        if (ln != rn) {
            return ln ? -1 : 1;
        }

        int result = compare_abs(ld, rd);
        return ln ? -result : result;
    }

    int result = strcmp(left, right);
    return result < 0 ? -1 : result > 0 ? 1 : 0;
}

int flix_eval_condition(const char* expression) {
    const char* operators[] = {
        "==", "!=", "<=", ">=", "<", ">"
    };

    for (size_t op = 0; op < 6; ++op) {
        const char* found = strstr(expression, operators[op]);

        if (!found) {
            continue;
        }

        size_t left_length = (size_t)(found - expression);

        char* left_text = (char*)malloc(left_length + 1);
        memcpy(left_text, expression, left_length);
        left_text[left_length] = '\0';

        char* right_text = trim_copy(
            found + strlen(operators[op])
        );

        char* left = resolve_value(left_text);
        char* right = resolve_value(right_text);

        int comparison = compare_values(left, right);
        int result = 0;

        if (op == 0) result = comparison == 0;
        if (op == 1) result = comparison != 0;
        if (op == 2) result = comparison <= 0;
        if (op == 3) result = comparison >= 0;
        if (op == 4) result = comparison < 0;
        if (op == 5) result = comparison > 0;

        free(left_text);
        free(right_text);
        free(left);
        free(right);

        return result;
    }

    char* value = resolve_value(expression);
    int result = strcmp(value, "true") == 0 || strcmp(value, "1") == 0;
    free(value);

    return result;
}

int flix_exit_code(const char* value) {
    char* resolved = resolve_value(value);
    int result = atoi(resolved);
    free(resolved);
    return result;
}
