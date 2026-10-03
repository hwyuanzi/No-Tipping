"""Student-owned strategy for No Tipping."""


def occupied_positions(state):
    return {block["position"] for block in state["board"]}


def legal_add_moves(state):
    occupied = occupied_positions(state)
    for weight in state["remaining"][state["player"]]:
        for position in range(-30, 31):
            if position not in occupied:
                yield {"position": position, "weight": weight}


def legal_remove_moves(state):
    return ({"position": block["position"]} for block in state["board"])


def stable_after_add(state, position, weight):
    left = state["torques"]["left"] - weight * (position + 3)
    right = state["torques"]["right"] - weight * (position + 1)
    return left <= 0 and right >= 0


def stable_after_remove(state, position):
    block = next(block for block in state["board"] if block["position"] == position)
    left = state["torques"]["left"] + block["weight"] * (position + 3)
    right = state["torques"]["right"] + block["weight"] * (position + 1)
    return left <= 0 and right >= 0


def choose_move(state):
    """Return {position, weight} during add, or {position} during remove."""
    if state["phase"] == "add":
        return next(legal_add_moves(state))
    return next(legal_remove_moves(state))
