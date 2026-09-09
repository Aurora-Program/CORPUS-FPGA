"""Small event-driven harness for the local FPGA primitive, not full Aurora."""
from collections import deque
from copy import deepcopy
from dataclasses import dataclass
from .trigate import solve, CHANGED, CONFLICT


@dataclass(eq=False)
class Cell:
    value: int = 2

    def __post_init__(self):
        if type(self.value) is not int or self.value not in (0, 1, 2):
            raise ValueError("invalid trit")


class Network:
    def __init__(self):
        self.gates = []
        self.watchers = {}
        self.queue = deque()
        self.queued = set()
        self.failed = False

    def _enqueue(self, index):
        if index not in self.queued:
            self.queue.append(index)
            self.queued.add(index)

    def add(self, *cells, allowed=255):
        if len(cells) != 5 or any(not isinstance(c, Cell) for c in cells):
            raise ValueError("five shared Cell objects required")
        if type(allowed) is not int or not 0 <= allowed <= 255:
            raise ValueError("invalid completion mask")
        index = len(self.gates)
        self.gates.append((cells, allowed))
        for cell in cells:
            self.watchers.setdefault(cell, set()).add(index)
        self._enqueue(index)
        return index

    def refine(self, cell, value):
        if type(value) is not int or value not in (0, 1):
            raise ValueError("external refinement must be binary")
        if cell.value != 2 and cell.value != value:
            raise ValueError("refinement contradicts the existing value")
        if cell.value == value:
            return
        cell.value = value
        for index in sorted(self.watchers.get(cell, ())):
            self._enqueue(index)

    def execute_one_action(self):
        """One gate evaluation per action; no recursive subtree hidden here."""
        if self.failed or not self.queue:
            return None
        index = self.queue.popleft()
        self.queued.remove(index)
        cells, allowed = self.gates[index]
        # Enforce aliases within a gate, too: copies could admit false witnesses.
        for assignment in range(8):
            bits = ((assignment >> 2) & 1, (assignment >> 1) & 1, assignment & 1)
            for i in range(3):
                for j in range(i):
                    if cells[i] is cells[j] and bits[i] != bits[j]:
                        allowed &= ~(1 << assignment)
        # Cross-role aliases need a general relational solver; reject explicitly.
        # Ordinary inter-gate sharing is fully supported by the watcher graph.
        if any(cells[i] is cells[j] for i in (3, 4) for j in range(i)):
            raise ValueError("cross-role alias within one gate is unsupported")
        result = solve(*(cell.value for cell in cells), allowed=allowed)
        if result.status == CONFLICT:
            self.failed = True
        elif result.status == CHANGED:
            for cell, value in zip(cells, result.values):
                if value != cell.value:
                    self.refine(cell, value)
        return index, result

    def fork(self):
        """Copy a speculative branch while retaining its internal shared identity."""
        return deepcopy(self)


class FractalTensor:
    """39 active trits; the four seed views reference the same overlapping cells."""
    def __init__(self):
        self.triplets = tuple(tuple(Cell() for _ in range(3)) for _ in range(13))

    @property
    def seeds(self):
        t = self.triplets
        return ((t[0], t[1], t[2], t[3]),
                (t[1], t[4], t[5], t[6]),
                (t[2], t[7], t[8], t[9]),
                (t[3], t[10], t[11], t[12]))
