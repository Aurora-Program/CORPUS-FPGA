"""Compare the fixed FPGA window pilot against its exact Python model."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "host" / "corpus_window_batch.exe"


def decode_byte(value: int) -> list[int]:
    physical = [(value >> shift) & 3 for shift in (0, 2, 4, 6)]
    return [0 if trit == 0 else 1 if trit == 3 else 2 for trit in physical]


def encode_byte(values: list[int]) -> int:
    result = 0
    for index, value in enumerate(values):
        result |= value << (index * 2)
    return result


def step(values: list[int], allowed: int) -> tuple[list[int], int, int, int]:
    candidates = []
    for index in range(8):
        base = [(index >> 2) & 1, (index >> 1) & 1, index & 1]
        if not all(value == 2 or value == actual for value, actual in zip(values[:3], base)):
            continue
        if not (allowed & (1 << index)):
            continue
        majority = 1 if sum(base) >= 2 else 0
        if values[3] != 2 and values[4] != 2 and (majority ^ values[4]) != values[3]:
            continue
        candidates.append(base)
    support = sum(1 << index for index in range(8) if index < len(candidates))
    # The pilot receives all masks as FF in the semantic test. Recompute support
    # using the same candidate indices used by the RTL truth table.
    support = 0
    for index in range(8):
        base = [(index >> 2) & 1, (index >> 1) & 1, index & 1]
        majority = 1 if sum(base) >= 2 else 0
        if (all(value == 2 or value == actual for value, actual in zip(values[:3], base))
            and allowed & (1 << index)
            and (values[3] == 2 or values[4] == 2 or
                 (majority ^ values[4]) == values[3])):
            support |= 1 << index
    if not support:
        return values[:], 3, 0, 0
    open_count = sum(value == 2 for value in values[:3])
    areas = int(open_count >= 2) + int(values[3] == 2) + int(values[4] == 2)
    majority = 2
    if values[0] == values[1] and values[0] != 2:
        majority = values[0]
    elif values[0] == values[2] and values[0] != 2:
        majority = values[0]
    elif values[1] == values[2] and values[1] != 2:
        majority = values[1]
    result = values[:]
    status = 0
    needs = 0
    if areas == 0:
        if majority == 2:
            needs = 1
        else:
            status = 2
    elif areas == 1:
        if open_count >= 2:
            for position in range(3):
                choices = {(index >> (2 - position)) & 1 for index in range(8) if support & (1 << index)}
                if values[position] == 2 and len(choices) == 1:
                    result[position] = choices.pop()
        elif values[3] == 2 and majority != 2:
            result[3] = majority ^ values[4]
        elif values[4] == 2 and majority != 2:
            result[4] = majority ^ values[3]
        if result != values:
            status = 1
    return result, status, areas, needs


def model(request: list[int]) -> list[int]:
    cells = []
    for byte in request[:4]:
        cells.extend(decode_byte(byte))
    cells = cells[:13]
    masks = request[4:7]
    ds = [cells[0], cells[1], cells[2], cells[12], cells[11]]
    result, _, _, _ = step(ds, masks[0])
    cells[12], cells[11] = result[3], result[4]
    de = [cells[3], cells[4], cells[5], cells[12], cells[11]]
    result, _, _, _ = step(de, masks[1])
    cells[12], cells[11] = result[3], result[4]
    do = [cells[6], cells[7], cells[8], cells[12], cells[11]]
    result, _, _, _ = step(do, masks[2])
    cells[6], cells[7], cells[8], cells[12], cells[11] = result
    return cells


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM5")
    args = parser.parse_args()
    cases = []
    for index in range(64):
        cases.append([
            (0x3F ^ ((index * 0x15) & 0xFF)),
            (0x50 ^ ((index * 0x29) & 0xFF)),
            (0x15 if index & 1 else 0x55) ^ ((index * 0x09) & 0xFF),
            (index << 2) & 0xFF,
            0xFF if index & 4 else 0x55,
            (index * 0x11) & 0xFF,
            0xFF if index & 2 else 0x55,
        ])
    input_text = "".join(" ".join(f"0x{value:02x}" for value in case) + "\n" for case in cases)
    result = subprocess.run([str(CLIENT), args.port], cwd=ROOT, input=input_text,
                            capture_output=True, text=True, check=False)
    responses = result.stdout.splitlines()
    failures = []
    for index, (case, response) in enumerate(zip(cases, responses)):
        fields = [int(field, 16) for field in response.split()]
        actual = []
        for byte in fields[1:5]:
            actual.extend(decode_byte(byte))
        actual = actual[:13]
        expected = model(case)
        if actual != expected:
            failures.append(f"case={index} expected={expected} actual={actual}")
    if result.returncode or len(responses) != len(cases) or failures:
        print("FAIL window semantic comparison")
        print("\n".join(failures) or f"responses={len(responses)}/{len(cases)}")
        return 1
    print(f"window_semantic_conformance={len(cases)}/{len(cases)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
