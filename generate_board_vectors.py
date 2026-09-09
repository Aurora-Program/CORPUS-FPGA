"""Regenerate the 16 illustrative board cases from the versioned reference."""
from pathlib import Path
from reference.trigate import solve, encode
from verify import payload

CASES = [
    (0,0,1,2,0), (1,1,0,0,2), (0,2,2,1,0), (0,2,2,0,0),
    (1,2,2,2,1), (1,1,0,0,0), (1,1,0,0,1), (0,0,2,0,0),
    (0,1,2,0,0), (2,2,2,2,2), (1,1,2,2,1), (0,0,2,2,1),
    (1,2,2,0,0), (2,0,2,1,0), (2,2,0,1,0), (2,2,2,1,0),
]


def generate():
    lines = ['// BEGIN GENERATED CASES']
    for name, width, values in (
        ('test_input', 10, [encode(v) for v in CASES]),
        ('test_expected', 24, [payload(solve(*v)) for v in CASES]),
    ):
        lines += [f'function [{width-1}:0] {name};', '    input [3:0] index;',
                  '    begin', '        case (index)']
        lines += [f"            4'd{i}: {name} = {width}'h{v:x};"
                  for i,v in enumerate(values)]
        lines += ['        endcase', '    end', 'endfunction']
    lines += ['// END GENERATED CASES']
    return '\n'.join(lines)


if __name__ == '__main__':
    path = Path(__file__).resolve().parent / 'boards/tang_nano_9k/corpus_selftest_top.v'
    text = path.read_text()
    start = text.index('// BEGIN GENERATED CASES')
    end = text.index('// END GENERATED CASES') + len('// END GENERATED CASES')
    path.write_text(text[:start] + generate() + text[end:])
