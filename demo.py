"""Readable local examples, including uncertainty that must remain open."""
from reference.trigate import solve, forward, STATUS_NAMES

examples = (
    ('Calcular R', (0,0,1,2,0)),
    ('Calcular E', (1,1,0,0,2)),
    ('Resolver base', (0,2,2,1,0)),
    ('Conservar alternativas', (0,2,2,0,0)),
    ('Esperar contexto', (1,2,2,2,1)),
    ('Rechazar contradiccion', (1,1,0,0,0)),
    ('Detectar frontera de la especificacion', (0,1,2,0,0)),
)
for label, values in examples:
    result = solve(*values)
    print(f'{label}: {values} -> {result.values}; '
          f'{STATUS_NAMES[result.status]}; alternativas={result.support:08b}')
print('Observacion residual (1,2,2): R,E =', forward(1,2,2))
