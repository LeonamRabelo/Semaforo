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
//Verde
{
    0, 1, 1, 1, 0,      
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1,   
    1, 1, 1, 1, 1,  
    0, 1, 1, 1, 0
    },
//Amarelo (Exclamação de atenção)
{
    0, 0, 1, 0, 0,      
    0, 0, 0, 0, 0, 
    0, 0, 1, 0, 0,   
    0, 0, 1, 0, 0,  
    0, 0, 1, 0, 0
    },
//Vermelho
{
    0, 1, 1, 1, 0,      
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1,   
    1, 1, 1, 1, 1,  
    0, 1, 1, 1, 0
    },
//Apagar tudo
{
    0, 0, 0, 0, 0,      
    0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0,   
    0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0
}
};

//Função para ligar os LEDs de acordo com o estado do semaforo
void set_semaforo_led(uint8_t r, uint8_t g, uint8_t b, int numero){
    for(int i = 0; i < NUM_PIXELS; i++){
        if(cores[numero][i] == 1){
            if(numero == 1){
                //Amarelo: toda área ativa é amarela
                put_pixel(urgb_u32(255, 255, 0));
            }else{
                //Verde ou Vermelho: centro na cor desejada, borda branca
                if(i == 6 || i == 7 || i == 8 ||    //segunda linha (centro)
                   i == 11 || i == 12 || i == 13 || //terceira linha (centro, exceto 13)
                   i == 16 || i == 17 || i == 18)   //quarta linha (centro)
                {
                    put_pixel(urgb_u32(r, g, b)); // centro
                }else{
                    put_pixel(urgb_u32(1, 1, 1)); // borda branca
                }
            }
        }else{
            //Fundo escuro
            put_pixel(urgb_u32(0, 0, 0));
        }
    }
}