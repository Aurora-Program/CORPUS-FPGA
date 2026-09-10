"""Compare CORPUS FPGA responses against the Python reference over UART."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from itertools import product
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from reference.trigate import decode, encode, solve
from verify import payload

CLIENT = ROOT / "host" / "corpus_uart_batch.exe"
VALUE_RE = re.compile(r"values=0x([0-9a-f]+)")
STATUS_RE = re.compile(r"status=(\d+) areas=(\d+)")
SUPPORT_RE = re.compile(r"support=0x([0-9a-f]+)")
FLAGS_RE = re.compile(r"needs_base_refinement=(\d+) direct=(\d+)")


def read_payload(output: str) -> int:
    value = int(VALUE_RE.search(output).group(1), 16)
    status_match = STATUS_RE.search(output)
    support = int(SUPPORT_RE.search(output).group(1), 16)
    flags = FLAGS_RE.search(output)
    return (
        (encode(decode(value)) << 14)
        | (int(status_match.group(1)) << 12)
        | (int(status_match.group(2)) << 10)
        | (support << 2)
        | (int(flags.group(1)) << 1)
        | int(flags.group(2))
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM5")
    parser.add_argument("--limit", type=int, default=0)
    args = parser.parse_args()
    masks = (0x00, 0x55, 0xAA, 0xFF)
    cases = []
    for values in product(range(3), repeat=5):
        for allowed in masks:
            word = encode(values)
            expected = payload(solve(*values, allowed=allowed))
            cases.append((word, allowed, expected))
            if args.limit and len(cases) >= args.limit:
                break
        if args.limit and len(cases) >= args.limit:
            break

    request_text = "".join(f"0x{word:03x} 0x{allowed:02x}\n"
                            for word, allowed, _ in cases)
    result = subprocess.run(
        [str(CLIENT), args.port], cwd=ROOT, input=request_text,
        capture_output=True, text=True, check=False,
    )
    responses = result.stdout.splitlines()
    if result.returncode:
        print(result.stderr)
        return 1
    if len(responses) != len(cases):
        print(f"FAIL response_count={len(responses)}/{len(cases)}")
        print(result.stderr)
        return 1

    for total, ((word, allowed, expected), response) in enumerate(zip(cases, responses)):
        if response == "ERROR":
            print(f"FAIL case={total} input=0x{word:03x} allowed=0x{allowed:02x}")
            return 1
        actual = int(response, 16)
        if actual != expected:
            print(
                f"FAIL case={total} input=0x{word:03x} "
                f"allowed=0x{allowed:02x} expected=0x{expected:06x} "
                f"actual=0x{actual:06x}"
            )
            return 1
    print(f"physical_conformance={len(cases)}/{len(cases)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
