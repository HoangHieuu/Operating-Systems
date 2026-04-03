#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math_logic.h"

int parse_operand(const char *token, double ans, double *value) {
    char *end = NULL;

    if (strcmp(token, "ANS") == 0) {
        *value = ans;
        return 0;
    }

    *value = strtod(token, &end);
    if (end == token || *end != '\0') {
        return 1;
    }

    return 0;
}

int calculate(double a, char op, double b, double *result) {
    switch (op) {
        case '+':
            *result = a + b;
            return 0;
        case '-':
            *result = a - b;
            return 0;
        case 'x':
            *result = a * b;
            return 0;
        case '/':
            if (fabs(b) < 1e-12) {
                return 1;
            }
            *result = a / b;
            return 0;
        case '%': {
            long long ia = (long long)a;
            long long ib = (long long)b;
            if (fabs(a - (double)ia) > 1e-12 || fabs(b - (double)ib) > 1e-12) {
                return 2;
            }
            if (ib == 0) {
                return 1;
            }
            *result = (double)(ia % ib);
            return 0;
        }
        default:
            return 2;
    }
}

void format_result(double value, char *out, int out_size) {
    int len;

    snprintf(out, out_size, "%.2f", value);
    len = (int)strlen(out);

    while (len > 0 && out[len - 1] == '0') {
        out[len - 1] = '\0';
        len--;
    }
    if (len > 0 && out[len - 1] == '.') {
        out[len - 1] = '\0';
    }
}
