"""Example student strategy using only the documented state object."""


def stable(left, right):
    return left <= 0 and right >= 0


def choose_move(state):
    board = state["board"]
    left, right = state["torques"]["left"], state["torques"]["right"]
    occupied = {block["position"] for block in board}
    if state["phase"] == "add":
        for weight in sorted(state["remaining"][state["player"]]):
            for position in range(-30, 31):
                if position in occupied:
                    continue
                if stable(left - weight * (position + 3),
                          right - weight * (position + 1)):
                    return {"position": position, "weight": weight}
        return {"position": next(p for p in range(-30, 31) if p not in occupied),
                "weight": min(state["remaining"][state["player"]])}
    for block in board:
        position, weight = block["position"], block["weight"]
        if stable(left + weight * (position + 3),
                  right + weight * (position + 1)):
            return {"position": position}
    return {"position": board[0]["position"]}
