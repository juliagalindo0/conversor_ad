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
  gpio_set_function(pino, GPIO_FUNC_PWM);  // Configura o pino como saída PWM
  uint canal_pwm = pwm_gpio_to_slice_num(pino); // Obtém o canal PWM correspondente
  pwm_set_wrap(canal_pwm, 4095); // Define o contador máximo PWM
  pwm_set_chan_level(canal_pwm, pwm_gpio_to_channel(pino), 0); // Inicia com 0% de duty cycle
  pwm_set_enabled(canal_pwm, true); // Ativa o PWM
}

int main()
{
  // Configuração dos botões e LEDs
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
  gpio_put(LUZ_VERDE, 0); // Começa apagado

  gpio_init(LUZ_VERMELHA);
  gpio_set_dir(LUZ_VERMELHA, GPIO_OUT);

  // Inicialização do I2C com frequência de 400Khz
  i2c_init(PORTA_I2C, 400 * 1000);

  gpio_set_function(PINO_SDA, GPIO_FUNC_I2C); // Configuração do pino SDA
  gpio_set_function(PINO_SCL, GPIO_FUNC_I2C); // Configuração do pino SCL
  gpio_pull_up(PINO_SDA); // Ativa pull-up
  gpio_pull_up(PINO_SCL); // Ativa pull-up
  
  ssd1306_t tela; // Estrutura de controle do display
  ssd1306_init(&tela, WIDTH, HEIGHT, false, ENDERECO_DISPLAY, PORTA_I2C); // Inicializa display
  ssd1306_config(&tela);
  ssd1306_send_data(&tela);

  // Limpa a tela
  ssd1306_fill(&tela, false);
  ssd1306_send_data(&tela);

  // Configuração do ADC
  adc_init();
  adc_gpio_init(EIXO_X);
  adc_gpio_init(EIXO_Y); 
  adc_select_input(1);
  
  // Configuração dos LEDs como PWM
  inicializar_pwm(LUZ_AZUL);
  inicializar_pwm(LUZ_VERMELHA);

  uint canal_azul = pwm_gpio_to_slice_num(LUZ_AZUL);
  uint canal_vermelho = pwm_gpio_to_slice_num(LUZ_VERMELHA);
  
  uint16_t leitura_x;
  uint16_t leitura_y;  
  char buffer_x[5];  // Armazena string do valor X
  char buffer_y[5];  // Armazena string do valor Y  
  
  // Variáveis de controle do quadrado
  int pos_x = 60, pos_y = 28;
  bool estado_verde = false;
  bool pwm_ligado = true;
  int tipo_borda = 0;
  bool botao_joystick_ativado = false;
  bool botao_extra_ativado = false;

  while (true)
  {
    // Leitura do joystick
    adc_select_input(0); // Lê o eixo X
    leitura_x = adc_read();
    adc_select_input(1); // Lê o eixo Y
    leitura_y = adc_read(); 

    // Conversão dos valores do ADC para posição do quadrado
    pos_x = (leitura_x * 112) / 4095 + 8;
    pos_y = (leitura_y * 48) / 4095 + 8;

    // Ajuste do brilho dos LEDs conforme os valores do ADC
    uint16_t intensidade_x = abs(2048 - leitura_x) * 2;
    uint16_t intensidade_y = abs(2048 - leitura_y) * 2;

    if (pwm_ligado) {
      pwm_set_chan_level(canal_vermelho, pwm_gpio_to_channel(LUZ_VERMELHA), intensidade_x);
      pwm_set_chan_level(canal_azul, pwm_gpio_to_channel(LUZ_AZUL), intensidade_y);
    } else {
      pwm_set_chan_level(canal_vermelho, pwm_gpio_to_channel(LUZ_VERMELHA), 0);
      pwm_set_chan_level(canal_azul, pwm_gpio_to_channel(LUZ_AZUL), 0);
    }

    // Controle do botão do joystick
    if (gpio_get(BOTAO_JOYSTICK) == 0 && !botao_joystick_ativado) {
      botao_joystick_ativado = true;
      estado_verde = !estado_verde;
      gpio_put(LUZ_VERDE, estado_verde);
      tipo_borda = (tipo_borda + 1) % 3;
    }
    if (gpio_get(BOTAO_JOYSTICK) == 1) {
      botao_joystick_ativado = false;
    }

    // Controle do botão auxiliar
    if (gpio_get(BOTAO_EXTRA) == 0 && !botao_extra_ativado) {
      botao_extra_ativado = true;
      pwm_ligado = !pwm_ligado;
    }
    if (gpio_get(BOTAO_EXTRA) == 1) {
      botao_extra_ativado = false;
    }

    // Atualização do display
    ssd1306_fill(&tela, false);

    // Desenho da borda conforme o estado atual
    if (tipo_borda == 0) {
      ssd1306_rect(&tela, 3, 3, 122, 60, true, false);
    } else if (tipo_borda == 1) {
      ssd1306_line(&tela, 3, 3, 122, 3, true);
      ssd1306_line(&tela, 3, 60, 122, 60, true);
    } else {
      ssd1306_rect(&tela, 3, 3, 122, 60, true, true);
    }

    // Desenha o quadrado móvel
    ssd1306_rect(&tela, pos_x, pos_y, 8, 8, true, true);
    
    // Exibe os valores do joystick
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
