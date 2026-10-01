/* Minimal C11 example. It uses only the C standard library and the runner's JSON protocol. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int position; int weight; } Block;

static int field_int(const char *text, const char *key) {
    char token[80];
    snprintf(token, sizeof token, "\"%s\"", key);
    const char *field = strstr(text, token);
    if (!field || !(field = strchr(field, ':'))) return 0;
    return (int)strtol(field + 1, NULL, 10);
}

static int stable(int left, int right) { return left <= 0 && right >= 0; }

static int respond(const char *input) {
    const int player = field_int(input, "player");
    const int left = field_int(input, "left");
    const int right = field_int(input, "right");
    int occupied[61] = {0};
    Block board[64];
    int board_count = 0;

    char *array = strstr(input, "\"board\"");
    if (!array || !(array = strchr(array, '['))) return 1;
    char *cursor = array + 1;
    while ((cursor = strchr(cursor, '{')) != NULL && cursor < strchr(array, ']')) {
        char *end = strchr(cursor, '}');
        if (!end) return 1;
        char object[256];
        size_t length = (size_t)(end - cursor + 1);
        if (length >= sizeof object) return 1;
        memcpy(object, cursor, length);
        object[length] = '\0';
        const int position = field_int(object, "position");
        const int weight = field_int(object, "weight");
        if (position < -30 || position > 30 || board_count >= 64) return 1;
        board[board_count++] = (Block){position, weight};
        occupied[position + 30] = 1;
        cursor = end + 1;
    }

    if (strstr(input, "\"phase\"") && strstr(strstr(input, "\"phase\""), "\"add\"")) {
        char *remaining = strstr(input, "\"remaining\"");
        if (!remaining || !(remaining = strchr(remaining, '['))) return 1;
        for (int i = 0; i <= player; ++i) {
            remaining = strchr(remaining + 1, '[');
            if (!remaining) return 1;
            char *end = strchr(remaining, ']');
            if (!end) return 1;
            if (i == player) {
                int weights[25], count = 0;
                for (char *p = remaining + 1; p < end && count < 25;) {
                    if (*p >= '0' && *p <= '9') {
                        weights[count++] = (int)strtol(p, &p, 10);
                    } else ++p;
                }
                if (!count) return 1;
                for (int w = 0; w < count; ++w) {
                    const int weight = weights[w];
                    for (int position = -30; position <= 30; ++position) {
                        if (occupied[position + 30]) continue;
                        if (stable(left - weight * (position + 3), right - weight * (position + 1))) {
                            printf("{\"position\":%d,\"weight\":%d}\n", position, weight);
                            return 0;
                        }
                    }
                }
                for (int position = -30; position <= 30; ++position) {
                    if (!occupied[position + 30]) {
                        printf("{\"position\":%d,\"weight\":%d}\n", position, weights[0]);
                        return 0;
                    }
                }
            }
            remaining = end;
        }
    } else {
        for (int i = 0; i < board_count; ++i) {
            const Block block = board[i];
            if (stable(left + block.weight * (block.position + 3),
                       right + block.weight * (block.position + 1))) {
                printf("{\"position\":%d}\n", block.position);
                return 0;
            }
        }
        if (board_count) {
            printf("{\"position\":%d}\n", board[0].position);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    char input[65536];
    while (fgets(input, sizeof input, stdin)) {
        if (respond(input) != 0) return 1;
        fflush(stdout);
    }
    return 0;
}
