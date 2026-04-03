#ifndef MATH_LOGIC_H
#define MATH_LOGIC_H

int parse_operand(const char *token, double ans, double *value);
int calculate(double a, char op, double b, double *result);
void format_result(double value, char *out, int out_size);

#endif
