cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
set(FREERTOS_KERNEL_PATH "C:/FreeRTOS-Kernel")
set(FREERTOS_KERNEL_PATH "C:/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(Semaforo C CXX ASM)
pico_sdk_init()

include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(${PROJECT_NAME}  
        Semaforo.c 
        lib/ssd1306.c # Biblioteca para o display OLED
        lib/matriz_leds.c
        )

# Generate PIO header
pico_generate_pio_header(Semaforo ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} 
        pico_stdlib 
        hardware_pwm
        hardware_i2c
        hardware_gpio
        hardware_pio
        FreeRTOS-Kernel         # Kernel do FreeRTOS
        FreeRTOS-Kernel-Heap4   # Gerenciador de memoria
        )

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})