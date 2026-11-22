#ifndef FLETCHER_H
#define FLETCHER_H

#include <stdint.h>

// Maximo de bytes que se pueden sumar en 32 bits sin reducir modulo 255
// (la cuenta esta en fletcher.c)
#define FLETCHER16_MAX_BLOCK 5802u

// Version de referencia: dos % 255 por byte
uint16_t fletcher16_naive(const uint8_t *data, uint32_t len);

// Optimizacion 1: resta condicional en lugar del % 255
uint16_t fletcher16_sub(const uint8_t *data, uint32_t len);

// Optimizacion 2: modulo diferido, se reduce una vez por bloque
uint16_t fletcher16_deferred(const uint8_t *data, uint32_t len);

// Optimizacion 3: modulo diferido en ensamblador, bucle desenrollado x8
// (fletcher_asm.S). Solo vale para len <= FLETCHER16_MAX_BLOCK
uint16_t fletcher16_asm(const uint8_t *data, uint32_t len);

#endif
