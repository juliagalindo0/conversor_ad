# Controle de Joystick e LEDs com RP2040

Este projeto implementa o controle de LEDs RGB e um display OLED SSD1306 utilizando um joystick analógico conectado ao microcontrolador RP2040 na placa BitDogLab. O joystick ajusta a intensidade dos LEDs via PWM e controla a posição de um quadrado móvel no display.

## 🛠 Funcionalidades

Leitura do Joystick: Obtém valores analógicos dos eixos X e Y.

Controle de LEDs RGB: Ajusta o brilho dos LEDs Azul e Vermelho com base na posição do joystick.

Exibição no Display: Mostra um quadrado 8x8 que se move conforme os valores do joystick.

Interrupções para botões:

Botão do joystick: Alterna o LED Verde e modifica a borda do display.

Botão A: Liga/desliga o controle de LEDs via PWM.

Implementação de debounce: Garante a estabilidade da leitura dos botões.

## 🎛 Hardware Utilizado

Microcontrolador: RP2040 (BitDogLab)

Joystick: Conectado às GPIOs 26 e 27 (Eixos X e Y), GPIO 22 (Botão)

LEDs RGB: Conectados às GPIOs 11, 12 e 13

Botão A: Conectado à GPIO 5

Display OLED SSD1306: Comunicação via I2C (GPIOs 14 e 15)

## 📜 Estrutura do Código

Inicialização dos periféricos (I2C, ADC, GPIOs e PWM)

Loop principal:

Leitura dos valores do joystick

Atualização dos LEDs e do display

Verificação dos botões e alternância de estados

