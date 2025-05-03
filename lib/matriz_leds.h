#ifndef MATRIZ_LEDS_H
#define MATRIZ_LEDS_H

#include <stdint.h>

#define NUM_PIXELS 25

//Função pública
void set_semaforo_led(uint8_t r, uint8_t g, uint8_t b, int numero);

#endif