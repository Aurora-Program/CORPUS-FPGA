"""CORPUS local TriGate profile for AURORA 1.4.1; no third-party packages.

An open trit is 2. Completion masks keep correlations between A, B and M.
They enumerate binary refinements, not three-valued data-table rows.
"""
from dataclasses import dataclass
from itertools import product

WAIT, CHANGED, CLOSED, CONFLICT = range(4)
STATUS_NAMES = ("wait", "changed", "closed", "conflict")


def majority(a, b, m):
    base = (a, b, m)
    return next((v for v in (0, 1) if base.count(v) >= 2), 2)


def forward(a, b, m, orientation=0):
    """Counting appendix: observation, independent of area activation."""
    if any(v not in (0, 1, 2) for v in (a, b, m, orientation)):
        raise ValueError("trits must be 0, 1 or 2")
    d = majority(a, b, m)
    if d != 2:
        return (d ^ orientation, orientation) if orientation != 2 else (2, 2)
    residue = {v for v in (a, b, m) if v != 2}
    return 2, next(iter(residue)) if len(residue) == 1 else 2


@dataclass(frozen=True)
class Resolution:
    values: tuple
    status: int
    areas: int
    support: int
    needs_base_refinement: bool = False

    @property
    def direct(self):
        return self.status == CLOSED and self.values[4] == 0


def solve(a, b, m, r, e, allowed=255):
    """Perform one bounded evaluation under the strict three-area rule.

    E constrains orientation only when R is concrete. At n=0 with an
    unresolved majority, preserve the latent base constraint and signal
    needs_base_refinement; see docs/PROFILE.md for the book's rule tension.
    """
    original = (a, b, m, r, e)
    if any(type(v) is not int or v not in (0, 1, 2) for v in original):
        raise ValueError("trits must be integers 0, 1 or 2")
    if type(allowed) is not int or not 0 <= allowed <= 255:
        raise ValueError("allowed must be an eight-bit completion mask")
    base_open = original[:3].count(2) >= 2
    n = int(base_open) + int(r == 2) + int(e == 2)
    candidates = []
    for av, bv, mv in product((0, 1), repeat=3):
        index = av * 4 + bv * 2 + mv
        if not (allowed & (1 << index)):
            continue
        if any(given != 2 and given != actual
               for given, actual in zip(original[:3], (av, bv, mv))):
            continue
        if r != 2 and e != 2 and majority(av, bv, mv) ^ e != r:
            continue
        candidates.append((av, bv, mv))
    support = sum(1 << (x * 4 + y * 2 + z) for x, y, z in candidates)
    if not candidates:
        return Resolution(original, CONFLICT, n, 0)
    if n >= 2:
        return Resolution(original, WAIT, n, support)
    d = majority(a, b, m)
    updated = list(original)
    if n == 0:
        if d == 2:
            return Resolution(original, WAIT, n, support, True)
        return Resolution(original, CLOSED, n, support)
    if base_open:
        for pos in range(3):
            choices = {candidate[pos] for candidate in candidates}
            if updated[pos] == 2 and len(choices) == 1:
                updated[pos] = next(iter(choices))
    elif r == 2 and d != 2:
        updated[3] = d ^ e
    elif e == 2 and d != 2:
        updated[4] = d ^ r
    values = tuple(updated)
    return Resolution(values, CHANGED if values != original else WAIT, n, support)


def encode(values):
    """A in bits 9:8, B 7:6, M 5:4, R 3:2, E 1:0."""
    word = 0
    for value in values:
        word = (word << 2) | (0, 3, 1)[value]
    return word


def decode(word):
    return tuple(0 if p == 0 else 1 if p == 3 else 2
                 for p in ((word >> shift) & 3 for shift in (8, 6, 4, 2, 0)))
