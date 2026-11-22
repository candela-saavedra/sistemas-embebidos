#include "MKL46Z4.h"
#include "fletcher.h"

// Practica 3: benchmarking de Fletcher-16
//
// Cada version se ejecuta con N_SMALL y con N_LARGE bytes y se miden los
// ciclos con el SysTick. Como no hay consola, los resultados se quedan en la
// variable global "results" y se leen con OpenOCD (make results).
//
// Al acabar se enciende el LED verde (PTD5) si todos los checksums estan bien
// y el rojo (PTE29) si alguno falla. Los dos LEDs son activos a nivel bajo.

#define N_SMALL 100
#define N_LARGE 1024

#define RESULTS_DONE 0xF16C0DE5u

// Checksum de "abcde" que da el enunciado
#define ABCDE_CHECKSUM 0xC8F0u

#define COMPILER_BARRIER() __asm volatile ("" ::: "memory")

typedef uint16_t (*fletcher16_fn)(const uint8_t *data, uint32_t len);

// La 0 es la de referencia, las demas se comparan con ella
static const fletcher16_fn versions[] = {
  fletcher16_naive,
  fletcher16_sub,
  fletcher16_deferred,
  fletcher16_asm,
};

#define N_VERSIONS (sizeof(versions) / sizeof(versions[0]))

// Todo palabras de 32 bits para poder volcarlo con mdw (ver results.sh)
struct results {
  uint32_t done;                    // RESULTS_DONE cuando ha terminado todo
  uint32_t errors;                  // bit i a 1: la version i ha fallado
  uint32_t overhead;                // ciclos que cuesta medir sin hacer nada
  uint32_t cycles[N_VERSIONS][2];   // [version][0] = N_SMALL, [1] = N_LARGE
  uint32_t checksum[N_VERSIONS][2];
};

volatile struct results results;

static const uint8_t abcde[] = { 'a', 'b', 'c', 'd', 'e' };
static const uint32_t sizes[2] = { N_SMALL, N_LARGE };
static uint8_t data[N_LARGE];

// LED verde = PTD5, LED rojo = PTE29
static void leds_init(void)
{
  SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK;
  PORTD->PCR[5] = PORT_PCR_MUX(1);
  PORTE->PCR[29] = PORT_PCR_MUX(1);
  GPIOD->PSOR = (1 << 5);
  GPIOE->PSOR = (1 << 29);
  GPIOD->PDDR |= (1 << 5);
  GPIOE->PDDR |= (1 << 29);
}

// SysTick como contador libre de 24 bits con el reloj del nucleo:
// un tick por ciclo y sin interrupcion
static void systick_init(void)
{
  SysTick->LOAD = SysTick_LOAD_RELOAD_Msk;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

static uint32_t systick_get_ticks(void)
{
  COMPILER_BARRIER();
  uint32_t t = SysTick->VAL;
  COMPILER_BARRIER();
  return t;
}

#define timestamp() systick_get_ticks()

// El SysTick cuenta hacia abajo, por eso es start - end. La mascara cubre el
// caso de que el contador de la vuelta entre las dos lecturas.
static uint32_t elapsed(uint32_t start, uint32_t end)
{
  return (start - end) & SysTick_LOAD_RELOAD_Msk;
}

// Bytes pseudoaleatorios (LCG) generados en ejecucion, para que el compilador
// no sepa lo que hay en el array
static void data_init(void)
{
  uint32_t seed = 12345;

  for (uint32_t i = 0; i < N_LARGE; i++) {
    seed = seed * 1664525u + 1013904223u;
    data[i] = seed >> 24;
  }
}

int main(void)
{
  uint32_t start, end;
  uint32_t errors = 0;

  SIM->COPC = 0;  // fuera watchdog
  leds_init();
  systick_init();
  data_init();

  // Lo que cuesta la propia medida
  start = timestamp();
  end = timestamp();
  results.overhead = elapsed(start, end);

  for (uint32_t v = 0; v < N_VERSIONS; v++) {
    if (versions[v](abcde, sizeof(abcde)) != ABCDE_CHECKSUM)
      errors |= 1u << v;

    for (uint32_t s = 0; s < 2; s++) {
      uint16_t checksum;

      start = timestamp();
      checksum = versions[v](data, sizes[s]);
      end = timestamp();

      results.cycles[v][s] = elapsed(start, end);
      results.checksum[v][s] = checksum;
      if (checksum != results.checksum[0][s])
        errors |= 1u << v;
    }
  }

  results.errors = errors;
  results.done = RESULTS_DONE;

  if (errors)
    GPIOE->PCOR = (1 << 29);
  else
    GPIOD->PCOR = (1 << 5);

  while (1);

  return 0;
}
