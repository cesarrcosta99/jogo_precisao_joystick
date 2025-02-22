#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
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
uint8_t click_count = 0;
bool playing = true;
bool paused = false;
bool game_over = false;
bool victory = false;

ssd1306_t ssd;

// Variáveis para temporizadores e interrupções
volatile bool button_a_pressed = false;
volatile bool button_b_pressed = false;
volatile bool joystick_btn_pressed = false;
volatile bool sound_active = false;
volatile int led_blink_count = 0;
volatile bool led_state = false;
struct repeating_timer game_timer;
struct repeating_timer *blink_timer_ptr = NULL;

// Declarações de funções
void init_peripherals(void);
void read_joystick(void);
bool detect_loud_sound(void);
void play_sound(uint buzzer, uint freq);
int64_t stop_sound(alarm_id_t id, void *user_data);
void update_led_matrix(uint8_t progress);
void set_rgb_led(uint8_t r, uint8_t g, uint8_t b);
void show_victory_screen(void);
void show_game_over_screen(void);
void reset_game(void);
void update_display(void);
void maintain_final_screen(void);

// Callbacks para temporizadores
int64_t button_a_debounce_callback(alarm_id_t id, void *user_data) {
    button_a_pressed = false;
    paused = !paused; // Alterna o estado de pausa após o debounce
    return 0;
}

int64_t button_b_debounce_callback(alarm_id_t id, void *user_data) {
    button_b_pressed = false;
    reset_game();
    return 0;
}

int64_t joystick_btn_debounce_callback(alarm_id_t id, void *user_data) {
    joystick_btn_pressed = false;
    return 0;
}

int64_t reset_game_callback(alarm_id_t id, void *user_data) {
    reset_game();
    return 0; // Não repetir o alarme
}

// Callback de interrupção para botões
void button_handler(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A && events & GPIO_IRQ_EDGE_FALL && !button_a_pressed) {
        button_a_pressed = true;
        add_alarm_in_ms(200, button_a_debounce_callback, NULL, false);
    }
    if (gpio == BUTTON_B && events & GPIO_IRQ_EDGE_FALL && !button_b_pressed) {
        button_b_pressed = true;
        add_alarm_in_ms(200, button_b_debounce_callback, NULL, false);
    }
    if (gpio == JOYSTICK_BTN && events & GPIO_IRQ_EDGE_FALL && !joystick_btn_pressed) {
        joystick_btn_pressed = true;
        add_alarm_in_ms(50, joystick_btn_debounce_callback, NULL, false);
    }
}

// Callback para o temporizador principal
bool game_loop(struct repeating_timer *t) {
    if (game_over) {
        maintain_final_screen(); // Mantém a tela de Game Over/Vitória
        return true;
    }

    if (playing && !paused) {
        read_joystick();

        if (detect_loud_sound()) {
            // Som do microfone removido
        }

        if (joystick_btn_pressed) {
            click_count++;

            if (abs(cursor_x - target_x) < 3 && abs(cursor_y - target_y) < 3) {
                score++;
                play_sound(BUZZER1, 2000);
                target_x = (rand() % (WIDTH - 4)) + 2;
                target_y = (rand() % (HEIGHT - 4)) + 2;
                click_count = 0;
                update_led_matrix(score);
            }
        }

        if (click_count > 10) {
            game_over = true;
            victory = false;
            show_game_over_screen();
            add_alarm_in_ms(5000, reset_game_callback, NULL, false);
        }

        if (score >= 25) {
            update_led_matrix(score);
            game_over = true;
            victory = true;
            show_victory_screen();
            add_alarm_in_ms(5000, reset_game_callback, NULL, false);
        }

        update_display(); // Atualiza o display apenas durante o jogo ativo
        set_rgb_led(0, 1, 0);
    } else if (paused) {
        set_rgb_led(1, 1, 0);
    } else {
        set_rgb_led(1, 0, 0);
    }

    return true;
}

// Função para atualizar o display
void update_display(void) {
    ssd1306_fill(&ssd, false);
    char score_str[4];
    snprintf(score_str, sizeof(score_str), "%d", score);
    ssd1306_draw_string(&ssd, "Score:", 0, 0);
    int x_pos = 48;
    for (int i = 0; score_str[i] != '\0'; i++) {
        ssd1306_draw_char(&ssd, score_str[i], x_pos, 0);
        x_pos += 8;
    }

    char click_str[4];
    snprintf(click_str, sizeof(click_str), "%d", click_count);
    x_pos = 0;
    for (int i = 0; click_str[i] != '\0'; i++) {
        ssd1306_draw_char(&ssd, click_str[i], x_pos, 12);
        x_pos += 8;
    }

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int x = cursor_x + dx;
            int y = cursor_y + dy;
            if (x >= 1 && x < WIDTH-1 && y >= 1 && y < HEIGHT-1) {
                ssd1306_pixel(&ssd, x, y, true);
            }
        }
    }

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int x = target_x + dx;
            int y = target_y + dy;
            if (x >= 1 && x < WIDTH-1 && y >= 1 && y < HEIGHT-1) {
                ssd1306_pixel(&ssd, x, y, true);
            }
        }
    }

    ssd1306_send_data(&ssd);
}

// Função para manter a tela de Game Over/Vitória
void maintain_final_screen(void) {
    if (victory) {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "PARABENS!", 20, 20);
        ssd1306_draw_string(&ssd, "Score: 25", 30, 35);
        ssd1306_draw_string(&ssd, "VOCE COMPLETOU!", 5, 50);
        ssd1306_send_data(&ssd);
    } else {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "GAME OVER!", 20, 20);
        ssd1306_draw_string(&ssd, "Voce perdeu!", 20, 35);
        ssd1306_send_data(&ssd);
    }
}

// Callback para piscar LEDs
bool blink_led_callback(struct repeating_timer *t) {
    if (led_blink_count > 0) {
        set_rgb_led(led_state ? 1 : 0, led_state ? (victory ? 1 : 0) : 0, led_state ? (victory ? 1 : 0) : 0);
        led_state = !led_state;
        if (!led_state) led_blink_count--;
        return true;
    }
    cancel_repeating_timer(t);
    free(blink_timer_ptr);
    blink_timer_ptr = NULL;
    return false;
}

int main() {
    stdio_init_all();
    init_peripherals();

    // Configurar interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_handler);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true, &button_handler);

    // Configurar temporizador principal do jogo
    add_repeating_timer_ms(20, game_loop, NULL, &game_timer);

    while (1) {
        tight_loop_contents();
    }

    return 0;
}

void init_peripherals(void) {
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

void read_joystick(void) {
    adc_select_input(1);
    uint16_t x_val = adc_read();
    adc_select_input(0);
    uint16_t y_val = adc_read();
    
    cursor_x = (x_val * WIDTH) / 4096;
    cursor_y = ((4095 - y_val) * HEIGHT) / 4096;

    if (cursor_x >= WIDTH) cursor_x = WIDTH - 1;
    if (cursor_y >= HEIGHT) cursor_y = HEIGHT - 1;
}

bool detect_loud_sound(void) {
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
    sound_active = true;
    add_alarm_in_ms(200, stop_sound, (void *)(uint32_t)buzzer, false);
}

int64_t stop_sound(alarm_id_t id, void *user_data) {
    uint buzzer = (uint)(uint32_t)user_data;
    uint slice_num = pwm_gpio_to_slice_num(buzzer);
    pwm_set_enabled(slice_num, false);
    sound_active = false;
    return 0;
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

void show_victory_screen(void) {
    update_led_matrix(25);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "PARABENS!", 20, 20);
    ssd1306_draw_string(&ssd, "Score: 25", 30, 35);
    ssd1306_draw_string(&ssd, "VOCE COMPLETOU!", 5, 50);
    ssd1306_send_data(&ssd);
    
    play_sound(BUZZER1, 2500);
    
    if (blink_timer_ptr) {
        cancel_repeating_timer(blink_timer_ptr);
        free(blink_timer_ptr);
        blink_timer_ptr = NULL;
    }
    led_blink_count = 20; // 20 ciclos de 250ms (5 segundos)
    led_state = true;
    blink_timer_ptr = (struct repeating_timer *)malloc(sizeof(struct repeating_timer));
    add_repeating_timer_ms(250, blink_led_callback, NULL, blink_timer_ptr);
}

void show_game_over_screen(void) {
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "GAME OVER!", 20, 20);
    ssd1306_draw_string(&ssd, "Voce perdeu!", 20, 35);
    ssd1306_send_data(&ssd);
    
    play_sound(BUZZER2, 3000);
    
    if (blink_timer_ptr) {
        cancel_repeating_timer(blink_timer_ptr);
        free(blink_timer_ptr);
        blink_timer_ptr = NULL;
    }
    led_blink_count = 20; // 20 ciclos de 250ms (5 segundos)
    led_state = true;
    blink_timer_ptr = (struct repeating_timer *)malloc(sizeof(struct repeating_timer));
    add_repeating_timer_ms(250, blink_led_callback, NULL, blink_timer_ptr);
}

void reset_game(void) {
    score = 0;
    click_count = 0;
    game_over = false;
    victory = false;
    playing = true;
    paused = false;
    cursor_x = WIDTH/2;
    cursor_y = HEIGHT/2;
    target_x = (rand() % (WIDTH - 4)) + 2;
    target_y = (rand() % (HEIGHT - 4)) + 2;
    update_led_matrix(0);
    set_rgb_led(0, 1, 0);
    if (blink_timer_ptr) {
        cancel_repeating_timer(blink_timer_ptr);
        free(blink_timer_ptr);
        blink_timer_ptr = NULL;
    }
}