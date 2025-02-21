#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include <stdlib.h>
#include <stdio.h>

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
#define JOYSTICK_X 27
#define JOYSTICK_Y 26
#define MICROPHONE 28
#define LED_R 13
#define LED_G 11
#define LED_B 12

// Tamanho do display
#define WIDTH 128
#define HEIGHT 64

// Variáveis do jogo
uint8_t cursor_x = 64;
uint8_t cursor_y = 32;
uint8_t target_x = 64;
uint8_t target_y = 32;
uint8_t score = 0;
bool playing = true;
bool paused = false;

ssd1306_t ssd;

void init_peripherals();
void read_joystick();
bool detect_loud_sound();
void play_sound(uint buzzer, uint freq);
void update_led_matrix(uint8_t progress);
void set_rgb_led(uint8_t r, uint8_t g, uint8_t b);

int main() {
    stdio_init_all();
    init_peripherals();
    adc_select_input(2);
    srand(adc_read());

    while (1) {
        if (playing && !paused) {
            read_joystick();

            if (detect_loud_sound()) {
                play_sound(BUZZER2, 3000);
            }

            // Verificação de colisão ajustada
            if (abs(cursor_x - target_x) < 3 && abs(cursor_y - target_y) < 3) {
                if (!gpio_get(JOYSTICK_BTN)) {
                    score++;
                    play_sound(BUZZER1, 2000);
                    target_x = rand() % (WIDTH - 2);
                    target_y = rand() % (HEIGHT - 2);
                    sleep_ms(50);
                }
            }

            ssd1306_fill(&ssd, false);

            // Desenhar cursor 3x3
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int x = cursor_x + dx;
                    int y = cursor_y + dy;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                        ssd1306_pixel(&ssd, x, y, true);
                    }
                }
            }

            // Desenhar alvo 3x3
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int x = target_x + dx;
                    int y = target_y + dy;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                        ssd1306_pixel(&ssd, x, y, true);
                    }
                }
            }

            // Exibir pontuação como string
            char score_str[4];
            snprintf(score_str, sizeof(score_str), "%d", score);
            ssd1306_draw_string(&ssd, "Score:", 0, 0);
            
            int x_pos = 48;
            for (int i = 0; score_str[i] != '\0'; i++) {
                ssd1306_draw_char(&ssd, score_str[i], x_pos, 0);
                x_pos += 8;
            }

            ssd1306_send_data(&ssd);
            update_led_matrix(score);
            set_rgb_led(0, 1, 0);
        } else if (paused) {
            set_rgb_led(1, 1, 0);
        } else {
            set_rgb_led(1, 0, 0);
        }

        if (!gpio_get(BUTTON_A)) {
            paused = !paused;
            sleep_ms(200);
        }
        if (!gpio_get(BUTTON_B)) {
            score = 0;
            playing = true;
            paused = false;
            target_x = rand() % WIDTH;
            target_y = rand() % HEIGHT;
            sleep_ms(200);
        }

        sleep_ms(20);
    }
    return 0;
}

void init_peripherals() {
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_MATRIX_PIN, 800000, false);

    gpio_set_function(BUZZER1, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER2, GPIO_FUNC_PWM);
    uint slice_num1 = pwm_gpio_to_slice_num(BUZZER1);
    uint slice_num2 = pwm_gpio_to_slice_num(BUZZER2);
    pwm_set_wrap(slice_num1, 1000);
    pwm_set_wrap(slice_num2, 1000);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    adc_gpio_init(MICROPHONE);

    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
}

void read_joystick() {
    adc_select_input(1);
    uint16_t x_val = adc_read();
    adc_select_input(0);
    uint16_t y_val = adc_read();
    cursor_x = (x_val * (WIDTH - 1)) / 4096;
    cursor_y = (y_val * (HEIGHT - 1)) / 4096;
}

bool detect_loud_sound() {
    adc_select_input(2);
    uint16_t mic_val = adc_read();
    return mic_val > 3000;
}

void play_sound(uint buzzer, uint freq) {
    uint slice_num = pwm_gpio_to_slice_num(buzzer);
    uint chan = pwm_gpio_to_channel(buzzer);
    uint wrap = 125000000 / freq;
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap / 2);
    pwm_set_enabled(slice_num, true);
    sleep_ms(200);  // Som mais longo
    pwm_set_enabled(slice_num, false);
}

void update_led_matrix(uint8_t progress) {
    uint32_t colors[25] = {0};
    for (uint i = 0; i < progress && i < 25; i++) {
        colors[i] = 0x00FF00;
    }
    for (uint i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio0, 0, colors[i] << 8u);
    }
}

void set_rgb_led(uint8_t r, uint8_t g, uint8_t b) {
    gpio_put(LED_R, r);
    gpio_put(LED_G, g);
    gpio_put(LED_B, b);
}