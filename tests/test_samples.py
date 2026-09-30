import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from notipping.game import Game
from notipping.runner import get_move, load_bots


ROOT = Path(__file__).resolve().parents[1]


class SampleBotTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.temp_path = Path(cls.temp.name)
        cls.bots = {bot['name']: bot for bot in load_bots(ROOT / 'sample-bots.json')}

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
        self.check_protocol(self.bots['Sample Python'])

    def test_sample_manifest_gets_distinct_random_identities(self):
        bots = list(self.bots.values())
        self.assertTrue(all(bot['icon'] and bot['color'].startswith('#') for bot in bots))
        self.assertEqual(len({bot['icon'] for bot in bots}), len(bots))
        self.assertEqual(len({bot['color'].lower() for bot in bots}), len(bots))

    def test_explicit_identity_is_preserved(self):
        import json

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

    def test_c_and_cpp_samples_when_compilers_are_available(self):
        builds = [
            ('Sample C++', 'g++', ['-std=c++17', '-O2'], 'cpp', 'bot.cpp'),
            ('Sample C', 'gcc', ['-std=c11', '-O2'], 'c', 'bot.c'),
        ]
        for name, compiler, flags, folder, source in builds:
            with self.subTest(language=name):
                compiler_path = shutil.which(compiler)
                if compiler_path is None:
                    self.skipTest(f'{compiler} is not installed')
                executable = self.temp_path / (name.lower().replace(' ', '-') + '-bot')
                subprocess.run([compiler_path, *flags,
                                str(ROOT / 'bots' / 'samples' / folder / source),
                                '-o', str(executable)], check=True, capture_output=True)
                bot = dict(self.bots[name], command=[str(executable)])
                self.check_protocol(bot)

    def test_julia_sample_when_runtime_is_available(self):
        julia = shutil.which('julia')
        if julia is None:
            self.skipTest('Julia is not installed in this environment')
        bot = dict(self.bots['Sample Julia'],
                   command=[julia, '--startup-file=no', 'bot.jl'])
        self.check_protocol(bot)


if __name__ == '__main__':
    unittest.main()
