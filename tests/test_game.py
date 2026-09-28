import tempfile
import unittest
from pathlib import Path
import sys
from unittest.mock import patch
from notipping.game import Game, IllegalMove
from notipping.runner import get_move, load_bots, tournament

class RulesTests(unittest.TestCase):
    def test_initial_torque_includes_board(self):
        self.assertEqual(Game().torques(), (-6, 6))

    def test_support_positions_allowed(self):
        g = Game(1)
        g.apply({'position': -3, 'weight': 1})
        g.apply({'position': -1, 'weight': 1})
        self.assertIsNone(g.winner)
        self.assertEqual(g.phase, 'remove')
        self.assertEqual(g.turn, 0)
        g.apply({'position': -1})  # first player removes opponent's block
        self.assertIsNone(g.winner)

    def test_occupied_and_invalid_move_do_not_mutate(self):
        g = Game()
        before = g.state()
        for move in [{'position': -4, 'weight': 1}, {'position': True, 'weight': 1}, {'position': 31, 'weight': 1}, {'position': 0, 'weight': 25}]:
            with self.assertRaises(IllegalMove):
                g.apply(move)
            self.assertEqual(g.state(), before)

    def test_zero_torque_is_stable(self):
        g = Game(1)
        g.apply({'position': 5, 'weight': 1})
        self.assertEqual(g.torques()[1], 0)
        self.assertIsNone(g.winner)

    def test_tip_loses(self):
        g = Game()
        g.apply({'position': 30, 'weight': 15})
        self.assertEqual(g.winner, 1)
        self.assertEqual(g.reason, 'tipping')

    def test_initial_block_removable(self):
        g = Game(1)
        g.apply({'position': -3, 'weight': 1})
        g.apply({'position': -1, 'weight': 1})
        g.apply({'position': -4})
        self.assertNotIn(-4, g.board)
        self.assertEqual(g.winner, 1)

    def test_weight_bounds(self):
        for k in [0, 25, True, 1.5]:
            with self.assertRaises(ValueError): Game(k)

class RunnerTests(unittest.TestCase):
    def test_double_round_robin(self):
        bots = load_bots(Path(__file__).resolve().parents[1] / 'bots.json')
        r = tournament(bots, k=2, clock_seconds=3)
        self.assertEqual(len(r['games']), 2)
        self.assertEqual(r['games'][0]['players'], list(reversed(r['games'][1]['players'])))
        self.assertEqual(sum(r['scores'].values()), 2)
        self.assertTrue(all(g['reason'] == 'tipping' for g in r['games']))

    def test_tournament_reports_each_round_before_continuing(self):
        bots = load_bots(Path(__file__).resolve().parents[1] / 'bots.json')[:2]
        callbacks = []

        def fake_play(ordered, k, clock_seconds, game_id, on_progress):
            players = [bot['name'] for bot in ordered]
            return {'players': players, 'winner': players[0], 'reason': 'test',
                    'clock_seconds': clock_seconds, 'game_id': game_id, 'frames': []}

        with patch('notipping.runner.play', side_effect=fake_play):
            result = tournament(bots, k=1, clock_seconds=120,
                                on_game_complete=lambda game, pair_index, round_number, count:
                                callbacks.append((game, pair_index, round_number, count)),
                                pairing=[bots[0]['name'], bots[1]['name']])

        self.assertEqual([entry[2] for entry in callbacks], [1, 2])
        self.assertEqual([entry[1:] for entry in callbacks], [(0, 1, 1), (0, 2, 1)])
        self.assertEqual(callbacks[0][0]['pairing_id'], callbacks[1][0]['pairing_id'])
        self.assertEqual(result['games'][0]['players'], list(reversed(result['games'][1]['players'])))

    def test_failed_bots(self):
        with tempfile.TemporaryDirectory() as cwd:
            for code, timeout in [('import time; time.sleep(2)', .05), ('print("invalid")', 1), ('print("x" * 70000)', 1), ('raise SystemExit(2)', 1)]:
                with self.assertRaises(ValueError):
                    get_move({'command': [sys.executable, '-c', code], 'cwd': cwd}, Game().state(), timeout)

if __name__ == '__main__': unittest.main()
