"""Submission integrity and official persistent-protocol checks for Fulcrum."""
import json
from pathlib import Path
import random
import shutil
import subprocess
import unittest
from notipping.game import Game
from notipping.runner import BotSession

ROOT = Path(__file__).resolve().parents[1]


def safe_moves(game):
    left, right = game.torques()
    if game.phase == 'add':
        return [{'position': p, 'weight': w}
                for w in game.remaining[game.turn] for p in range(-30, 31)
                if p not in game.board and left - w * (p + 3) <= 0 and right - w * (p + 1) >= 0]
    return [{'position': p} for p, b in game.board.items()
            if left + b['weight'] * (p + 3) <= 0 and right + b['weight'] * (p + 1) >= 0]


class FulcrumTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if shutil.which('c++') is None:
            raise unittest.SkipTest('C++17 compiler is unavailable')
        subprocess.run(['./build'], cwd=ROOT / 'bots/fulcrum', check=True, capture_output=True)

    def test_organizer_files_identical(self):
        for name in ('README.md', 'build', 'runner.cpp', 'strategy.hpp'):
            self.assertEqual((ROOT / 'bots/templates/cpp' / name).read_bytes(),
                             (ROOT / 'bots/fulcrum' / name).read_bytes(), name)

    def test_persistent_protocol_and_safe_fallbacks(self):
        rng = random.Random(20261003)
        session = BotSession({'cwd': str(ROOT / 'bots/fulcrum'), 'command': ['./bot']})
        try:
            cases = []
            for k in (1, 4, 8, 15, 24):
                for trial in range(3):
                    game = Game(k)
                    for _ in range(2 * k):
                        if rng.random() < 0.22:
                            cases.append(game.state())
                        legal = safe_moves(game)
                        if not legal:
                            break
                        game.apply(rng.choice(legal))
                    if game.phase == 'remove':
                        cases.append(game.state())
                        for _ in range(len(game.board)):
                            legal = safe_moves(game)
                            if not legal:
                                cases.append(game.state())
                                break
                            game.apply(rng.choice(legal))
                            if rng.random() < 0.3:
                                cases.append(game.state())
            # Explicit zero-torque boundary and short-clock state.
            boundary = Game(1)
            boundary.apply({'position': 5, 'weight': 1})
            cases.append(boundary.state())
            for i, state in enumerate(cases):
                state.update(clocks=[0.15, 0.15], game_id=f'property-{i}', ply=i)
                move = session.get_move(state, 1)
                clone = Game(state['k'])
                clone.board = {b['position']: {'weight': b['weight'], 'owner': b['owner']} for b in state['board']}
                clone.remaining = [list(x) for x in state['remaining']]
                clone.turn, clone.phase = state['player'], state['phase']
                legal = safe_moves(clone)
                if legal:
                    self.assertIn(move, legal, json.dumps(state))
                clone.apply(move)  # Even a forced tipping move must be structurally valid.
                if legal:
                    self.assertIsNone(clone.winner)
        finally:
            session.close()
