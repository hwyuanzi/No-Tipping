"""Organizer-owned wrapper; do not modify for a student submission."""
import json
import sys
import traceback

from strategy import choose_move


for line in sys.stdin:
    try:
        state = json.loads(line)
        move = choose_move(state)
        if not isinstance(move, dict):
            raise TypeError("choose_move(state) must return a dictionary")
        print(json.dumps(move, separators=(",", ":")), flush=True)
    except Exception as exc:
        print(f"strategy error: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise SystemExit(1)
