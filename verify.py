"""Run from this directory: python verify.py [--rtl] [--synth]."""
import argparse
from itertools import product
import json
from pathlib import Path
import shutil
import subprocess
import sys
from reference.trigate import solve, forward, encode, decode, STATUS_NAMES

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / 'build'


def run(command):
    result = subprocess.run([str(x) for x in command], cwd=ROOT,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if result.returncode:
        print(result.stdout)
        raise RuntimeError(f"command failed: {command[0]}")
    return result.stdout


def payload(result):
    return ((encode(result.values) << 14) | (result.status << 12)
            | (result.areas << 10) | (result.support << 2)
            | (int(result.needs_base_refinement) << 1) | int(result.direct))


def tool(name):
    found = shutil.which(name)
    if not found:
        candidate = Path.home() / '.local/bin' / name
        if candidate.exists():
            found = str(candidate)
    return found


def vectors():
    rows = []
    states = {name: 0 for name in STATUS_NAMES}
    for word in range(1024):  # Every physical encoding, including mixed 10.
        result = solve(*decode(word))
        rows.append(f'{word:03x} ff {payload(result):06x}\n')
    for values in product(range(3), repeat=5):
        states[STATUS_NAMES[solve(*values).status]] += 1
        for mask in range(256):  # Every externally retained joint restriction.
            result = solve(*values, allowed=mask)
            rows.append(f'{encode(values):03x} {mask:02x} {payload(result):06x}\n')
    (BUILD / 'step.hex').write_text(''.join(rows))
    count = len(rows)
    rows = []
    for word in range(256):
        a,b,m,e = decode(word)[1:]
        r, state = forward(a,b,m,e)
        rows.append(f'{word:02x} {encode((r,state)):01x}\n')
    (BUILD / 'forward.hex').write_text(''.join(rows))
    return count, states


def simulate(name, sources, vector_file):
    verilator = tool('verilator') or tool('verilator-cli')
    iverilog = tool('iverilog')
    if iverilog and tool('vvp'):
        output = BUILD / name
        compile_log = run([iverilog, '-g2012', '-Iboards/tang_nano_9k', '-s', name, '-o', output, *sources])
        result = run([tool('vvp'), output, f'+vectors={vector_file}'])
    elif verilator:
        obj = BUILD / name
        compile_log = run([verilator, '--binary', '--timing', '-j', '2',
                           '--top-module', name, '--Mdir', obj,
                           '-Wno-TIMESCALEMOD', '-Iboards/tang_nano_9k', *sources])
        result = run([obj / f'V{name}', f'+vectors={vector_file}'])
    else:
        raise RuntimeError('Install Icarus Verilog or Verilator to run RTL simulation')
    (BUILD / f'{name}.compile.log').write_text(compile_log)
    (BUILD / f'{name}.log').write_text(result)
    print(result.strip())
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rtl', action='store_true')
    parser.add_argument('--synth', action='store_true')
    args = parser.parse_args()
    BUILD.mkdir(exist_ok=True)
    (BUILD / 'verification.json').unlink(missing_ok=True)
    log = run([sys.executable, '-m', 'unittest', 'discover', '-s', 'tests', '-v'])
    (BUILD / 'reference.log').write_text(log)
    print(log.strip())
    count, states = vectors()
    report = {'profile': 'corpus-trigate-v141-strict-areas-1',
              'board': 'Tang Nano 9K', 'device': 'GW1NR-LV9QN88PC6/I5',
              'synthesis_family': 'gw1n',
              'reference_tests': 'passed', 'semantic_states': 243,
              'state_counts': states, 'relational_vectors': count,
              'forward_vectors': 256, 'rtl': 'not_run', 'synthesis': 'not_run',
              'board_test': 'not_run', 'full_aurora_conformance': 'not_claimed'}
    if args.rtl:
        simulate('tb_step', ['rtl/trigate_step.v', 'tests/tb_step.sv'], BUILD / 'step.hex')
        simulate('tb_forward', ['rtl/trigate_forward.v', 'tests/tb_forward.sv'], BUILD / 'forward.hex')
        simulate('tb_core', ['rtl/trigate_step.v', 'rtl/trigate_core.v', 'tests/tb_core.sv'], BUILD / 'step.hex')
        simulate('tb_board', ['rtl/trigate_step.v', 'rtl/trigate_core.v',
                             'boards/tang_nano_9k/corpus_selftest_top.v',
                             'tests/tb_board.sv'], BUILD / 'step.hex')
        report['rtl'] = 'passed'
        report['board_simulation'] = '16_cases_passed'
    if args.synth:
        yosys = tool('yosys') or tool('yowasp-yosys')
        if not yosys:
            raise RuntimeError('Install Yosys to synthesize the core')
        script = ('read_verilog rtl/trigate_step.v rtl/trigate_core.v; '
                  'synth_gowin -family gw1n -top trigate_core -noiopads -json build/trigate_gowin.json; '
                  'check -assert; stat')
        synth_log = run([yosys, '-p', script])
        (BUILD / 'synthesis.log').write_text(synth_log)
        report['synthesis'] = 'gowin_mapping_passed_no_place_and_route'
        netlist = json.loads((BUILD / 'trigate_gowin.json').read_text())
        counts = {}
        for cell in netlist['modules']['trigate_core']['cells'].values():
            counts[cell['type']] = counts.get(cell['type'], 0) + 1
        report['gowin_cell_counts'] = counts
        print('Gowin mapped cells:', counts)
    (BUILD / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
