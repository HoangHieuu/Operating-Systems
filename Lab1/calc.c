#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "math_logic.h"

#define INPUT_SIZE 256
#define TOKEN_SIZE 64


static void clear_screen(void) {
    if (isatty(STDOUT_FILENO)) {
        printf("\033[2J\033[H");
        fflush(stdout);
    }
}


static void wait_for_key(void) {
    int ch;

    if (isatty(STDIN_FILENO)) {
        struct termios oldt;
        struct termios newt;

        if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            ch = getchar();
            (void)ch;
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return;
        }
    }

    ch = getchar();
    (void)ch;
}

static double load_ans(void) {
    FILE *f = fopen(".calc_ans", "r");
    double ans = 0.0;

    if (!f) {
        return 0.0;
    }

    if (fscanf(f, "%lf", &ans) != 1) {
        ans = 0.0;
    }
    fclose(f);
    return ans;
}

static void save_ans(double ans) {
    FILE *f = fopen(".calc_ans", "w");

    if (!f) {
        return;
    }

    fprintf(f, "%.10g\n", ans);
    fclose(f);
}

int main(void) {
    char input[INPUT_SIZE];
    char left[TOKEN_SIZE], op_str[TOKEN_SIZE], right[TOKEN_SIZE], extra[TOKEN_SIZE];
    double ans = load_ans();

    clear_screen();

    while (1) {
        double a;
        double b;
        double result;
        int parsed;
        int status;
        char out[64];

        printf(">> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "EXIT") == 0) {
            break;
        }

        parsed = sscanf(input, "%63s %63s %63s %63s", left, op_str, right, extra);
        if (parsed != 3 || strlen(op_str) != 1) {
            printf("SYNTAX ERROR\n");
            wait_for_key();
            clear_screen();
            continue;
        }

        if (parse_operand(left, ans, &a) != 0 || parse_operand(right, ans, &b) != 0) {
            printf("SYNTAX ERROR\n");
            wait_for_key();
            clear_screen();
            continue;
        }

        status = calculate(a, op_str[0], b, &result);
        if (status == 1) {
            printf("MATH ERROR\n");
        } else if (status == 2) {
            printf("SYNTAX ERROR\n");
        } else {
            format_result(result, out, sizeof(out));
            printf("%s\n", out);
            ans = result;
            save_ans(ans);
        }

        wait_for_key();
        clear_screen();
    }

    return 0;
}

