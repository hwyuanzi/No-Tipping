import random

def stable(board):
    torques = [-3 * (0 - s) - sum(b['weight'] * (b['position'] - s) for b in board) for s in (-3, -1)]
    return torques[0] <= 0 and torques[1] >= 0

def choose(state):
    board = state['board']
    if state['phase'] == 'add':
        # The course baseline chooses a random weight, then its leftmost safe slot.
        weight = random.choice(state['remaining'][state['player']])
        empty = [p for p in range(-30, 31) if all(b['position'] != p for b in board)]
        for p in empty:
            if stable(board + [{'position': p, 'weight': weight}]):
                return {'position': p, 'weight': weight}
        return {'position': empty[0], 'weight': weight}  # unavoidable loss for chosen weight
    safe = [b for b in board if stable([x for x in board if x['position'] != b['position']])]
    return {'position': random.choice(safe or board)['position']}
