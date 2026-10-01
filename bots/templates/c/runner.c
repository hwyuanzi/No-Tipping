/* Organizer-owned wrapper. Compile with strategy.c. */
#include <stdio.h>
#include "strategy.h"
int main(void) { char state[65536], move[256]; while (fgets(state, sizeof state, stdin)) { if (!choose_move(state, move, sizeof move)) { fputs("strategy returned no move\n", stderr); return 1; } puts(move); fflush(stdout); } return 0; }
