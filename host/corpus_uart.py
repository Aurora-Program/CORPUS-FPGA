"""PC client for the CORPUS Tang Nano 9K UART protocol."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass

try:
    import serial
    from serial.tools import list_ports
except ImportError as exc:
    raise SystemExit("Instala pyserial con: python -m pip install pyserial") from exc


@dataclass(frozen=True)
class Response:
    values: int
    status: int
    areas: int
    support: int
    needs_base_refinement: bool
    direct: bool


def encode_request(values: int, allowed: int) -> bytes:
    if not 0 <= values <= 0x3FF:
        raise ValueError("values debe estar entre 0x000 y 0x3FF")
    if not 0 <= allowed <= 0xFF:
        raise ValueError("allowed debe estar entre 0x00 y 0xFF")
    return bytes((0xA5, values & 0xFF, (values >> 8) & 0x03, allowed))


def decode_response(frame: bytes) -> Response:
    if len(frame) != 4 or frame[0] != 0x5A:
        raise ValueError(f"respuesta invalida: {frame.hex(' ')}")
    payload = int.from_bytes(frame[1:], "big")
    return Response(
        values=(payload >> 14) & 0x3FF,
        status=(payload >> 12) & 0x03,
        areas=(payload >> 10) & 0x03,
        support=(payload >> 2) & 0xFF,
        needs_base_refinement=bool((payload >> 1) & 1),
        direct=bool(payload & 1),
    )


def query(port: str, values: int, allowed: int, timeout: float) -> Response:
    with serial.Serial(port, 115200, timeout=timeout) as device:
        device.reset_input_buffer()
        device.write(encode_request(values, allowed))
        device.flush()
        frame = device.read(4)
        if len(frame) != 4:
            raise TimeoutError(f"respuesta incompleta ({len(frame)}/4 bytes)")
        return decode_response(frame)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="puerto serie, por ejemplo COM7")
    parser.add_argument("--values", type=lambda value: int(value, 0), default=0x34)
    parser.add_argument("--allowed", type=lambda value: int(value, 0), default=0xFF)
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument("--list", action="store_true", help="lista puertos serie disponibles")
    args = parser.parse_args()

    if args.list:
        for port in list_ports.comports():
            print(f"{port.device}: {port.description}")
        return 0
    if not args.port:
        parser.error("--port es obligatorio salvo cuando se usa --list")

    try:
        result = query(args.port, args.values, args.allowed, args.timeout)
    except (OSError, TimeoutError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    print(f"values=0x{result.values:03x}")
    print(f"status={result.status} areas={result.areas}")
    print(f"support=0x{result.support:02x}")
    print(f"needs_base_refinement={int(result.needs_base_refinement)} direct={int(result.direct)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
