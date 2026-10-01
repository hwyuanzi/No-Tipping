/* Starter template: implement choose_move and print one JSON move per input line. */
#include <stdio.h>

static void choose_move(const char *state, char *output, size_t output_size) {
    (void)state;
    /* TODO: Parse state and write, for example, {"position":-3,"weight":2}. */
    snprintf(output, output_size, "{}");
}

int main(void) {
    char state[65536];
    char move[256];
    while (fgets(state, sizeof state, stdin)) {
        choose_move(state, move, sizeof move);
        puts(move);
        fflush(stdout);
    }
    return 0;
}
