#include "MKL46Z4.h"

// LED (RG)
// LED_GREEN = PTD5
// LED_RED = PTE29
// Los dos LEDs son activos a nivel bajo (0 = encendido)

// SWITCHES
// SW1 = PTC3  (izquierdo)
// SW3 = PTC12 (derecho)
// Los dos botones son activos a nivel bajo (pulsado = 0)

// Estados del autómata:
//   0: los dos apagados (estado inicial)
//   1: rojo encendido, verde apagado
//   2: los dos encendidos
//   3: rojo apagado, verde encendido
volatile int estado = 0;   // se modifica dentro de la ISR

void delay(void)
{
  volatile int i;

  for (i = 0; i < 20000; i++);
}

// LED_GREEN = PTD5
void led_green_init()
{
  SIM->COPC = 0;
  SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
  PORTD->PCR[5] = PORT_PCR_MUX(1);
  GPIOD->PDDR |= (1 << 5);
  GPIOD->PSOR = (1 << 5);
}

void led_green_on(void)
{
  GPIOD->PCOR = (1 << 5);
}

void led_green_off(void)
{
  GPIOD->PSOR = (1 << 5);
}

// LED_RED = PTE29
void led_red_init()
{
  SIM->COPC = 0;
  SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
  PORTE->PCR[29] = PORT_PCR_MUX(1);
  GPIOE->PDDR |= (1 << 29);
  GPIOE->PSOR = (1 << 29);
}

void led_red_on(void)
{
  GPIOE->PCOR = (1 << 29);
}

void led_red_off(void)
{
  GPIOE->PSOR = (1 << 29);
}

// SW1 = PTC3, SW3 = PTC12
// Entradas GPIO con pull-up (PE = 1, PS = 1) e interrupción
// en flanco de bajada (IRQC = 1010), es decir, al pulsar
void sw_init(void)
{
  SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
  PORTC->PCR[3] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK |
                  PORT_PCR_IRQC(0xA);
  PORTC->PCR[12] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK |
                   PORT_PCR_IRQC(0xA);
  GPIOC->PDDR &= ~((1 << 3) | (1 << 12));

  // NVIC: PORTC y PORTD comparten la IRQ 31 (PORTC_PORTD_IRQn)
  NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
  NVIC_EnableIRQ(PORTC_PORTD_IRQn);
}

int sw1_pressed(void)
{
  return !(GPIOC->PDIR & (1 << 3));
}

int sw3_pressed(void)
{
  return !(GPIOC->PDIR & (1 << 12));
}

// Enciende/apaga los LEDs según el estado actual
void leds_update(void)
{
  switch (estado) {
  case 0: led_red_off(); led_green_off(); break;
  case 1: led_red_on();  led_green_off(); break;
  case 2: led_red_on();  led_green_on();  break;
  case 3: led_red_off(); led_green_on();  break;
  }
}

// Rutina de servicio de la IRQ 31 (PORTC/PORTD).
// El nombre PORTDIntHandler es el que aparece en la tabla de vectores de startup.c
void PORTDIntHandler(void)
{
  // Antirrebote sencillo: esperamos a que la señal del botón se estabilice
  delay();

  // Botón izquierdo (SW1): siguiente estado
  if ((PORTC->ISFR & (1 << 3)) && sw1_pressed()) {
    estado++;
    if (estado > 3)
      estado = 0;
  }

  // Botón derecho (SW3): estado anterior
  if ((PORTC->ISFR & (1 << 12)) && sw3_pressed()) {
    estado--;
    if (estado < 0)
      estado = 3;
  }

  leds_update();

  // Borramos los flags escribiendo un 1. Si no se borran,
  // la interrupción se volvería a ejecutar sin parar.
  PORTC->ISFR = (1 << 3) | (1 << 12);
}

int main(void)
{
  led_green_init();   // los dos LEDs empiezan apagados
  led_red_init();
  sw_init();

  // Flujo dirigido por interrupciones (Tema 2, Figura 2):
  // el procesador duerme hasta que llega una interrupción
  while (1)
    __WFI();

  return 0;
}
