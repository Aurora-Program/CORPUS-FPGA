"""Run fixed 1-3-9 window checks against the FPGA over COM5."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "host" / "corpus_window_batch.exe"

CASES = (
    ("0x3f 0x50 0x55 0x01 0xff 0xff 0xff", "5b 3f 50 55 01 00 3a 00"),
    ("0x3f 0x50 0x15 0x01 0xff 0xff 0xff", "5b 3f 50 15 03 0e 10 00"),
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM5")
    args = parser.parse_args()
    input_text = "".join(request + "\n" for request, _ in CASES)
    result = subprocess.run(
        [str(CLIENT), args.port], cwd=ROOT, input=input_text,
        capture_output=True, text=True, check=False,
    )
    actual = [line.strip().lower() for line in result.stdout.splitlines()]
    expected = [response for _, response in CASES]
    if result.returncode or actual != expected:
        print("FAIL window FPGA")
        print(f"expected={expected}")
        print(f"actual={actual}")
        if result.stderr:
            print(result.stderr)
        return 1
    print(f"window_fpga_conformance={len(expected)}/{len(expected)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
