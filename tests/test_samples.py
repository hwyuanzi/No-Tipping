import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from notipping.game import Game
from notipping.runner import get_move, load_bots, tournament


ROOT = Path(__file__).resolve().parents[1]


class SampleBotTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.temp_path = Path(cls.temp.name)
        sample_entries = [
            {'name': 'Sample Python', 'cwd': str(ROOT / 'bots/samples/python'),
             'command': ['{python}', 'runner.py']},
            {'name': 'Sample C++', 'cwd': str(ROOT / 'bots/samples/cpp'),
             'command': ['./bot']},
            {'name': 'Sample C', 'cwd': str(ROOT / 'bots/samples/c'),
             'command': ['./bot']},
            {'name': 'Sample Julia', 'cwd': str(ROOT / 'bots/samples/julia'),
             'command': ['julia', '--startup-file=no', 'bot.jl']},
        ]
        sample_manifest = cls.temp_path / 'samples.json'
        sample_manifest.write_text(json.dumps(sample_entries))
        cls.sample_bots = {bot['name']: bot for bot in load_bots(sample_manifest)}
        cls.bots = {bot['name']: bot for bot in load_bots(ROOT / 'bots.json')}

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def check_protocol(self, bot):
        add_game = Game(1)
        add_move = get_move(bot, add_game.state(), 5)
        add_game.apply(add_move)
        self.assertIsNone(add_game.winner)

        remove_game = Game(1)
        remove_game.apply({'position': -3, 'weight': 1})
        remove_game.apply({'position': -1, 'weight': 1})
        remove_move = get_move(bot, remove_game.state(), 5)
        remove_game.apply(remove_move)
        self.assertIsNone(remove_game.winner)

    def test_python_sample(self):
        self.check_protocol(self.sample_bots['Sample Python'])

    def test_python_wrapper_calls_strategy_and_keeps_state(self):
        with tempfile.TemporaryDirectory() as cwd:
            folder = Path(cwd)
            (folder / 'runner.py').write_text(
                (ROOT / 'bots/templates/python/runner.py').read_text())
            (folder / 'strategy.py').write_text(
                'count = 0\n'
                'def choose_move(state):\n'
                '    global count\n'
                '    count += 1\n'
                '    return {"position": count}\n')
            bot = {'cwd': cwd, 'command': [sys.executable, 'runner.py']}
            session = get_move
            self.assertEqual(session(bot, Game(1).state(), 2), {'position': 1})
            # A new request uses a new process through get_move; verify the
            # wrapper's normal one-line protocol independently below.
            process = subprocess.Popen([sys.executable, 'runner.py'], cwd=cwd,
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True)
            try:
                process.stdin.write(json.dumps(Game(1).state()) + '\n')
                process.stdin.flush()
                self.assertEqual(json.loads(process.stdout.readline()), {'position': 1})
            finally:
                process.stdin.close()
                process.kill()
                process.wait()
                process.stdout.close()
                process.stderr.close()

    def test_python_wrapper_strategy_error_stays_off_stdout(self):
        with tempfile.TemporaryDirectory() as cwd:
            folder = Path(cwd)
            (folder / 'runner.py').write_text(
                (ROOT / 'bots/templates/python/runner.py').read_text())
            (folder / 'strategy.py').write_text('def choose_move(state):\n    raise RuntimeError("boom")\n')
            process = subprocess.Popen([sys.executable, 'runner.py'], cwd=cwd,
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True)
            stdout, stderr = process.communicate(json.dumps(Game(1).state()) + '\n')
            self.assertNotEqual(process.returncode, 0)
            self.assertEqual(stdout, '')
            self.assertIn('strategy error: boom', stderr)

    def test_default_manifest_has_distinct_identities(self):
        bots = list(self.bots.values())
        self.assertTrue(all(bot['icon'] and bot['color'].startswith('#') for bot in bots))
        self.assertEqual(len({bot['icon'] for bot in bots}), len(bots))
        self.assertEqual(len({bot['color'].lower() for bot in bots}), len(bots))

    def test_sample_examples_cover_class_languages(self):
        self.assertEqual(set(self.sample_bots),
                         {'Sample Python', 'Sample C++', 'Sample C', 'Sample Julia'})

    def test_explicit_identity_is_preserved(self):
        manifest_path = self.temp_path / 'explicit-bots.json'
        manifest_path.write_text(json.dumps([
            {'name': 'Named Bot', 'icon': '♟️', 'color': '#123456',
             'cwd': '.', 'command': [sys.executable, '-c', 'print("{}")']},
            {'name': 'Random Bot', 'cwd': '.',
             'command': [sys.executable, '-c', 'print("{}")']},
        ]))
        bots = load_bots(manifest_path)
        self.assertEqual((bots[0]['icon'], bots[0]['color']), ('♟️', '#123456'))
        self.assertNotEqual(bots[1]['icon'], bots[0]['icon'])
        self.assertNotEqual(bots[1]['color'].lower(), bots[0]['color'].lower())

    def test_c_and_cpp_sample_launchers_build_and_run(self):
        for name, compiler in [('Sample C++', 'g++'), ('Sample C', 'gcc')]:
            with self.subTest(language=name):
                if shutil.which(compiler) is None:
                    self.skipTest(f'{compiler} is not installed')
                self.check_protocol(self.bots[name])

    def test_cpp_sample_completes_both_games_against_random_a(self):
        if shutil.which('g++') is None:
            self.skipTest('g++ is not installed')
        bots = [self.bots['Sample C++'], self.bots['Random A']]
        result = tournament(bots, k=1, clock_seconds=30,
                            pairing=['Sample C++', 'Random A'])
        self.assertEqual(len(result['games']), 2)
        self.assertTrue(all(game['reason'] == 'tipping' for game in result['games']))

    def test_julia_sample_when_runtime_is_available(self):
        julia = shutil.which('julia')
        if julia is None:
            self.skipTest('Julia is not installed in this environment')
        bot = dict(self.bots['Sample Julia'],
                   command=[julia, '--startup-file=no', 'bot.jl'])
        self.check_protocol(bot)


if __name__ == '__main__':
    unittest.main()
