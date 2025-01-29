//bibliotecas necessárias
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/bootrom.h"
#include "ws2812.pio.h"

//definições de hardware
#define PIN_TX 7
#define NLEDS 25
#define WIDTH 5
#define HEIGHT 5
#define BUTTON_A 5
#define BUTTON_B 6
#define LED_R 13


// Variáveis globais para controle de hardware
static PIO pio;
static int sm;
static uint dma_chan;
static uint32_t fitaEd[NLEDS]; // Buffer para armazenar o estado dos LEDs
volatile int number = 0;       // Número a ser exibido na matriz
volatile bool button_a_pressed = false; // Flag para botão A pressionado
volatile bool button_b_pressed = false; // Flag para botão B pressionado

// Função para converter valores RGB para um formato de 32 bits para os LEDs WS2812
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(b) << 8);
}

// Atualiza a matriz de LEDs utilizando DMA
static void atualizaFita() {
    dma_channel_wait_for_finish_blocking(dma_chan);
    while (!pio_sm_is_tx_fifo_empty(pio, sm)) {
        sleep_us(10);
    }
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &c, &pio->txf[sm], fitaEd, NLEDS, true);
    sleep_us(300);
}

// Apaga todos os LEDs da matriz
static void apagaLEDS() {
    memset(fitaEd, 0, sizeof(fitaEd));
    atualizaFita();
}

// Função para piscar o LED vermelho
void blink_red_led() {
    static bool state = false;
    gpio_put(LED_R, state);
    state = !state;
}

// Exibe um número na matriz de LEDs
void display_number(int num) {
     static const uint32_t numbers[10][25] = {
        // Matriz representando os dígitos de 0 a 9
        {   // Dígito 0
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 0, 0, 0, 1,
            1, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 1
            0, 0, 1, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 1, 0, 0
        },
        {   // Dígito 2
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 0,
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 3
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            0, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 4
            1, 0, 0, 0, 0,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 0, 0, 0, 1
        },
        {   // Dígito 5 
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 0,
            1, 1, 1, 1, 1
        },
        {   // Dígito 6
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 7
            1, 0, 0, 0, 0,
            0, 0, 0, 0, 1,
            1, 0, 0, 0, 0,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 8
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 9
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        }
    };

    // Atualiza a matriz de LEDs com base no número selecionado
    for (int i = 0; i < 25; i++) {
        fitaEd[i] = numbers[num][i] ? urgb_u32(33,33,33) : urgb_u32(0, 0, 0);
    }

    // Atualiza a fita LED com a nova exibição
    atualizaFita();
}

void button_isr(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A) {
        button_a_pressed = true;
    } else if (gpio == BUTTON_B) {
        button_b_pressed = true;
    }
}


int main() {
    stdio_init_all(); // Inicializa a comunicação serial
    pio = pio0;       // Define o bloco PIO a ser usado
    sm = 0;           // Define o state machine
    dma_chan = dma_claim_unused_channel(true); // Obtém um canal DMA disponível
    uint offset = pio_add_program(pio, &ws2812_program); // Adiciona programa PIO para LEDs
    ws2812_program_init(pio, sm, offset, PIN_TX, 800000, false); // Inicializa WS2812

    // Configuração do LED vermelho
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    
    // Configuração do timer para piscar o LED vermelho
    repeating_timer_t timer;
    add_repeating_timer_ms(-200, (repeating_timer_callback_t)blink_red_led, NULL, &timer);

    // Configuração do botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, button_isr);
    
    // Configuração do botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, button_isr);

    apagaLEDS(); // Garante que os LEDs comecem apagados

    while (1) {
    if (button_a_pressed) {
        number = (number + 1) % 10;
        display_number(number);
        button_a_pressed = false;  // Resetar a flag após o uso
    }
    if (button_b_pressed) {
        number = (number - 1 + 10) % 10;
        display_number(number);
        button_b_pressed = false;  // Resetar a flag após o uso
    }
    sleep_ms(500); // Pequeno delay para evitar múltiplas leituras acidentais
}
}