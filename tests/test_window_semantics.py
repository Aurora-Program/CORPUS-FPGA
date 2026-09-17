import unittest

from host.compare_window_semantics import model, model_details


class WindowSemanticsTests(unittest.TestCase):
    def test_model_matches_three_round_details(self):
        for index in range(64):
            request = [
                0x3F ^ ((index * 0x15) & 0xFF),
                0x50 ^ ((index * 0x29) & 0xFF),
                (0x15 if index & 1 else 0x55) ^ ((index * 0x09) & 0xFF),
                (index << 2) & 0xFF,
                0xFF if index & 4 else 0x55,
                (index * 0x11) & 0xFF,
                0xFF if index & 2 else 0x55,
            ]
            self.assertEqual(model(request), model_details(request)[0])


if __name__ == '__main__':
    unittest.main()