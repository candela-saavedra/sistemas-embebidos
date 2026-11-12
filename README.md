# Práctica 2 - Sistemas Embebidos (FRDM-KL46Z)

Ejemplos `led_blinky` y `hello_world` de las *demo apps* de la SDK de la
FRDM-KL46Z, compilados con la toolchain GNU (GCC + OpenOCD + GDB) mediante
un Makefile propio.

## Uso

- `make`: compila los dos binarios (`led_blinky.elf` y `hello_world.elf`).
- `make flash_led`: compila y graba `led_blinky` en la flash de la placa.
- `make flash_hello`: compila y graba `hello_world` en la flash de la placa.
- `make debug_led` / `make debug_hello`: abre una sesión de GDB contra la placa.
- `make clean`: borra los ficheros generados.

`hello_world` escribe por el puerto serie de la OpenSDA a 115200 baudios.

## Estructura

- `led_blinky/`, `hello_world/`: código de cada ejemplo y su `pin_mux`.
- `board/`, `drivers/`, `utilities/`, `device/`, `CMSIS/`: código de la SDK
  común a los dos ejemplos.
- `startup.c`, `link.ld`, `openocd.cfg`: arranque, linker script y
  configuración de OpenOCD.

## Autores

- Candela Saavedra
- Cristian Vázquez
