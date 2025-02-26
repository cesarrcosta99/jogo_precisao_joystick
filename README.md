# Jogo de Precisão com Joystick e Feedback Multimodal

## Descrição

Este projeto consiste em um jogo interativo desenvolvido para a placa BitDoLab, equipada com o microcontrolador RP2040. O sistema permite que o usuário controle um cursor em um display OLED por meio de um joystick analógico, registrando acertos em um alvo e atualizando, em tempo real, a pontuação e a contagem de cliques. Além disso, o jogo oferece feedback multimodal, combinando respostas visuais e sonoras para criar uma experiência intuitiva e envolvente.

## Funcionalidades

- **Movimentação Precisa:**  
  Controle do cursor no display OLED utilizando o joystick analógico.

- **Registro de Acertos:**  
  Detecção dos acertos no alvo, com atualização imediata da pontuação e contagem de cliques.

- **Feedback Multimodal:**  
  - **Visual:** Atualização do display OLED, LED RGB e matriz de LEDs WS2812.  
  - **Sonoro:** Emissão de sinais pelos buzzers para indicar eventos do jogo.

- **Gerenciamento de Estados:**  
  Controle dos estados do jogo (jogando, pausado, vitória e derrota) com reinicialização automática após a conclusão de uma partida.

## Requisitos de Hardware

- **Placa:** BitDoLab com microcontrolador RP2040  
- **Entrada:** Joystick analógico (conectado via ADC e GPIO)  
- **Saídas:**  
  - Display OLED (via I2C, endereço 0x3C)  
  - Matriz de LEDs WS2812 (via PIO)  
  - LED RGB (via GPIO)  
  - Buzzers (via PWM)  
  - Botões para pausar e reiniciar (via GPIO)

## Ambiente de Desenvolvimento

O projeto foi desenvolvido utilizando o Visual Studio Code (VS Code) como IDE principal. Inicialmente, instalou-se a extensão **Raspberry Pi Pico Project**, que automatiza a criação do ambiente de compilação baseado em CMake, integrando o SDK oficial do RP2040 e as bibliotecas essenciais, como:
- pico/stdlib.h
- hardware/gpio.h
- hardware/adc.h
- hardware/i2c.h
- hardware/pio.h
- hardware/timer.h

Além disso, o Wokwi foi utilizado como ferramenta de simulação online para realizar testes preliminares e ajustes no código antes da programação física.

## Compilação e Programação

1. **Configuração do Ambiente:**  
   - Instalar o VS Code e a extensão Raspberry Pi Pico Project.  
   - A extensão configura automaticamente o SDK oficial do RP2040 e cria um ambiente baseado em CMake.

2. **Compilação:**  
   - O código, escrito em C e organizado de forma modular, é compilado utilizando as ferramentas CMake e Make integradas à extensão, gerando um binário compatível com o RP2040.

3. **Programação:**  
   - Conectar a placa BitDoLab via USB e carregar o código compilado diretamente no dispositivo através das ferramentas integradas.

## Testes e Validação

Foram realizados testes abrangentes para garantir o funcionamento correto do sistema, tanto em ambiente simulado quanto na placa física:

- **Simulação no Wokwi:**  
  - Validação da lógica do jogo, detecção de entradas do joystick e resposta dos botões com mecanismos de debounce.
  - Ajustes na atualização do display OLED e nos feedbacks visuais e sonoros.

- **Testes na Placa Física:**  
  - Monitoramento via comunicação serial (usando funções como `stdio_init_all()` e `printf`) para acompanhar os valores do joystick, estados dos botões e progresso do jogo em tempo real.
  - Verificação do comportamento dos periféricos (display, LEDs, buzzers) para assegurar a estabilidade e a integração adequada entre hardware e software.

## Uso

Após a programação, basta conectar a placa BitDoLab via USB e ligar o sistema. Utilize o joystick para mover o cursor no display OLED e pressione o botão do joystick para registrar acertos. O sistema responderá com feedback visual e sonoro, atualizando a pontuação e controlando os diferentes estados do jogo.

## Conclusão

O "Jogo de Precisão com Joystick e Feedback Multimodal" demonstra uma integração eficiente entre hardware e software, oferecendo uma experiência interativa de alta qualidade. O projeto foi desenvolvido com base em pesquisas aprofundadas, escolha criteriosa de componentes e rigorosos testes de validação, garantindo desempenho, estabilidade e confiabilidade em sua execução.

# PROJETO DESENVOLVIDO POR CÉSAR REBOUÇAS COSTA
