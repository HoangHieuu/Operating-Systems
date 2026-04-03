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

