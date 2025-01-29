# Projeto: Matriz de LEDs WS2812 com Raspberry Pi Pico

## Descrição
Este projeto implementa uma matriz de LEDs WS2812 utilizando a **Raspberry Pi Pico W**. O sistema permite a exibição de números de 0 a 9 na matriz de LEDs 5x5, com controle via botões físicos para incrementar e decrementar os valores.

## Autor
- **Kalel Ezveither**

## Funcionalidades
- Exibição de números de 0 a 9 em uma matriz de LEDs.
- Controle via botões físicos:
  - **Botão A**: Incrementa o número exibido.
  - **Botão B**: Decrementa o número exibido.
- LED vermelho piscando a cada 200ms como indicação de funcionamento.
- Uso de **PIO e DMA** para otimização da comunicação com os LEDs WS2812.
