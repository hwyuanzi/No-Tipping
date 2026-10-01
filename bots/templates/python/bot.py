"""Starter template. Replace choose_move with your strategy."""
import json
import sys


def choose_move(state):
    # TODO: Return one move dictionary based on the current state.
    # Placement: {"position": -3, "weight": 2}
    # Removal:   {"position": -4}
    raise NotImplementedError("implement choose_move")


for line in sys.stdin:
    state = json.loads(line)
    move = choose_move(state)
    print(json.dumps(move), flush=True)
