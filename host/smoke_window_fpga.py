"""Protocol smoke test for the fixed FPGA window pilot."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "host" / "corpus_window_batch.exe"


def valid_packed_trits(byte: int) -> bool:
    return all(((byte >> shift) & 3) in (0, 1, 3) for shift in (0, 2, 4, 6))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM5")
    args = parser.parse_args()
    cases = []
    for index in range(16):
        # Vary R/E and the three completion masks while keeping legal 2-bit trits.
        cases.append((0x3F, 0x50, 0x15 if index & 1 else 0x55,
                      (index << 2) & 0xFF, 0xFF, (index * 0x11) & 0xFF,
                      0xFF if index & 2 else 0x55))
    input_text = "".join(" ".join(f"0x{value:02x}" for value in case) + "\n"
                          for case in cases)
    result = subprocess.run([str(CLIENT), args.port], cwd=ROOT, input=input_text,
                            capture_output=True, text=True, check=False)
    responses = result.stdout.splitlines()
    failures = []
    if result.returncode:
        failures.append(f"client_exit={result.returncode}")
    if len(responses) != len(cases):
        failures.append(f"response_count={len(responses)}/{len(cases)}")
    for index, line in enumerate(responses):
        fields = line.split()
        if len(fields) != 8:
            failures.append(f"case={index} malformed={line}")
            continue
        try:
            values = [int(field, 16) for field in fields]
        except ValueError:
            failures.append(f"case={index} non_hex={line}")
            continue
        if values[0] != 0x5B:
            failures.append(f"case={index} header=0x{values[0]:02x}")
        if not all(valid_packed_trits(byte) for byte in values[1:5]):
            failures.append(f"case={index} invalid_trit_encoding={line}")
        status_byte, area_byte, needs_byte = values[5:8]
        if any(((status_byte >> shift) & 3) > 3 for shift in (0, 2, 4)):
            failures.append(f"case={index} invalid_status={line}")
        if any(((area_byte >> shift) & 3) > 3 for shift in (0, 2, 4)):
            failures.append(f"case={index} invalid_areas={line}")
        if needs_byte & 0xF8:
            failures.append(f"case={index} invalid_needs={line}")
    if failures:
        print("FAIL window FPGA smoke")
        print("\n".join(failures))
        if result.stderr:
            print(result.stderr)
        return 1
    print(f"window_fpga_smoke={len(cases)}/{len(cases)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
