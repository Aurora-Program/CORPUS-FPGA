"""Compare the C TriGate solver against the Python reference."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "host" / "corpus_fractal_vectors.exe"

sys.path.insert(0, str(ROOT))
from reference.trigate import decode, encode, solve  # noqa: E402
from verify import payload  # noqa: E402


def main() -> int:
    process = subprocess.Popen([str(CLIENT)], cwd=ROOT, stdout=subprocess.PIPE, text=True)
    expected_count = 0
    for line in process.stdout:
        word_text, mask_text, actual_text = line.split()
        word = int(word_text, 16)
        mask = int(mask_text, 16)
        actual = int(actual_text, 16)
        expected = payload(solve(*decode(word), allowed=mask))
        expected_count += 1
        if actual != expected:
            print(f"FAIL case={expected_count} word=0x{word:03x} mask=0x{mask:02x}")
            print(f"expected=0x{expected:06x} actual=0x{actual:06x}")
            process.kill()
            return 1
    exit_code = process.wait()
    if exit_code:
        print(f"C solver exited with {exit_code}")
        return 1
    print(f"c_reference_conformance={expected_count}/{expected_count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
