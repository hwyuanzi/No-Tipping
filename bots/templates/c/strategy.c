#include "strategy.h"
#include <stdio.h>
/* Student-owned strategy; runner.c handles protocol I/O. */
int choose_move(const char *state, char *output, size_t output_size) { (void)state; return snprintf(output, output_size, "{\"position\":-3,\"weight\":1}") > 0; }
