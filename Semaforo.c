#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include <stdio.h>
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "hardware/pwm.h"

uint16_t wrap = 0;
//Trecho para modo BOOTSEL com botão B//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events){
    reset_usb_boot(0, 0);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BOTAO_A 5
#define LED_VERDE 11
#define LED_AZUL 12
#define LED_VERMELHO 13
#define BUZZER_PIN 21
#define WS2812_PIN 7    //Pino do WS2812
#define NUM_PIXELS 25    //Quantidade de LEDs
#define IS_RGBW false   //Maquina PIO para RGBW
#define I2C_PORT i2c1 //I2C port
#define I2C_SDA 14    //I2C SDA -> dados
#define I2C_SCL 15    //I2C SCL -> clock
#define endereco 0x3C //Endereço do display
#define LED_AMARELO (gpio_get(LED_VERMELHO) && gpio_get(LED_VERDE)) //Led do semaforoamarelo, combinando o vermelho e verde

volatile int modo_noturno = 0; // 0 = Normal, 1 = Noturno
uint buzzer_slice;  //Slice para o buzzer

//Estrutura para o display
ssd1306_t ssd;

//Funcao para modularizar a inicialização dos componentes
void inicializar_componentes(){
    //Inicializa o botão A
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    //Inicializar os LEDs RGB
    gpio_init(LED_VERDE);
    gpio_init(LED_AZUL);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);

    //Inicializa o pio
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
  
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line                                              // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}
  


//Tarefa: Alternar Modo com botão
void vTaskModo(){
    int estado_anterior = 1;

    while(true){
        int atual = gpio_get(BOTAO_A);

        if (estado_anterior && !atual) {  // Borda de descida
            modo_noturno = !modo_noturno;
        }

        estado_anterior = atual;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//Tarefa: Controle do semaforo
void vTaskSemaforo(){
    while (true){
        if(modo_noturno){
            //Modo noturno: amarelo piscando
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 0);
            vTaskDelay(pdMS_TO_TICKS(1500));
        }else{
            // Modo normal: ciclo semafórico
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 0);
            gpio_put(LED_AZUL, 0);
            vTaskDelay(pdMS_TO_TICKS(3000)); // verde 3s
            if(modo_noturno) continue;  //Verifica se o modo noturno foi ativado, assim nao espera o ciclo do semaforo

            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            vTaskDelay(pdMS_TO_TICKS(1500)); //amarelo 1.5s
            if(modo_noturno) continue;  //Verifica se o modo noturno foi ativado, assim nao espera o ciclo do semaforo

            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            vTaskDelay(pdMS_TO_TICKS(2000)); //vermelho 2s
            //gpio_put(LED_VERMELHO, 0);
        }
    }
}

//Tarefa: Buzzer
void vTaskBuzzer(){
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint buzzer_slice = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Define frequência de 1000Hz (tom agradável)
    uint32_t freq = 1000;
    uint32_t clock = 125000000; // clock base do RP2040
    uint32_t wrap = 125000;     // clock / freq
    pwm_set_wrap(buzzer_slice, wrap);
    pwm_set_clkdiv(buzzer_slice, 1.0f); // divisão mínima para melhor resolução

    // Duty cycle (~30% do wrap)
    uint16_t duty = 0.3 * wrap;

    // Ativa PWM, mas começa com duty 0
    pwm_set_gpio_level(BUZZER_PIN, 0);
    pwm_set_enabled(buzzer_slice, true);

    while (true) {
        if (modo_noturno) {
            // Bipe suave a cada 2s
            pwm_set_gpio_level(BUZZER_PIN, duty);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1800));
        } else if (gpio_get(LED_VERDE) && !gpio_get(LED_VERMELHO)) {
            // Verde: 1 bipe curto
            pwm_set_gpio_level(BUZZER_PIN, duty);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(800));
        } else if (gpio_get(LED_VERDE) && gpio_get(LED_VERMELHO)) {
            // Amarelo: 4 bipes rápidos
            for (int i = 0; i < 4; i++) {
                pwm_set_gpio_level(BUZZER_PIN, duty);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_gpio_level(BUZZER_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); // pausa após o ciclo
        } else if (!gpio_get(LED_VERDE) && gpio_get(LED_VERMELHO)) {
            // Vermelho: bipe longo
            pwm_set_gpio_level(BUZZER_PIN, duty);
            vTaskDelay(pdMS_TO_TICKS(500));
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1500));
        } else {
            // Nenhuma condição ativa
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}


int main(){
        // Para ser utilizado o modo BOOTSEL com botão B
        gpio_init(botaoB);
        gpio_set_dir(botaoB, GPIO_IN);
        gpio_pull_up(botaoB);
        gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
        ////////////////////////////////////////////
        
    inicializar_componentes();

    xTaskCreate(vTaskModo, "BotaoModo", 256, NULL, 2, NULL);
    xTaskCreate(vTaskSemaforo, "LEDs", 256, NULL, 1, NULL);
    xTaskCreate(vTaskBuzzer, "Buzzer", 256, NULL, 1, NULL);

    vTaskStartScheduler();
}
