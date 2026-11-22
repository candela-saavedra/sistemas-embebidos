#include "fletcher.h"

// Estan en un fichero aparte de main.c para que con -Ofast el compilador no
// pueda meterlas inline ni calcular el resultado al compilar. Asi siempre se
// mide una llamada de verdad.

// Version naive, tal cual la definicion:
//   sum1 = (sum1 + data[i]) % 255
//   sum2 = (sum2 + sum1) % 255
//   checksum = (sum2 << 8) | sum1
// El M0+ no tiene division, cada % acaba en una llamada a __aeabi_idivmod
uint16_t fletcher16_naive(const uint8_t *data, uint32_t len)
{
  uint16_t sum1 = 0;
  uint16_t sum2 = 0;

  for (uint32_t i = 0; i < len; i++) {
    sum1 = (sum1 + data[i]) % 255;
    sum2 = (sum2 + sum1) % 255;
  }
  return (sum2 << 8) | sum1;
}

// sum1 < 255 y data[i] <= 255, asi que sum1 + data[i] < 2*255 y basta con
// restar 255 una vez. Con sum2 pasa lo mismo. Sin ninguna division.
uint16_t fletcher16_sub(const uint8_t *data, uint32_t len)
{
  uint32_t sum1 = 0;
  uint32_t sum2 = 0;

  for (uint32_t i = 0; i < len; i++) {
    sum1 += data[i];
    if (sum1 >= 255)
      sum1 -= 255;
    sum2 += sum1;
    if (sum2 >= 255)
      sum2 -= 255;
  }
  return (sum2 << 8) | sum1;
}

// x mod 255 sin dividir. Como 256 = 1 (mod 255), sumar los bytes de x no
// cambia el resto (es la prueba del 9, pero en base 256).
static inline uint32_t mod255(uint32_t x)
{
  x = (x & 0xFFFF) + (x >> 16);  // <= 0x1FFFE
  x = (x & 0xFF) + (x >> 8);     // <= 0x2FE
  x = (x & 0xFF) + (x >> 8);     // <= 0x100
  if (x >= 255)
    x -= 255;
  return x;
}

// Modulo diferido: se suma en 32 bits y solo se reduce al final de cada bloque.
// En el peor caso (todos los bytes a 255) y empezando con las sumas < 255:
//   sum1 <= 254 + 255*n
//   sum2 <= 254 + 254*n + 255*n*(n+1)/2
// sum2 cabe en 32 bits hasta n = 5802, de ahi FLETCHER16_MAX_BLOCK.
// Con 100 o 1024 bytes solo se reduce una vez.
uint16_t fletcher16_deferred(const uint8_t *data, uint32_t len)
{
  uint32_t sum1 = 0;
  uint32_t sum2 = 0;

  while (len > 0) {
    uint32_t block = len > FLETCHER16_MAX_BLOCK ? FLETCHER16_MAX_BLOCK : len;
    const uint8_t *end = data + block;

    len -= block;
    do {
      sum1 += *data++;
      sum2 += sum1;
    } while (data != end);

    sum1 = mod255(sum1);
    sum2 = mod255(sum2);
  }
  return (sum2 << 8) | sum1;
}
