# Práctica 3 - Sistemas Embebidos (FRDM-KL46Z)

Benchmarking de implementaciones del checksum Fletcher-16 en el ARM
Cortex-M0+ de la FRDM-KL46Z, midiendo los ciclos de ejecución con el SysTick
para arrays de 100 y 1024 bytes, con `-O0` y con `-Ofast`.

La memoria, con las tablas de tiempos, la descripción de las optimizaciones y
las conclusiones, está en [`practica3.md`](practica3.md).

## Uso

- `make OPT=-O0` / `make OPT=-Ofast`: compila con ese nivel de optimización
  (por defecto `-O0`). Al cambiar de nivel se recompila todo.
- `make OPT=... flash`: compila y graba el benchmark en la flash de la placa.
- `make OPT=... results`: reinicia la placa, espera a que termine el benchmark
  y muestra la tabla de ciclos leyendo la memoria con OpenOCD.
- `make clean`: borra los ficheros generados.

Al terminar el benchmark se enciende el LED verde si todos los checksums son
correctos, y el rojo si alguno falla.

## Estructura

- `fletcher.c`, `fletcher.h`: versiones en C (naive, resta condicional y
  módulo diferido).
- `fletcher_asm.S`: módulo diferido en ensamblador, con el bucle desenrollado
  x8.
- `main.c`: medición con el SysTick y comprobación de los checksums.
- `results.sh`: lectura de los resultados desde la placa (`make results`).
- `startup.c`, `link.ld`, `openocd.cfg`, `includes/`: arranque, linker script,
  configuración de OpenOCD y cabeceras del MCU.

## Autores

- Candela Saavedra
- Cristian Vázquez
