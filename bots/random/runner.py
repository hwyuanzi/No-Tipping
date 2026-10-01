"""Organizer-owned wrapper for the bundled random strategy."""
import json
import sys
from strategy import choose

for line in sys.stdin:
    try:
        print(json.dumps(choose(json.loads(line)), separators=(",", ":")), flush=True)
    except Exception as exc:
        print(f"strategy error: {exc}", file=sys.stderr)
        raise SystemExit(1)
