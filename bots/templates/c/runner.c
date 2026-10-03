/* Organizer-owned wrapper. It parses the stable tournament protocol. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strategy.h"

static const char *value(const char *s, const char *key) { char needle[80]; snprintf(needle, sizeof needle, "\"%s\"", key); const char *p = strstr(s, needle); return p ? strchr(p, ':') + 1 : NULL; }
static int integer(const char *s, const char *key, int fallback) { const char *p = value(s, key); char *end; long n; if (!p || *p == 'n') return fallback; n = strtol(p, &end, 10); return end == p ? fallback : (int)n; }
static int next_int(const char **p, const char *end, int *out) { while (*p < end && (**p < '0' || **p > '9') && **p != '-') ++*p; if (*p >= end) return 0; *out = (int)strtol(*p, (char **)p, 10); return 1; }
static void quoted(const char *p, char *out, size_t size) { const char *a = strchr(p, '"'); size_t n = 0; if (!a) { if (size) *out = 0; return; } ++a; while (a[n] && a[n] != '"' && n + 1 < size) { out[n] = a[n]; ++n; } if (size) out[n] = 0; }
static int parse_state(const char *s, GameState *g) {
    memset(g, 0, sizeof *g); g->winner = integer(s, "winner", -1); g->ply = integer(s, "ply", -1);
    g->protocol_version = integer(s, "protocol_version", 0); g->k = integer(s, "k", 0); g->player = integer(s, "player", 0);
    const char *p = value(s, "phase"); if (p) while (*p == ' ' || *p == '\t') ++p; if (!p || *p != '"') return 0; quoted(p, g->phase, sizeof g->phase);
    g->torque_left = integer(strstr(s, "\"torques\""), "left", 0); g->torque_right = integer(strstr(s, "\"torques\""), "right", 0);
    const char *b = value(s, "board"); if (!b) return 0; const char *end = strchr(b, ']');
    while ((b = strchr(b, '{')) && b < end && g->board_count < 128) { const char *e = strchr(b, '}'); if (!e || e > end) return 0; char item[256]; size_t n = (size_t)(e - b + 1); if (n >= sizeof item) return 0; memcpy(item, b, n); item[n] = 0; Block *x = &g->board[g->board_count++]; x->position = integer(item, "position", 0); x->weight = integer(item, "weight", 0); x->owner = integer(item, "owner", -1); b = e + 1; }
    const char *r = value(s, "remaining"); if (!r) return 0; for (int player = 0; player < 2; ++player) { r = strchr(r, '['); if (!r) return 0; const char *e = strchr(r, ']'); if (!e) return 0; const char *q = r + 1; while (g->remaining_count[player] < 128 && next_int(&q, e, &g->remaining[player][g->remaining_count[player]])) ++g->remaining_count[player]; r = e + 1; }
    const char *c = value(s, "clocks"); if (c) sscanf(c, " [%lf , %lf]", &g->clocks[0], &g->clocks[1]);
    p = value(s, "reason"); if (p) while (*p == ' ' || *p == '\t') ++p; if (p && *p == '"') quoted(p, g->reason, sizeof g->reason);
    p = value(s, "game_id"); if (p) while (*p == ' ' || *p == '\t') ++p; if (p && *p == '"') quoted(p, g->game_id, sizeof g->game_id);
    p = value(s, "players"); if (p) { const char *q = strchr(p, '['); for (int i = 0; q && i < 2; ++i) { q = strchr(q, '"'); if (!q) break; quoted(q, g->players[i], sizeof g->players[i]); q = strchr(q + 1, '"'); if (q) ++q; } }
    return 1;
}
int main(void) {
    char line[65536]; GameState state; Move move;
    while (fgets(line, sizeof line, stdin)) {
        if (!parse_state(line, &state) || !choose_move(&state, &move)) { fputs("strategy error\n", stderr); return 1; }
        if (move.has_weight) printf("{\"position\":%d,\"weight\":%d}\n", move.position, move.weight); else printf("{\"position\":%d}\n", move.position); fflush(stdout);
    }
    return 0;
}
