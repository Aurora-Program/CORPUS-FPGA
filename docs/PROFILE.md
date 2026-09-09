# Perfil local de ejecución

Identificador: `corpus-trigate-v141-strict-areas-1`.

## Alcance

Este perfil implementa un TriGate escalar y su interfaz síncrona. La referencia
incluye un pequeño motor de eventos y las vistas compartidas 1-3-9 para probar
identidad y propagación. No implementa la composición tensorial completa,
el restablecimiento de DO, las invariantes I/K/O, el diccionario, su promoción,
la tokenización, el Carry sucesivo, la red distribuida ni el aprendizaje lingüístico.
Un estado CLOSED aquí es cierre local, nunca autorización de Output externo.

## Representación física

| Trit | Bits de entrada | Bits de salida |
|---|---|---|
| 0 | 00 | 00 |
| 1 | 11 | 11 |
| 2 abierto | 01 o 10 | 01 |

El paquete de 10 bits es `{A,B,M,R,E}`: A ocupa 9:8 y E ocupa 1:0.
Los dos códigos mixtos tienen la misma semántica. Los estados X y Z del
simulador no son valores Aurora admitidos por la interfaz.

## Dos operaciones diferentes

`trigate_forward` es un observador combinacional: produce la mayoría orientada
o, si no hay mayoría estable, R=2 y el residuo único. No ejecuta el planificador
de áreas ni escribe de vuelta en una relación activa. Si la base tiene mayoría
pero la orientación es 2, devuelve R=E=2.

`trigate_step` ejecuta una acción sobre la relación activa. Cuenta la ambigüedad
de ABM (al menos dos trits abiertos), de R y de E. Con dos o tres áreas ambiguas
espera; con una intenta resolver esa área; con cero comprueba la relación.
La contradicción de las restricciones se detecta también cuando la activación
normal estaría en espera. No sobrescribe valores binarios dados.

Con R concreto, E restringe la orientación de la mayoría. Con R abierto, E no
se utiliza para filtrar las posibles bases como si fuera una orientación ya
observada. Cuando solo R está abierto y existe mayoría estable, el E suministrado
determina el cálculo, conforme a la dirección explícita de 3.2.

El observador de residuo y el solucionador no están conectados automáticamente:
el integrador debe mantener la procedencia de las observaciones y de los
requisitos fijos de cierre. Un E residual no autoriza por sí mismo un Output.

## Alternativas conjuntas

`allowed_i[7:0]` representa restricciones externas de la rama y se inicia en FF.
El bit `4*A+2*B+M` representa una asignación binaria admisible de la base.
`support_o` devuelve su intersección con la relación actual. El cero indica
contradicción; no se reemplaza por un estado abierto.

Las máscaras enumeran refinamientos binarios de posiciones abiertas; no
convierten el símbolo 2 en un dato binario escogido. Los trits solo cambian
cuando todos los candidatos conservados coinciden. El contexto debe retener
la relación y su máscara, y verificar conjuntamente las realizaciones futuras.

Ejemplo: `(0,2,2; R=0,E=0)` conserva `(B,M) = 00,01,10`, máscara 07.
Cada trit individual continúa abierto, pero la realización conjunta 11 se
rechaza. Comprobar únicamente dos coincidencias de comodín perdería esta regla.

## Estados de una acción

| Código | Nombre | Significado |
|---|---|---|
| 0 | WAIT | No hay escritura forzada en el área activada |
| 1 | CHANGED | Se han resuelto uno o más trits; reactivar dependientes |
| 2 | CLOSED | Relación local estable, válida y con R y E concretos |
| 3 | CONFLICT | No quedan refinamientos compatibles |

`areas_o` cuenta las áreas antes de la acción. Una respuesta CHANGED exige
otra evaluación si se quiere obtener su estado estable. `direct_o` solo vale
uno cuando el estado es CLOSED y E=0. Complementariedad válida y contradicción
son estados diferentes.

## Tensión explícita de la especificación

El capítulo 3 declara ABM ambiguo solo con al menos dos valores 2; con cero
áreas ambiguas describe una relación ya cristalizada que no emite otro evento.
El apéndice, sin embargo, permite resolver M en `(0,1,2; R=0,E=0)`.
En ese ejemplo, n=0, la mayoría presente es 2 y el único refinamiento admisible
es M=0. No se puede ejecutar esa escritura y aplicar literalmente el criterio
de activación descrito a la vez.

Este perfil prioriza la activación estricta: devuelve WAIT,
`needs_base_refinement_o=1` y la máscara que conserva M=0. No marca la relación
como contradictoria ni cristalizada. El punto requiere decidir si la validación
de n=0 debe activar también los huecos internos de la base. No se presume
conformidad completa con ambas formulaciones mientras esa decisión esté pendiente.

## Interfaz de reloj

Reloj único, reset síncrono activo a uno. Una solicitud se acepta en el flanco
cuando `request_valid && request_ready`. La respuesta queda registrada tras
ese flanco; puede consumirse en el siguiente. Hay capacidad para una respuesta
pendiente. Mientras `response_valid && !response_ready`, el contenido y la
señal de validez se mantienen. Es posible consumir y reemplazar una respuesta
en el mismo flanco. Reset vacía la respuesta y desactiva request_ready.

El emisor mantiene solicitud y restricciones estables hasta su aceptación.
Este módulo no implementa cruces de dominios de reloj, UART ni señales GPIO
asíncronas. La autoprueba de placa opera enteramente con su reloj local.

## Eventos y estructura de referencia

`Network.execute_one_action()` evalúa como máximo un TriGate. La cola FIFO
deduplica eventos y despierta dependientes por identidad de celda. Una rama
se copia con `fork()` conservando las identidades compartidas dentro de ella,
sin compartir valores mutables con la original. Un conflicto detiene esa rama.
La cola vacía significa punto fijo local; puede contener relaciones WAIT.

Se soportan alias de posiciones de base dentro de un TriGate; alias entre
base, R y E dentro de una misma puerta se rechazan explícitamente en este
arnés. Compartir celdas entre puertas distintas sí está implementado.

Las 13 tripletas del tensor constituyen 39 celdas. Las cuatro semillas son
vistas `(t1;t2,t3,t4)`, `(t2;t5,t6,t7)`, `(t3;t8,t9,t10)` y
`(t4;t11,t12,t13)`. Esta prueba de estructura no afirma que sus operaciones
tensoriales estén implementadas.
