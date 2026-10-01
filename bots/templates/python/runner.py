"""Organizer-owned JSON wrapper. Students should edit strategy.py only."""
import json
import sys
import traceback

from strategy import choose_move


def main():
    for line in sys.stdin:
        try:
            state = json.loads(line)
            move = choose_move(state)
            if not isinstance(move, dict):
                raise TypeError("choose_move(state) must return a dictionary")
            sys.stdout.write(json.dumps(move, separators=(",", ":")) + "\n")
            sys.stdout.flush()
        except Exception as exc:
            print(f"strategy error: {exc}", file=sys.stderr)
            traceback.print_exc(file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
