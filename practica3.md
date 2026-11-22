# Práctica 3: benchmarking de implementaciones de Fletcher-16

Placa FRDM-KL46Z (MCU KL46Z, ARM Cortex-M0+), `arm-none-eabi-gcc` 13.2.1.

## Trabajo realizado

Se calcula el checksum Fletcher-16 de un array de bytes:

```
sum1 = (sum1 + dato[i]) mod 255
sum2 = (sum2 + sum1)    mod 255
checksum = (sum2 << 8) | sum1
```

Hay cuatro implementaciones, todas con la misma interfaz
`uint16_t f(const uint8_t *data, uint32_t len)`:

| Rutina | Fichero | Idea |
|---|---|---|
| `fletcher16_naive` | `fletcher.c` | La definición tal cual: dos `% 255` por byte |
| `fletcher16_sub` | `fletcher.c` | El módulo se sustituye por una resta condicional |
| `fletcher16_deferred` | `fletcher.c` | Módulo diferido: sumas de 32 bits, se reduce una vez por bloque |
| `fletcher16_asm` | `fletcher_asm.S` | Módulo diferido en ensamblador, bucle desenrollado x8 |

`main.c` rellena un array con bytes pseudoaleatorios en tiempo de ejecución,
ejecuta cada rutina con 100 y con 1024 elementos y mide los ciclos con el
SysTick. Además comprueba que todas devuelven lo mismo que la versión naive y
que el checksum de `abcde` es `0xC8F0`: si todo es correcto enciende el LED
verde, y si no, el rojo.

### Medición

El SysTick se configura como contador libre de 24 bits con el reloj del núcleo
(`CLKSOURCE = 1`), sin interrupción, así que cada tick es un ciclo de reloj. Se
toma el valor justo antes y justo después de la llamada, como en el tema 6.
Como el SysTick cuenta hacia abajo, los ciclos son `(start - end) & 0xFFFFFF`.

Las rutinas están en una unidad de compilación distinta de `main.c` y el array
se rellena en tiempo de ejecución, para que con `-Ofast` el compilador no pueda
hacer *inline* ni precalcular el resultado.

El proyecto no tiene consola, así que los resultados se guardan en la variable
global `results` y se leen con OpenOCD (`make results`). El proyecto tampoco
configura el reloj, así que el MCU funciona con la configuración de reset y no
a 48 MHz como en las diapositivas; por eso los resultados se dan en ciclos.

## Resultados

Ciclos de reloj medidos en la placa con el SysTick. Las medidas son
deterministas: varias ejecuciones dan exactamente los mismos valores.

| Rutina | N=100, `-O0` | N=100, `-Ofast` | N=1024, `-O0` | N=1024, `-Ofast` |
|---|---:|---:|---:|---:|
| C naive | 10031 | 6045 | 101507 | 61483 |
| C resta condicional | 4729 | 1432 | 47209 | 14366 |
| C módulo diferido | 2974 | 885 | 27922 | 8275 |
| asm módulo diferido x8 | 576 | 523 | 4724 | 4669 |

Coste de la propia medición (dos *timestamps* seguidos): 29 ciclos en `-O0` y 3
en `-Ofast`. Está incluido en todos los valores de la tabla.

Ciclos por byte, calculados como (ciclos con 1024 − ciclos con 100) / 924, lo
que elimina la parte fija (llamada, medición y reducción final):

| Rutina | `-O0` | `-Ofast` |
|---|---:|---:|
| C naive | 99,0 | 60,0 |
| C resta condicional | 46,0 | 14,0 |
| C módulo diferido | 27,0 | 8,0 |
| asm módulo diferido x8 | 4,5 | 4,5 |

## Optimizaciones

**Por qué la versión naive es tan lenta.** El Cortex-M0+ no tiene instrucción
de división, así que cada `% 255` se convierte en una llamada a la rutina
software `__aeabi_idivmod` de libgcc. `-Ofast` no lo arregla: en el
desensamblado siguen estando las dos llamadas por byte. El compilador mejora
todo lo que rodea a la división (de 99 a 60 ciclos por byte), pero no la
división.

**1. Resta condicional (`fletcher16_sub`).** Como `sum1 < 255` y el dato vale
como mucho 255, `sum1 + dato <= 509 < 2·255`, y lo mismo ocurre con `sum2`.
Una sola resta condicional (`if (x >= 255) x -= 255`) equivale al módulo, y
desaparece la división.

**2. Módulo diferido (`fletcher16_deferred`).** El módulo se puede aplicar al
final en lugar de en cada paso, siempre que las sumas no desborden. Empezando
un bloque con las dos sumas por debajo de 255, tras `n` bytes de valor 255:

```
sum1 <= 254 + 255·n
sum2 <= 254 + 254·n + 255·n·(n+1)/2
```

lo que cabe en 32 bits hasta `n = 5802`. El bucle interno queda en una carga y
dos sumas por byte, y solo se reduce una vez por bloque de 5802 bytes (una
única vez para 100 y para 1024 elementos).

La reducción final tampoco divide: como 2^8 ≡ 1 (mod 255), un número es
congruente con la suma de sus "dígitos" en base 256. Se pliega tres veces
(`(x & 0xFFFF) + (x >> 16)`, y dos veces `(x & 0xFF) + (x >> 8)`) y se termina
con una resta condicional.

**3. Ensamblador (`fletcher16_asm`).** Es el mismo algoritmo de módulo diferido
escrito a mano para el Cortex-M0+ (solo Thumb-1). El compilador genera 8 ciclos
por byte: `ldrb` (2), incremento del puntero (1), dos sumas (2), comparación
(1) y salto (2). Desenrollando el bucle de 8 en 8 con `ldrb r4, [r0, #k]`, el
control del bucle (4 ciclos) se paga una vez cada 8 bytes y quedan 4,5 ciclos
por byte. Los bytes que sobran (`len % 8`) se procesan en un bucle simple.
Limitación: exige `len <= 5802`, porque solo reduce una vez al final.

Los ciclos por byte medidos (8,0 y 4,5) coinciden exactamente con los que se
obtienen sumando los ciclos de cada instrucción según el manual del
Cortex-M0+.

## Conclusiones

- El cambio de algoritmo es lo que más aporta. Con 1024 elementos, `-Ofast`
  por sí solo mejora la versión naive 1,65 veces; quitar la división la mejora
  4,3 veces más, y diferir el módulo, 7,4 veces (siempre respecto a naive con
  `-Ofast`).
- La versión en ensamblador es 13,2 veces más rápida que el mejor código que
  saca el compilador de la versión naive (61483 frente a 4669 ciclos), y 1,8
  veces más rápida que la mejor versión en C (8275 ciclos).
- El coste escala linealmente con el tamaño: de 100 a 1024 elementos los ciclos
  se multiplican aproximadamente por 10 en todas las versiones. La parte fija
  (entrada, salida y reducción final) solo se nota en las versiones rápidas con
  100 elementos.
- En la rutina en ensamblador `-O0` y `-Ofast` dan casi lo mismo (unos 55
  ciclos de diferencia), porque el código de la rutina es idéntico: la
  diferencia es el coste de la llamada y de la toma de tiempos en `main.c`,
  igual que en el ejemplo de `reverse_bits()` del tema 6.

## Cómo reproducir las medidas

Con la placa conectada:

```
make OPT=-O0 flash
make OPT=-O0 results

make OPT=-Ofast flash
make OPT=-Ofast results
```

`make results` reinicia la placa, espera a que termine el benchmark y vuelca
la variable `results` en forma de tabla. Al cambiar `OPT` se recompila todo
automáticamente.
