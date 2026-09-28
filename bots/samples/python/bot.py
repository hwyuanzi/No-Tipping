"""Small standard-library example bot for the No Tipping JSON protocol."""
import json
import sys


def stable(left, right):
    return left <= 0 and right >= 0


def choose(state):
    board = state["board"]
    left = state["torques"]["left"]
    right = state["torques"]["right"]
    occupied = {block["position"] for block in board}

    if state["phase"] == "add":
        for weight in sorted(state["remaining"][state["player"]]):
            for position in range(-30, 31):
                if position in occupied:
                    continue
                next_left = left - weight * (position + 3)
                next_right = right - weight * (position + 1)
                if stable(next_left, next_right):
                    return {"position": position, "weight": weight}
        # A legal move is still required when every option tips the board.
        return {"position": next(p for p in range(-30, 31) if p not in occupied),
                "weight": min(state["remaining"][state["player"]])}

    for block in board:
        position, weight = block["position"], block["weight"]
        next_left = left + weight * (position + 3)
        next_right = right + weight * (position + 1)
        if stable(next_left, next_right):
            return {"position": position}
    return {"position": board[0]["position"]}


if __name__ == "__main__":
    state = json.loads(sys.stdin.readline())
    print(json.dumps(choose(state), separators=(",", ":")))
