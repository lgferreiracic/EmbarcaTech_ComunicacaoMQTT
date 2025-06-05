# EmbarcaTech_ComunicaçãoMQTT
<p align="center">
  <img src="Group 658.png" alt="EmbarcaTech" width="300">
</p>

## Projeto: Simulação de AMR para Logística 4.0 com Webserver e Comunicação MQTT

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/-Raspberry_Pi-C51A4A?style=for-the-badge&logo=Raspberry-Pi)
![HTML](https://img.shields.io/badge/HTML-%23E34F26.svg?style=for-the-badge&logo=html5&logoColor=white)
![CSS](https://img.shields.io/badge/CSS-1572B6?style=for-the-badge&logo=css3&logoColor=fff)
![GitHub](https://img.shields.io/badge/github-%23121011.svg?style=for-the-badge&logo=github&logoColor=white)
![Windows 11](https://img.shields.io/badge/Windows%2011-%230079d5.svg?style=for-the-badge&logo=Windows%2011&logoColor=white)

## Descrição do Projeto

Este projeto é uma continuidade das fases anteriores, mantendo as funcionalidades já desenvolvidas, como o servidor web embarcado com interface responsiva, que permite o controle e o monitoramento remoto de um robô móvel autônomo (AMR) em um ambiente simulado de logística 4.0.

Nesta nova etapa, foi implementado um **cliente MQTT embarcado**, utilizando a plataforma **BitDogLab**, o **Pico SDK** e a **biblioteca LwIP**, com o objetivo de ampliar a capacidade de comunicação do sistema.

Com essa integração, o robô é capaz de se comunicar com um **broker MQTT**, publicando e assinando tópicos para o envio de comandos e recebimento de dados em tempo real. Isso torna o sistema mais adequado para aplicações em **ambientes industriais modernos** e **redes IoT**.

## Componentes Utilizados

- **Joystick (ADC nos eixos X e Y)**: Captura dos valores analógicos de movimentação.
- **Microcontrolador Raspberry Pi Pico W (RP2040)**: Controle central dos periféricos.
- **LED RGB**: Conectado às GPIOs 11, 12 e 13.
- **Botões A e B**: Conectados às GPIOs 5 e 6.
- **Botão do Joystick**: Conectado à GPIO 22.
- **Display SSD1306 (I2C)**: GPIOs 14 e 15.
- **Matriz de LEDs WS2812B**: GPIO 7.
- **Buzzers (1 e 2)**: GPIOs 10 e 21.
- **Potenciômetro do Joystick**: GPIO 26 (X) e GPIO 27 (Y).

## Ambiente de Desenvolvimento

- **VS Code** com extensão da Raspberry Pi.
- **Linguagem C**, utilizando o **Pico SDK**.
- **Biblioteca LwIP** para comunicação em rede.
- **HTML/CSS** para construção da interface web.
- **Broker MQTT (e.g., Mosquitto)** para testes locais ou em nuvem.

## Guia de Instalação

1. Clone o repositório.
2. Importe o projeto com a extensão da Raspberry Pi no VS Code.
3. Compile utilizando a extensão.
4. Para execução no dispositivo BitDogLab, envie o arquivo `.uf2` com o botão `bootsel` pressionado.
5. Inicie a simulação via ambiente de desenvolvimento.

## Guia de Uso

O projeto possui as seguintes funcionalidades:

### 🔧 Movimentação Manual
Controle do AMR via joystick ou interface web.
- Robô = LED vermelho
- Cargas = LED verde
- Paredes = LED azul
- Ponto de entrega = LED magenta

### 🤖 Navegação Automática
Movimentação inteligente até um destino usando algoritmos de busca.

### 📦 Entregas Automáticas (CVRP)
Uso de algoritmos de roteamento com capacidade para entrega de múltiplas cargas.

### 📊 Monitoramento em Tempo Real
Display OLED exibe:
- Setor atual
- Capacidade de carga
- Cargas restantes

### 🌐 Comunicação MQTT
- Publicação de comandos e eventos do robô.
- Assinatura de tópicos com atualizações em tempo real.
- Permite integração com plataformas externas, dashboards ou sistemas supervisórios.

## Testes

- Leitura analógica do joystick e resposta dos LEDs.
- Alerta via buzzer e LED RGB para colisões.
- Validação da renderização na matriz de LEDs.
- Simulação de movimentação automática e lógica de entregas.
- Verificação do envio e recebimento MQTT com ferramentas como MQTT Explorer.

## Desenvolvedor

[Lucas Gabriel Ferreira](https://github.com/usuario-lider)

## Vídeo da Solução

[Link do YouTube](https://www.youtube.com/watch?v=O2jbdGXdGoE)
