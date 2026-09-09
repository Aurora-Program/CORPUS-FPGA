# CORPUS TriGate para FPGA

Entrega corregida para la **Tang Nano 9K**. Sustituye la configuración de placa
de la primera entrega. Implementación de hardware del núcleo local de Aurora, basada en
**AURORA 1.4.1 revisado**. Incluye Verilog sintetizable, referencia Python,
pruebas exhaustivas y proyecto de autoprueba para Tang Nano 9K.

El módulo es independiente de la biblioteca C anterior de Aurora-Trinity.
Todavía no constituye el modelo Aurora completo: el diccionario, la extensión,
los Carry y el Transcender no están integrados en este hardware.

## Ejecutar en Windows 11

Descomprime el paquete, abre PowerShell en esta carpeta y ejecuta:

```powershell
py verify.py
py demo.py
```

La referencia utiliza únicamente Python 3.10 o posterior y su biblioteca
estándar. No necesita API, cuenta ni modelo externo.

Para repetir la simulación HDL, instala Icarus Verilog (`iverilog` y `vvp`) o
Verilator con compilador C++ y make. En WSL o Linux:

```sh
python verify.py --rtl
python verify.py --rtl --synth
```

La segunda orden requiere Yosys o `yowasp-yosys` y mapea el núcleo a la familia
Gowin GW1N. El informe se escribe en `build/verification.json`; los registros
de simulación y síntesis quedan junto a él. Una comprobación fallida elimina
el informe anterior para evitar confundirlo con un resultado nuevo.

## Probar en la Tang Nano 9K

1. Abre `boards/tang_nano_9k/corpus.gprj` con GOWIN IDE.
2. Comprueba el dispositivo `GW1NR-LV9QN88PC6/I5`, familia `GW1NR-9C`,
   y selecciona `corpus_selftest_top` como módulo superior.
3. Ejecuta síntesis y Place & Route. Revisa que el reloj de 27 MHz cumple
   el análisis temporal antes de generar el bitstream.
4. Carga el resultado con GOWIN Programmer a través del USB de la placa.
   La autoprueba utiliza el oscilador y los LED incorporados.

Los LED son activos a nivel bajo. LED0–LED3 muestran el número de caso en
binario, de 0 a 15; LED4 permanece encendido mientras los casos coinciden con
su resultado esperado; LED5 alterna con cada respuesta. Un fallo apaga LED4
hasta reiniciar. Se ejecuta un caso por segundo y la secuencia se repite.

El proyecto y sus pines se han contrastado con el ejemplo oficial de Sipeed.
La autoprueba ha sido simulada; no se ha ejecutado GOWIN IDE ni probado una
placa física en este entorno. **No se entrega un bitstream validado.**

## Archivos principales

| Archivo | Función |
|---|---|
| `rtl/trigate_forward.v` | Mayoría estable, complemento y residuo |
| `rtl/trigate_step.v` | Una acción relacional y máscara de alternativas conjuntas |
| `rtl/trigate_core.v` | Interfaz de reloj con aceptación y retención de respuestas |
| `reference/trigate.py` | Referencia ejecutable `solve(A,B,M,R,E)` |
| `reference/network.py` | Prueba de eventos, celdas compartidas y ramas aisladas |
| `docs/PROFILE.md` | Contrato, decisiones explícitas y límite de conformidad |
| `docs/VALIDATION.md` | Resultados de la ejecución entregada |

## Decisión pendiente del libro

La regla estricta de las tres áreas y el ejemplo inverso con un único `2`
en la base necesitan una conciliación: `(0,1,2; R=0,E=0)` exige `M=0`,
pero ninguna de sus tres áreas se clasifica como ambigua. El núcleo conserva
esa restricción y devuelve `needs_base_refinement=1`, sin declararla resuelta.
El detalle y su prueba están en `docs/PROFILE.md`.

## Fuentes

- Especificación proporcionada: AURORA 1.4.1 revisado, capítulos 3 y 6.1,
  apéndice de conteo y contrato de implementación.
- Base inspeccionada: Aurora-Program/Aurora-Trinity,
  commit `b35069b0c6f8cc0072a094d1e6508e01c7d4d540`.
- [Sipeed Tang Nano 9K](https://wiki.sipeed.com/hardware/en/tang/Tang-Nano-9K/Nano-9K.html).
- [Ejemplo oficial de LED y proyecto GOWIN](https://github.com/sipeed/TangNano-9K-example/tree/main/led).
- [Yosys synth_gowin](https://yosyshq.readthedocs.io/projects/yosys/en/v0.52/cmd/synth_gowin.html).

Código Apache-2.0; documentación CC-BY-4.0, siguiendo las licencias del proyecto.
Autoría y contexto: Programa Aurora, Pablo Álvarez Carrera y colaboración de IA.
