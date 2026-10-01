"""Authoritative game rules. Torque units are kg*m (gravity omitted)."""
class IllegalMove(ValueError):
    pass


class Game:
    def __init__(self, k=15):
        if type(k) is not int or k < 1:
            raise ValueError('k must be a positive integer')
        self.k = k
        self.board = {-4: {'weight': 3, 'owner': None}}
        self.remaining = [list(range(1, k + 1)), list(range(1, k + 1))]
        self.turn = 0
        self.phase = 'add'
        self.winner = None
        self.reason = None

    def torques(self):
        # Clockwise negative: a mass right of a support contributes negatively.
        return tuple(-3 * (0 - support) - sum(b['weight'] * (p - support)
                     for p, b in self.board.items()) for support in (-3, -1))

    def state(self):
        left, right = self.torques()
        return {'protocol_version': 1, 'k': self.k, 'phase': self.phase,
                'player': self.turn, 'board': [dict(position=p, **b) for p, b in sorted(self.board.items())],
                'remaining': [list(r) for r in self.remaining],
                'torques': {'left': left, 'right': right},
                'winner': self.winner, 'reason': self.reason}

    def forfeit(self, reason):
        self.winner = 1 - self.turn
        self.reason = reason

    def apply(self, move):
        if self.winner is not None:
            raise IllegalMove('game is over')
        if not isinstance(move, dict) or type(move.get('position')) is not int:
            raise IllegalMove('position must be an integer')
        p = move['position']
        if not -30 <= p <= 30:
            raise IllegalMove('position must be between -30 and 30')
        if self.phase == 'add':
            w = move.get('weight')
            if type(w) is not int or w not in self.remaining[self.turn]:
                raise IllegalMove('weight is not in your inventory')
            if p in self.board:
                raise IllegalMove('position is occupied')
            self.board[p] = {'weight': w, 'owner': self.turn}
            self.remaining[self.turn].remove(w)
        else:
            if p not in self.board:
                raise IllegalMove('position is empty')
            del self.board[p]
        left, right = self.torques()
        if left > 0 or right < 0:
            self.forfeit('tipping')
        else:
            self.turn = 1 - self.turn
            if not any(self.remaining):
                self.phase = 'remove'
