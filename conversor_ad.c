#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define PORTA_I2C i2c1
#define PINO_SDA 14
#define PINO_SCL 15
#define ENDERECO_DISPLAY 0x3C
#define EIXO_X 26  // GPIO para leitura do eixo X
#define EIXO_Y 27  // GPIO para leitura do eixo Y
#define BOTAO_JOYSTICK 22 // GPIO do botão do joystick
#define BOTAO_EXTRA 5 // GPIO do botão auxiliar
#define LUZ_VERMELHA 13
#define LUZ_AZUL 12
#define LUZ_VERDE 11

void inicializar_pwm(uint pino) {
    gpio_set_function(pino, GPIO_FUNC_PWM);
    uint canal_pwm = pwm_gpio_to_slice_num(pino);
    pwm_set_wrap(canal_pwm, 4095);
    pwm_set_chan_level(canal_pwm, pwm_gpio_to_channel(pino), 0);
    pwm_set_enabled(canal_pwm, true);
}

bool debounce(uint pino) {
    static uint32_t ultimo_tempo = 0;
    const uint32_t intervalo_debounce = 50;
    if (gpio_get(pino) == 0) {
        if (to_ms_since_boot(get_absolute_time()) - ultimo_tempo > intervalo_debounce) {
            ultimo_tempo = to_ms_since_boot(get_absolute_time());
            return true;
        }
    }
    return false;
}

int main() {
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);

    gpio_init(BOTAO_EXTRA);
    gpio_set_dir(BOTAO_EXTRA, GPIO_IN);
    gpio_pull_up(BOTAO_EXTRA);

    gpio_init(LUZ_AZUL);
    gpio_set_dir(LUZ_AZUL, GPIO_OUT);

    gpio_init(LUZ_VERDE);
    gpio_set_dir(LUZ_VERDE, GPIO_OUT);
    gpio_put(LUZ_VERDE, 0);

    gpio_init(LUZ_VERMELHA);
    gpio_set_dir(LUZ_VERMELHA, GPIO_OUT);

    i2c_init(PORTA_I2C, 400 * 1000);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SDA);
    gpio_pull_up(PINO_SCL);

    ssd1306_t tela;
    ssd1306_init(&tela, WIDTH, HEIGHT, false, ENDERECO_DISPLAY, PORTA_I2C);
    ssd1306_config(&tela);
    ssd1306_send_data(&tela);

    ssd1306_fill(&tela, false);
    ssd1306_send_data(&tela);

    adc_init();
    adc_gpio_init(EIXO_X);
    adc_gpio_init(EIXO_Y);
    adc_select_input(1);

    inicializar_pwm(LUZ_AZUL);
    inicializar_pwm(LUZ_VERMELHA);

    uint canal_azul = pwm_gpio_to_slice_num(LUZ_AZUL);
    uint canal_vermelho = pwm_gpio_to_slice_num(LUZ_VERMELHA);

    uint16_t leitura_x, leitura_y;
    int pos_x = 60, pos_y = 28;
    bool estado_verde = false;
    bool pwm_ligado = true;
    int tipo_borda = 0;
    bool botao_joystick_ativado = false;
    bool botao_extra_ativado = false;

    while (true) {
        adc_select_input(0);
        leitura_x = adc_read();
        adc_select_input(1);
        leitura_y = adc_read();

        pos_x = (leitura_x * 112) / 4095 + 8;
        pos_y = (leitura_y * 48) / 4095 + 8;

        uint16_t intensidade_x = abs(2048 - leitura_x) * 2;
        uint16_t intensidade_y = abs(2048 - leitura_y) * 2;

        if (pwm_ligado) {
            pwm_set_chan_level(canal_vermelho, pwm_gpio_to_channel(LUZ_VERMELHA), intensidade_x);
            pwm_set_chan_level(canal_azul, pwm_gpio_to_channel(LUZ_AZUL), intensidade_y);
        } else {
            pwm_set_chan_level(canal_vermelho, pwm_gpio_to_channel(LUZ_VERMELHA), 0);
            pwm_set_chan_level(canal_azul, pwm_gpio_to_channel(LUZ_AZUL), 0);
        }

        if (debounce(BOTAO_JOYSTICK) && !botao_joystick_ativado) {
            botao_joystick_ativado = true;
            estado_verde = !estado_verde;
            gpio_put(LUZ_VERDE, estado_verde);
            tipo_borda = (tipo_borda + 1) % 3;
        }
        if (gpio_get(BOTAO_JOYSTICK) == 1) {
            botao_joystick_ativado = false;
        }

        if (debounce(BOTAO_EXTRA) && !botao_extra_ativado) {
            botao_extra_ativado = true;
            pwm_ligado = !pwm_ligado;
        }
        if (gpio_get(BOTAO_EXTRA) == 1) {
            botao_extra_ativado = false;
        }

        ssd1306_fill(&tela, false);

        if (tipo_borda == 0) {
            ssd1306_rect(&tela, 3, 3, 122, 60, true, false);
        } else if (tipo_borda == 1) {
            ssd1306_line(&tela, 3, 3, 122, 3, true);
            ssd1306_line(&tela, 3, 60, 122, 60, true);
        } else {
            ssd1306_rect(&tela, 3, 3, 122, 60, true, true);
        }

        ssd1306_rect(&tela, pos_x, pos_y, 8, 8, true, true);

        char buffer_x[6], buffer_y[6];
        sprintf(buffer_x, "%d", leitura_x);
        sprintf(buffer_y, "%d", leitura_y);
        ssd1306_draw_string(&tela, "X:", 10, 30);
        ssd1306_draw_string(&tela, buffer_x, 30, 30);
        ssd1306_draw_string(&tela, "Y:", 10, 45);
        ssd1306_draw_string(&tela, buffer_y, 30, 45);

        ssd1306_send_data(&tela);
        sleep_ms(50);
    }
}
