#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include <stdio.h>
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "lib/matriz_leds.h"

#define BOTAO_A 5               //Pino do botão A
#define LED_VERDE 11            //Pino do LED Verde
#define LED_AZUL 12             //Pino do LED Azul
#define LED_VERMELHO 13         //Pino do LED Vermelho
#define BUZZER_PIN 21           //Pino do Buzzer
#define WS2812_PIN 7            //Pino do WS2812
#define IS_RGBW false           //Usado na configuração da maquina PIO
#define I2C_PORT i2c1    //I2C port
#define I2C_SDA 14      //I2C SDA -> dados
#define I2C_SCL 15      //I2C SCL -> clock
#define endereco 0x3C   //Endereço do display

volatile bool modo_noturno = false; //0 = Normal, 1 = Noturno / Variavel global para o modo noturno
volatile int estado_semaforo = 0; //0 = Verde, 1 = Amarelo, 2 = Vermelho /  Variavel global para o semaforo

//Estrutura para o display
ssd1306_t ssd;

//Tarefa: Alternar Modo ("Normal" e "Noturno") com botão A
void vTaskModo(){
    //Inicializa o botão A
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    int estado_anterior = true;     //Variavel para capturar borda de descida do botao

    while(true){
        int atual = gpio_get(BOTAO_A);  //Captura o estado do botao

        if(estado_anterior && !atual){  //Borda de descida
            modo_noturno = !modo_noturno;   //Alterna o estado da variavel do modo noturno
            estado_semaforo = 0;    //Reinicializa o semaforo a partir do verde
        }
        estado_anterior = atual;    //Armazena o estado anterior
        vTaskDelay(pdMS_TO_TICKS(100)); //Pequeno debounce
    }
}

//Tarefa: Controle dos leds RGB
void vTaskLeds(){
    //Inicializar os LEDs RGB
    gpio_init(LED_VERDE);
    gpio_init(LED_AZUL);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);

    bool piscar = false;    //Variavel para controle do piscar do led amarelo no modo noturno

    while(true){
        if(modo_noturno){   //Caso modo noturno ativado
            estado_semaforo = -1;   //O semafaro fica em um estado "desconhecido"
            if(piscar){ //Piscando
                gpio_put(LED_VERDE, 1);
                gpio_put(LED_VERMELHO, 1); // Amarelo
            }else{  //Apagado
                gpio_put(LED_VERDE, 0);
                gpio_put(LED_VERMELHO, 0);
            }
            gpio_put(LED_AZUL, 0); //Azul sempre desligado no modo noturnoja que utilizamos o amarelo
            piscar = !piscar; // alterna o estado
            vTaskDelay(pdMS_TO_TICKS(1000)); //Piscar a cada 1 segundo
            continue;
        }else{  //Caso modo noturno desativado
        switch(estado_semaforo){    //Switch para controle do semaforo
            case 0:     //Verifica o estado do semaforo, se 0 = verde
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 0);
            gpio_put(LED_AZUL, 0);
            vTaskDelay(pdMS_TO_TICKS(3000));
            estado_semaforo = 1; //Passa para o amarelo
            break;

            case 1: //Verifica o estado do semaforo, se 1 = amarelo
            gpio_put(LED_VERDE, 1);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            vTaskDelay(pdMS_TO_TICKS(1500));
            estado_semaforo = 2; //Passa para o vermelho
            break;

            case 2: //Verifica o estado do semaforo, se 2 = vermelho
            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 1);
            gpio_put(LED_AZUL, 0);
            vTaskDelay(pdMS_TO_TICKS(3000));
            estado_semaforo = 0; //Volta para o verde
            break;
        }
    }
    }
}

//Tarefa: Controle da matriz de LEDs
void vTaskMatrizLeds(){
    //Inicializa o pio
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    while(true){
        if(modo_noturno){   //Caso modo noturno ativado
            set_semaforo_led(25, 25, 0, 1); //Amarelo
            vTaskDelay(pdMS_TO_TICKS(200));
        }else{
            switch(estado_semaforo){
                case 0:
                    set_semaforo_led(0, 25, 0, 0); //Verde
                    break;
                case 1:
                    set_semaforo_led(25, 25, 0, 1); //Amarelo
                    break;
                case 2:
                    set_semaforo_led(25, 0, 0, 2); //Vermelho
                    //vTaskDelay(pdMS_TO_TICKS(1000));    //Delay para sincronizar com buzzer
                    break;
                default:
                    set_semaforo_led(0, 0, 0, 3); //Desliga tudo
                    break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

//Tarefa: Controle do display
void vTaskDisplay(){
    //I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    //Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    //Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        //Pull up the data line
    gpio_pull_up(I2C_SCL);                                        //Pull up the clock line                                              // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); //Inicializa o display
    ssd1306_config(&ssd);                                         //Configura o display
    ssd1306_send_data(&ssd);                                      //Envia os dados para o display
    //Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    while(true){
        if(modo_noturno){   //Caso o modo noturno ativado
            //Limpa o display. O display inicia com todos os pixels apagados.
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "MODO NOTURNO", 15, 30);
        }else{  //Caso o modo noturno desativado
            //Limpa o display.
            ssd1306_fill(&ssd, false);
            if(estado_semaforo == 0){       //Verifica o estado do semaforo, se for 0 = verde
                ssd1306_draw_string(&ssd, "Sinal Verde", 20, 20);
                ssd1306_draw_string(&ssd, "Prossiga!", 30, 35);
            }else if(estado_semaforo == 1){ //Verifica o estado do semaforo, se for 1 = amarelo
                ssd1306_draw_string(&ssd, "Sinal Amarelo", 10, 20);
                ssd1306_draw_string(&ssd, "Atencao!", 33, 35);
            }else if(estado_semaforo == 2){ //Verifica o estado do semaforo, se for 2 = vermelho
                ssd1306_draw_string(&ssd, "Sinal Vermelho", 5, 20);
                ssd1306_draw_string(&ssd, "Pare!", 42, 35);
            }else   //Caso o semaforo esteja em um estado desconhecido, caso de erro
                ssd1306_draw_string(&ssd, "Aguardando...", 20, 30);
        }
        ssd1306_rect(&ssd, 3, 2, 120, 61, true, false);     //Borda do display
        ssd1306_send_data(&ssd);    //Envia os dados para o display
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//Tarefa: Controle do Buzzer
void vTaskBuzzer(){
    //Inicialização do Buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint buzzer_slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    //Define frequência de 1000Hz (tom agradável)
    uint32_t freq = 1000;
    uint32_t clock = 125000000; //Clock base do RP2040
    uint32_t wrap = 125000;     //Clock / freq
    pwm_set_wrap(buzzer_slice, wrap);
    pwm_set_clkdiv(buzzer_slice, 1.0f); //Divisão mínima para melhor resolução
    //Duty cycle (~30% do wrap)
    uint16_t duty = 0.3 * wrap;
    //Ativa PWM, mas começa com duty 0
    pwm_set_gpio_level(BUZZER_PIN, 0);
    pwm_set_enabled(buzzer_slice, true);

    while(true){
        if(modo_noturno){   //Caso o modo noturno ativado
            //Bipe suave a cada 2s (200ms de alto e 1800ms de baixo)
            pwm_set_gpio_level(BUZZER_PIN, duty);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1800));
        }else if(gpio_get(LED_VERDE) && !gpio_get(LED_VERMELHO)){   //Caso o semaforo esteja verde
            //Verde: 1 bipe curto por um segundo
            pwm_set_gpio_level(BUZZER_PIN, duty);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(800));
            if(modo_noturno) continue;  //Verifica se o modo noturno foi ativado
        }else if(gpio_get(LED_VERDE) && gpio_get(LED_VERMELHO)){    //Caso o semaforo esteja amarelo
            //Amarelo: 4 bipes rápidos
            for(int i = 0; i < 4; i++){
                pwm_set_gpio_level(BUZZER_PIN, duty);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_gpio_level(BUZZER_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                if(modo_noturno) continue;  //Verifica se o modo noturno foi ativado
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); //pausa após o ciclo
        }else if(!gpio_get(LED_VERDE) && gpio_get(LED_VERMELHO)){   //Caso o semaforo esteja vermelho
            //Vermelho: bipe longo (500 ligado e 1500 desligado)
            pwm_set_gpio_level(BUZZER_PIN, duty);
            vTaskDelay(pdMS_TO_TICKS(500));
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1500));
            if(modo_noturno) continue;  //Verifica se o modo noturno foi ativado
        }else{
            //Nenhuma condição ativa
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

int main(){
    stdio_init_all();

    //Cria as tarefas e inicia o scheduler
    xTaskCreate(vTaskModo, "BotaoModo", 256, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskLeds, "LEDs", 256, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskBuzzer, "Buzzer", 256, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskMatrizLeds, "MatrizLEDs", 256, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskDisplay, "Display", 256, NULL, tskIDLE_PRIORITY, NULL);

    vTaskStartScheduler();
}
