import unittest
from notipping.server import run_weights


class RunSettingsTests(unittest.TestCase):
    def test_allowed_weight_counts(self):
        for k in (1, 7, 15, 24):
            self.assertEqual(run_weights({'k': k}), k)

    def test_invalid_settings(self):
        for value in ({}, [], None, {'k': 0}, {'k': 25}, {'k': True},
                      {'k': 2.5}, {'k': '15'}):
            with self.subTest(value=value), self.assertRaises(ValueError):
                run_weights(value)
