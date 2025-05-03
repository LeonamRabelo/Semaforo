#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "matriz_leds.h"

//Função para ligar um LED
static inline void put_pixel(uint32_t pixel_grb){
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

//Função para converter cores RGB para um valor de 32 bits
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b){
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

bool cores[4][NUM_PIXELS] = {
    //Verde (posição 17 central)
    {
        [17] = 1
    },
    //Amarelo (posição 12 central)
    {
        [12] = 1
    },
    //Vermelho (posição 7 central)
    {
        [7] = 1
    },
    //Número 2, cor vermelha
    {
    0, 1, 1, 1, 0,      
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0,   
    0, 1, 0, 1, 0,  
    0, 1, 1, 1, 0
    }
};

void set_semaforo_led(uint8_t r, uint8_t g, uint8_t b, int numero){
    for(int i = 0; i < NUM_PIXELS; i++){
        if(cores[3][i]){
            //Moldura → cinza
            put_pixel(urgb_u32(15, 4, 0));
        }else if(cores[numero][i]){
            //Centro ativo → cor do semáforo
            put_pixel(urgb_u32(r, g, b));
        }else{
            //Apagar LEDs não usados
            put_pixel(0);
        }
    }
}