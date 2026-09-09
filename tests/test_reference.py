import unittest
from itertools import product
from reference.trigate import (solve, forward, majority, encode, decode,
                                WAIT, CHANGED, CLOSED, CONFLICT)
from reference.network import Cell, Network, FractalTensor


class ReferenceTests(unittest.TestCase):
    def test_book_examples(self):
        self.assertEqual(solve(0, 0, 1, 2, 0).values, (0, 0, 1, 0, 0))
        self.assertEqual(solve(1, 1, 0, 0, 2).values, (1, 1, 0, 0, 1))
        self.assertEqual(solve(0, 2, 2, 1, 0).values, (0, 1, 1, 1, 0))
        x = solve(0, 2, 2, 0, 0)
        self.assertEqual(x.values, (0, 2, 2, 0, 0))
        self.assertEqual(x.support, 0b00000111)  # (B,M)=00,01,10; excludes 11
        self.assertEqual(x.status, WAIT)
        self.assertEqual(solve(1, 2, 2, 2, 1).status, WAIT)
        self.assertEqual(solve(1, 1, 0, 0, 0).status, CONFLICT)
        self.assertEqual(solve(1, 1, 0, 0, 1).status, CLOSED)
        self.assertFalse(solve(1, 1, 0, 0, 1).direct)

    def test_residual_is_not_a_vote_or_orientation(self):
        for base in set(__import__('itertools').permutations((1, 2, 2))):
            self.assertEqual(forward(*base), (2, 1))
        for base in set(__import__('itertools').permutations((0, 1, 2))):
            self.assertEqual(forward(*base), (2, 2))
        self.assertEqual(forward(0, 2, 2), (2, 0))
        self.assertEqual(forward(2, 2, 2), (2, 2))
        self.assertEqual(solve(1, 2, 2, 2, 1).support, 0b11110000)

    def test_all_local_states_against_binary_truth_table(self):
        # Independent Boolean truth table: majority on three resolved bits.
        truth = (0, 0, 0, 1, 0, 1, 1, 1)
        for values in product(range(3), repeat=5):
            result = solve(*values)
            witnesses = []
            for index, d in enumerate(truth):
                base = ((index >> 2) & 1, (index >> 1) & 1, index & 1)
                if any(v != 2 and v != w for v, w in zip(values[:3], base)):
                    continue
                if values[3] != 2 and values[4] != 2 and d ^ values[4] != values[3]:
                    continue
                witnesses.append((index, base))
            self.assertEqual(result.support, sum(1 << i for i, _ in witnesses))
            self.assertEqual(result.status == CONFLICT, not witnesses)
            for old, new in zip(values, result.values):
                self.assertTrue(old == new or old == 2 and new in (0, 1))
            if result.status == CHANGED:
                for pos in range(3):
                    if values[pos] != result.values[pos]:
                        self.assertTrue(all(b[pos] == result.values[pos] for _, b in witnesses))
            if result.status == CLOSED:
                self.assertIn(majority(*values[:3]), (0, 1))
                self.assertEqual(majority(*values[:3]) ^ values[4], values[3])
            # Finite monotone local updates terminate, without oscillation.
            current = values
            for _ in range(6):
                step = solve(*current)
                if step.status != CHANGED:
                    break
                current = step.values
            else:
                self.fail("local propagation failed to terminate")

    def test_nonunique_constraint_survives_realization(self):
        pending = solve(0, 2, 2, 0, 0)
        self.assertEqual(solve(0, 1, 1, 0, 0, pending.support).status, CONFLICT)
        self.assertEqual(solve(0, 1, 0, 0, 0, pending.support).status, CLOSED)
        self.assertEqual(solve(0, 2, 2, 0, 0, 1 << 2).values, (0, 1, 0, 0, 0))

    def test_all_completion_masks(self):
        for mask in range(256):
            for r, e in product((0, 1), repeat=2):
                result = solve(2, 2, 2, r, e, mask)
                expected = [i for i in range(8) if mask & (1 << i)
                            and ((0,0,0,1,0,1,1,1)[i] ^ e) == r]
                self.assertEqual(result.support, sum(1 << i for i in expected))
                for pos, shift in enumerate((2, 1, 0)):
                    supported = {(i >> shift) & 1 for i in expected}
                    if len(supported) == 1:
                        self.assertEqual(result.values[pos], supported.pop())

    def test_strict_area_boundary_is_explicit(self):
        pending = solve(0, 1, 2, 0, 0)
        self.assertEqual(pending.values, (0, 1, 2, 0, 0))
        self.assertEqual(pending.status, WAIT)
        self.assertTrue(pending.needs_base_refinement)
        self.assertEqual(pending.support, 1 << 2)  # M=0 is recorded, not lost
        self.assertEqual(solve(0, 0, 2, 0, 0).status, CLOSED)

    def test_encoding_and_validation(self):
        for values in product(range(3), repeat=5):
            self.assertEqual(decode(encode(values)), values)
        for value in (-1, 3, True, None):
            with self.assertRaises(ValueError):
                solve(value, 0, 0, 0, 0)
        with self.assertRaises(ValueError):
            solve(0, 0, 0, 0, 0, 256)

    def test_shared_events_and_isolated_branches(self):
        net = Network()
        shared, out = Cell(), Cell()
        # Dependent is deliberately evaluated before its producer.
        net.add(shared, Cell(1), Cell(0), out, Cell(0))
        net.add(Cell(0), Cell(2), shared, Cell(1), Cell(0))
        first = net.execute_one_action()
        self.assertEqual(first[1].status, WAIT)
        fork = net.fork()
        events = []
        while (event := net.execute_one_action()) is not None:
            events.append(event)
        self.assertEqual((shared.value, out.value), (1, 1))
        self.assertFalse(net.failed)
        self.assertEqual(len(events), 4)
        self.assertEqual(fork.gates[0][0][0].value, 2)
        self.assertIs(fork.gates[0][0][0], fork.gates[1][0][2])
        self.assertIsNone(net.execute_one_action())

    def test_fractal_sharing(self):
        tensor = FractalTensor()
        s = tensor.seeds
        self.assertEqual(sum(len(t) for t in tensor.triplets), 39)
        for i in range(1, 4):
            self.assertIs(s[0][i], s[i][0])
        s[2][0][0].value = 1
        self.assertEqual(s[0][2][0].value, 1)


if __name__ == '__main__':
    unittest.main()
