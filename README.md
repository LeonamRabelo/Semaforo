# 🚦 Semáforo Inteligente com Modo Noturno (FreeRTOS + BitDogLab)

Este projeto implementa um semáforo inteligente utilizando a placa **BitDogLab (RP2040)** com o sistema operacional em tempo real **FreeRTOS**. O semáforo possui dois modos de operação: **Normal** e **Noturno**, alternáveis via botão físico. O estado do semáforo é exibido nos LEDs RGB e em uma **matriz WS2812B**, além de um display OLED que mostra informações do sistema e buzzer buscando acessibilidade para pessoas cegas.

## 🎯 Funcionalidades

- ✅ Modo **Normal**:
  - Sequência padrão do semáforo:
    - Verde: 3s
    - Amarelo: 1.5s
    - Vermelho: 3s
  - Controlado via variáveis globais com tarefas FreeRTOS.
  - LEDs RGB e matriz WS2812 mostram o estado atual.

  - Emitir sinais sonoros com o buzzer para feedback a pessoas cegas.
    - Modo Normal:
        - Verde: 1 beep curto por um segundo “pode atravessar”; 
        - Amarelo: beep rápido intermitente “atenção”; 
        - Vermelho: tom contínuo curto (500ms ligado e 1.5s desligado) “pare”;
    - Modo Noturno: beep lento a cada 2s.
  
- 🌙 Modo **Noturno**:
  - Ativado pressionando o botão A.
  - LEDs Verde e Vermelho piscam juntos (simulando amarelo) a cada 1s.
  - A matriz WS2812B também pisca amarelo.
  - Display mostra o texto "MODO NOTURNO".

- 🧠 Tarefas RTOS:
  - `vTaskModo`: detecta pressionamento do botão A e alterna o modo.
  - `vTaskLeds`: gerencia LEDs RGB conforme o modo.
  - `vTaskMatrizLeds`: gerencia os LEDs WS2812.
  - `vTaskDisplay`: exibe o estado atual no display OLED via I2C.

---

## 🔧 Requisitos

- **Hardware:**
  - Placa BitDogLab (RP2040)
  - Matriz de LEDs WS2812B (5x5 ou compatível)
  - Display OLED SSD1306 (I2C)
  - Botão A conectado ao GPIO 5
  - LEDs RGB conectados aos GPIOs:
    - Verde: GPIO 11
    - Azul: GPIO 12
    - Vermelho: GPIO 13
  - Buzzer: GPIO 21

- **Software:**
  - FreeRTOS (incluso no projeto)
  - SDK do Raspberry Pi Pico
  - VS Code com extensões para C/C++
  - CMake

  ---

## ▶️ Como Compilar e Rodar

1. Clone o repositório:
   ```bash
   git clone https://github.com/LeonamRabelo/Semaforo.git
   cd semaforo-inteligente

2. Configure e Compile:
    mkdir build
    cd build
    cmake ..
    make

3. Grave o .uf2 na placa BitDogLab via USB.

   ---

## 📚 Autor
Desenvolvido por **Leonam Sousa Rabelo**
Baseado no RTOS FreeRTOS e exemplos do repositório do professor Wilton Lacerda, no programa EmbarcaTech.
Link para o repositório de exemplo do professor Wilton: https://github.com/wiltonlacerda/EmbarcaTechResU1Ex02