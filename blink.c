#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"  // Inclua o cabeçalho gerado pelo PIO
#include "inc/ssd1306.h" // Inclua a biblioteca do display OLED
#include "inc/font.h"
#include <stdlib.h>      // Para rand() e srand()

// Definições de pinos
#define LED_MATRIX_PIN 7
#define I2C_PORT i2c1
#define OLED_SDA 14
#define OLED_SCL 15
#define OLED_ADDR 0x3C
#define BUZZER1 10
#define BUZZER2 21
#define BUTTON_A 5
#define BUTTON_B 6
#define JOYSTICK_BTN 22
#define JOYSTICK_X 27 // ADC1
#define JOYSTICK_Y 26 // ADC0
#define MICROPHONE 28 // ADC2
#define LED_R 13
#define LED_G 11
#define LED_B 12

// Variáveis do jogo
uint8_t cursor_x = 64; // Posição inicial do cursor (meio do display)
uint8_t cursor_y = 32;
uint8_t target_x = 64; // Posição inicial do alvo
uint8_t target_y = 32;
uint8_t score = 0;   // Pontuação
bool playing = true; // Estado do jogo
bool paused = false; // Pausado

// Declaração da variável global do display OLED
ssd1306_t ssd;

// Funções auxiliares
void init_peripherals();
void read_joystick();
bool detect_loud_sound();
void play_sound(uint buzzer, uint freq);
void update_led_matrix(uint8_t progress);
void set_rgb_led(uint8_t r, uint8_t g, uint8_t b);

// Função principal
int main() {
    stdio_init_all();
    init_peripherals();
    adc_select_input(2); // Usar microfone para semente
    srand(adc_read());   // Inicializar semente para rand()

    while (1) {
        if (playing && !paused) {
            // Ler joystick e atualizar cursor
            read_joystick();

            // Detectar som alto (exemplo: aumentar dificuldade)
            if (detect_loud_sound()) {
                play_sound(BUZZER2, 2000); // Toca som de alerta
            }

            // Verificar se o cursor está sobre o alvo
            if (abs(cursor_x - target_x) < 5 && abs(cursor_y - target_y) < 5) {
                if (!gpio_get(JOYSTICK_BTN)) {
                    score++;
                    play_sound(BUZZER1, 1000); // Toca som de acerto
                    target_x = rand() % WIDTH;
                    target_y = rand() % HEIGHT;
                    sleep_ms(200); // Debounce
                }
            }

            // Atualizar display OLED
            ssd1306_fill(&ssd, false); // Limpar display
            ssd1306_pixel(&ssd, cursor_x, cursor_y, true); // Desenhar cursor
            ssd1306_pixel(&ssd, target_x, target_y, true); // Desenhar alvo
            ssd1306_draw_string(&ssd, "Score: ", 0, 0); // Escrever "Score:"
            ssd1306_draw_char(&ssd, '0' + score, 48, 0); // Exibir pontuação
            ssd1306_send_data(&ssd);

            // Atualizar matriz de LEDs
            update_led_matrix(score);

            // Atualizar LED RGB (verde = jogando)
            set_rgb_led(0, 1, 0);
        } else if (paused) {
            // LED RGB amarelo (pausado)
            set_rgb_led(1, 1, 0);
        } else {
            // LED RGB vermelho (game over)
            set_rgb_led(1, 0, 0);
        }

        // Verificar botões
        if (!gpio_get(BUTTON_A)) {
            paused = !paused; // Alternar pausa
            sleep_ms(200); // Debounce
        }
        if (!gpio_get(BUTTON_B)) {
            // Reiniciar jogo
            score = 0;
            playing = true;
            paused = false;
            target_x = rand() % WIDTH;
            target_y = rand() % HEIGHT;
            sleep_ms(200); // Debounce
        }

        sleep_ms(20);
    }
    return 0;
}

void init_peripherals() {
    // Inicializar display OLED
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false); // Limpar display
    ssd1306_send_data(&ssd);

    // Inicializar matriz de LEDs
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_MATRIX_PIN, 800000, false);

    // Inicializar buzzers (PWM)
    gpio_set_function(BUZZER1, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER2, GPIO_FUNC_PWM);
    uint slice_num1 = pwm_gpio_to_slice_num(BUZZER1);
    uint slice_num2 = pwm_gpio_to_slice_num(BUZZER2);
    uint chan1 = pwm_gpio_to_channel(BUZZER1);
    uint chan2 = pwm_gpio_to_channel(BUZZER2);
    pwm_set_wrap(slice_num1, 1000);
    pwm_set_wrap(slice_num2, 1000);
    pwm_set_chan_level(slice_num1, chan1, 500);
    pwm_set_chan_level(slice_num2, chan2, 500);
    pwm_set_enabled(slice_num1, false);
    pwm_set_enabled(slice_num2, false);

    // Inicializar botões (entradas com pull-up)
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    // Inicializar joystick e microfone (ADC)
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    adc_gpio_init(MICROPHONE);

    // Inicializar LED RGB (saídas)
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
}

void read_joystick() {
    adc_select_input(1); // JOYSTICK_X (ADC1, GPIO 27)
    uint16_t x_val = adc_read();
    adc_select_input(0); // JOYSTICK_Y (ADC0, GPIO 26)
    uint16_t y_val = adc_read();
    cursor_x = (x_val * WIDTH) / 4096;  // Mapeia 0-4095 para 0-128
    cursor_y = (y_val * HEIGHT) / 4096; // Mapeia 0-4095 para 0-64
}

bool detect_loud_sound() {
    adc_select_input(2); // MICROPHONE (ADC2, GPIO 28)
    uint16_t mic_val = adc_read();
    return mic_val > 3000; // Limiar de som alto (ajuste conforme necessário)
}

void play_sound(uint buzzer, uint freq) {
    uint slice_num = pwm_gpio_to_slice_num(buzzer);
    uint chan = pwm_gpio_to_channel(buzzer);
    uint wrap = 125000000 / freq; // Calcula o período para a frequência desejada
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap / 2); // Duty cycle 50%
    pwm_set_enabled(slice_num, true);
    sleep_ms(100);
    pwm_set_enabled(slice_num, false);
}

void update_led_matrix(uint8_t progress) {
    // Controla uma matriz com 25 LEDs
    uint32_t colors[25] = {0}; // Inicializa todos desligados
    for (uint i = 0; i < progress && i < 25; i++) {
        colors[i] = 0x00FF00; // Verde
    }
    for (uint i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio0, 0, colors[i] << 8u); // Envia os dados para a matriz
    }
}

void set_rgb_led(uint8_t r, uint8_t g, uint8_t b) {
    gpio_put(LED_R, r);
    gpio_put(LED_G, g);
    gpio_put(LED_B, b);
}