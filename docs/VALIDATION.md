# Validación de la entrega

Ejecución: 11 de septiembre de 2026. Perfil
`corpus-trigate-v141-strict-areas-1`.

Placa de esta entrega: **Tang Nano 9K**, GW1NR-LV9QN88PC6/I5.
Pines de reloj y LED verificados con el ejemplo oficial de Sipeed.

## Resultados

| Comprobación | Cobertura | Resultado |
|---|---|---|
| Referencia Python | 9 pruebas, con enumeraciones internas | Superada |
| Estados semánticos | Los 243 paquetes A/B/M/R/E | Superada |
| RTL frente a referencia | 62.208 combinaciones de estado y máscara, más 1.024 codificaciones físicas | 63.232 coincidencias |
| Evaluador directo RTL | Las 256 codificaciones físicas A/B/M/orientación | Superada |
| Interfaz registrada | 1.024 solicitudes, huecos, presión de retorno, sustitución y reinicio | Superada |
| Autoprueba Tang Nano | Los 16 casos, con reloj acelerado en simulación | Superada |
| Síntesis del núcleo | Mapeo Gowin, familia GW1N, sin pads | Superada |
| C existente, sin cambios | 23.902 comprobaciones del repositorio base | Superada |
| Place & Route y tiempos de placa | No ejecutados | Pendiente |
| Prueba física | No ejecutada | Pendiente |
| Piloto FPGA de ventana A6/5B | 256 consultas, 13 posiciones, DS/DE/DO, estados y áreas | Superada: 256/256 coincidencias en Tang Nano 9K por COM5 |
| FractalTensor FPGA completo | 39 trits, ramas, Carry y Dictionary | Fuera del piloto |
| Aurora completo | Fuera del alcance de este módulo | No se afirma conformidad |

La referencia se comprobó con una tabla booleana independiente de ocho filas
y ejemplos del documento. Comparar RTL y referencia verifica la traducción
de este perfil; no elimina la tensión de activación descrita en PROFILE.md.
Los resultados no evalúan comprensión, aprendizaje general ni inteligencia.

## Piloto de ventana FPGA

El piloto desplegado en la Tang Nano 9K no es todavía el FractalTensor completo.
Implementa 13 posiciones empaquetadas, tres acciones fijas `DS -> DE -> DO`,
tres rondas de propagación y la interfaz UART `A6/5B`. El orquestador C conserva
las 39 posiciones, las ramas, `Carry`, la poda y el Dictionary.

La prueba reproducible se ejecuta con:

```powershell
python host/compare_window_semantics.py --port COM5
```

Compara los 13 trits resultantes, los estados y las áreas de las tres acciones
contra el modelo Python del piloto. La prueba de 256 consultas obtuvo 256/256
coincidencias en la Tang Nano 9K conectada por `COM5`, incluyendo trits,
estados, áreas y `needs`. No implica conformidad del FractalTensor de 39 trits
ni del Aurora completo.

## Recursos mapeados

El núcleo registrado se sintetizó en 143 primitivas LUT1–LUT4, 25 registros,
4 primitivas ALU y 63 multiplexores de expansión LUT, además de constantes.
El desglose exacto figura en `verification.json`. Son recursos del núcleo,
no de todo el proyecto de autoprueba ni del modelo Aurora completo. No se
deduce de ellos una frecuencia máxima, consumo energético o capacidad final
de la Tang Nano: eso exige implementación física y medidas.

## Reproducción

Desde la carpeta que contiene verify.py:

```sh
python verify.py --rtl --synth
```

Se utilizó la distribución Python de Verilator (el ejecutable informa 5.49)
y Yosys a través de YoWASP. Los registros adjuntos recogen la ejecución.
`verification.json` identifica la especificación, el commit C inspeccionado,
los estados observados y las huellas de las fuentes del módulo.

Para repetir las comprobaciones C desde Aurora-Trinity en un entorno con GCC:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude src/*.c tests/test_aurora.c -o aurora_tests
./aurora_tests
```

El paquete entregado contiene únicamente el nuevo módulo CORPUS y su
documentación; la biblioteca C anterior permanece en Aurora-Trinity.
