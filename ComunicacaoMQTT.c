#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"   
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "pico/cyw43_arch.h"    
#include "pico/unique_id.h"         // Biblioteca com recursos para trabalhar com os pinos GPIO do Raspberry Pi Pico
#include "lwip/pbuf.h"           
#include "lwip/tcp.h"            
#include "lwip/netif.h"  
#include "lwip/apps/mqtt.h"         // Biblioteca LWIP MQTT -  fornece funções e recursos para conexão MQTT
#include "lwip/apps/mqtt_priv.h"    // Biblioteca que fornece funções e recursos para Geração de Conexões
#include "lwip/dns.h"               // Biblioteca que fornece funções e recursos suporte DNS:
#include "lwip/altcp_tls.h"         // Biblioteca que fornece funções e recursos para conexões seguras usando TLS:

#include "./lib/button.h"
#include "./lib/buzzer.h"
#include "./lib/joystick.h"
#include "./lib/led_rgb.h"
#include "./lib/matrix.h"
#include "./lib/factory.h"
#include "./lib/web_server.h"
#include "./lib/mqtt_client.h"
#include "./lib/temperature.h" // Include the temperature sensor library 

ssd1306_t ssd; // Variável para o display OLED SSD1306
volatile uint32_t last_time_button_a = 0; // Variável para debounce do botão A
volatile uint32_t last_time_button_b = 0; // Variável para debounce do botão B
volatile uint32_t last_time_joystick_button = 0; // Variável para debounce do botão do joystick
volatile int option_selected = 1; // Variável para armazenar a opção selecionada
uint16_t joystick_x, joystick_y; // Variáveis para armazenar a leitura do joystick em relação aos eixos x e y
uint8_t sector = 0; // Variável para armazenar o setor atual
Robot objectives[NUM_LOADS]; // Vetor para armazenar os objetivos do robô
int distances[NUM_LOADS]; // Vetor para armazenar as distâncias entre o robô e os objetivos
bool delivered[NUM_LOADS] = {false, false, false, false, false}; // Vetor para armazenar se os objetivos foram entregues
char html[2048]; // Buffer para armazenar a resposta HTML
Robot destination; // Variável para armazenar o destino do robô
static MQTT_CLIENT_DATA_T state;
static char client_id_buf[sizeof(MQTT_DEVICE_NAME) + 4]; // 4 chars + '\0'

// Estrutura responsável por armazenar a fábrica e o robô
Factory factory = {
    .sectors[0] = {
        2, 2, 2, 2, 2,
        2, 1, 0, 0, 2,
        2, 0, 0, 0, 0,
        2, 0, 0, 0, 2,
        2, 2, 0, 2, 2
    },
    .sectors[1] = {
        2, 2, 5, 2, 2,
        2, 0, 0, 0, 2,
        0, 0, 0, 0, 0,
        2, 0, 0, 0, 2,
        2, 2, 0, 2, 2
    },
    .sectors[2] = {
        2, 2, 2, 2, 2,
        2, 0, 0, 0, 2,
        0, 0, 0, 0, 2,
        2, 0, 0, 0, 2,
        2, 2, 0, 2, 2
    },
    .sectors[3] = {
        2, 2, 0, 2, 2,
        2, 0, 0, 0, 2,
        2, 0, 0, 0, 0,
        2, 0, 0, 0, 2,
        2, 2, 0, 2, 2
    },
    .sectors[4] = {
        2, 2, 0, 2, 2,
        2, 0, 0, 0, 2,
        0, 0, 0, 0, 0,
        2, 0, 0, 0, 2,
        2, 2, 0, 2, 2
    },
    .sectors[5] = {
        2, 2, 0, 2, 2,
        2, 0, 0, 0, 2,
        0, 0, 0, 0, 2,
        2, 0, 0, 0, 2,
        2, 2, 0, 2, 2
    },
    .sectors[6] = {
        2, 2, 0, 2, 2,
        2, 0, 0, 0, 2,
        2, 0, 0, 0, 0,
        2, 0, 0, 0, 2,
        2, 2, 2, 2, 2
    },
    .sectors[7] = {
        2, 2, 0, 2, 2,
        2, 0, 0, 0, 2,
        0, 0, 0, 0, 0,
        2, 0, 0, 0, 2,
        2, 2, 4, 2, 2
    },
    .sectors[8] = {
        2, 2, 0, 2, 2,
        2, 0, 0, 0, 2,
        0, 0, 0, 0, 2,
        2, 0, 0, 0, 2,
        2, 2, 2, 2, 2
    },
    .robot = {
        .position = {
            .x = 1,
            .y = 1
        },
        .sector = 0,
        .charge = 100 // Nível de carga inicial do robô
    }
};

// Função para gerenciar a interrupção dos botões
void irq_handler(uint gpio, uint32_t events){
    if (gpio == BUTTON_A_PIN && debounce(&last_time_button_a)){
        clear_matrix();
        clear_display(&ssd);
        reset_usb_boot(0,0);
    }
    else if (gpio == BUTTON_B_PIN && debounce(&last_time_button_b)){
        option_selected = 1;
        clear_matrix();
        randomize_objectives(objectives, &factory);
        reset_delivered(delivered);
        for(int i = 0; i < NUM_SECTORS; i++){
            for(int j = 0; j < NUM_PIXELS; j++){
                if(factory.sectors[i][j] == 3){
                    factory.sectors[i][j] = 0;
                }
            }
        }
    }
}

// Função de callback para receber dados TCP
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    char *request = (char *)p->payload;
    int a = 0, b = 0;

    if (strncmp(request, "GET /cvrp", 9) == 0) { // Verifica se é a requisição CVRP
        printf("Botão CVRP pressionado!\n");
        reset_delivered(delivered);
        option_selected = 2;
        user_request(html, sizeof(html));
    }else if (strstr(request, "GET /move?a=")) { // Verifica se é a requisição de movimentação
        sscanf(request, "GET /move?a=%d&b=%d", &a, &b);
        printf("Parâmetros recebidos: Setor = %d, Posicao = %d\n", a, b);
        destination.sector = a - 1;
        destination.position.x = b / 5;
        destination.position.y = b % 5;
        option_selected = 3;
        user_request(html, sizeof(html));
    }else if(strncmp(request, "GET /manual?dir=up", 18) == 0){ // Verifica se é a requisição é a de movimentação manual para cima
        manual_mode_movimentation(&factory, &sector, 2048, 3500, &ssd, delivered, objectives);
    }else if(strncmp(request, "GET /manual?dir=down", 20) == 0){ // Verifica se é a requisição é a de movimentação manual para baixo
        manual_mode_movimentation(&factory, &sector, 2048, 500, &ssd, delivered, objectives);
    }else if(strncmp(request, "GET /manual?dir=left", 20) == 0){ // Verifica se é a requisição é a de movimentação manual para esquerda
        manual_mode_movimentation(&factory, &sector, 500, 2048, &ssd, delivered, objectives);
    }else if(strncmp(request, "GET /manual?dir=right", 21) == 0){ // Verifica se é a requisição é a de movimentação manual para direita
        manual_mode_movimentation(&factory, &sector, 3500, 2048, &ssd, delivered, objectives);
    }else{
        user_request(html, sizeof(html));
    }
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    pbuf_free(p);
    return ERR_OK;
}

// Função para inicializar todo o hardware
void setup(){
    stdio_init_all(); // Inicializa a comunicação padrão
    buzzer_init_all(); // Inicializa todos os buzzers
    matrix_init(); // Inicializa a matriz de LEDs RGB
    joystick_init(); // Inicializa o joystick
    led_init_all(); // Inicializa todos os LEDs
    button_init_all(); // Inicializa todos os botões
    display_init(&ssd); // Inicializa o display OLED SSD1306
    start_display(&ssd); // Inicia o display
    server_init(); // Inicializa o servidor TCP
    temperature_sensor_init(); // Inicializa o sensor de temperatura
    generate_client_id(client_id_buf, sizeof(client_id_buf)); // Gera o ID do cliente MQTT
    configure_mqtt_client(&state, client_id_buf); // Configura o cliente MQTT
    resolve_and_connect_mqtt(&state); // Resolve o endereço do servidor MQTT e conecta
    state.factory = &factory; // Atribui a fábrica ao estado do cliente MQTT

    // Configuração dos pinos de interrupção
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &irq_handler); // Configura interrupção para o botão A
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &irq_handler); // Configura interrupção para o botão B

}

bool check_battery(struct repeating_timer *t) {
    if (factory.robot.charge > 5) {
        factory.robot.charge-=5; // Consome 5 unidades de carga a cada 10 segundos
        printf("Nivel de carga do robo: %d%%\n", factory.robot.charge); // Exibe o nível de carga do robô
    } 
    return true; // Retorna verdadeiro para continuar o timer
}

int main(){
    struct repeating_timer timer;
    setup(); // Inicializa o hardware e configurações
    randomize_objectives(objectives, &factory); // Posiciona os objetivos aleatoriamente
    reset_delivered(delivered); // Reinicializa o vetor que responsável por armazenar os objetivos entregues
    manual_mode_display(&ssd); // Exibe a tela de movimentação manual
    add_repeating_timer_ms(10000, check_battery, NULL, &timer);
    while (verify_mqtt(&state)) {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000)); // Espera por trabalho até 10 segundos

        switch(option_selected){
            case 1: // Modo manual
                reading_joystick(&joystick_x, &joystick_y); // Leitura do joystick
                manual_mode_movimentation(&factory, &sector, joystick_x, joystick_y, &ssd, delivered, objectives); // Movimentação manual
                break;
            case 2: // Modo CVRP
                clear_display(&ssd); // Limpa o display
                clear_matrix(); // Limpa a matriz de LEDs RGB1
                solve_capacitated_vrp(&factory, objectives, distances, delivered, &sector, &ssd); // Resolve o problema do CVRP
                manual_mode_display(&ssd); // Exibe a tela de movimentação manual 
                option_selected = 1; // Retorna para o modo manual
                break;
            case 3: // Modo de movimentação automática
                clear_display(&ssd); // Limpa o display
                clear_matrix(); // Limpa a matriz de LEDs RGB
                find_path(destination, &factory, &sector, delivered, objectives, &ssd); // Encontra o caminho e movimenta o robô

                if(factory.robot.charge > 10)
                    factory.robot.charge -= 10; // Consome 10% da carga do robô
                else
                    factory.robot.charge = 5; // Carga mínima para ainda ser possível caminhar até a estação de carregamento
                    
                manual_mode_display(&ssd); // Exibe a tela de movimentação manual
                option_selected = 1; // Retorna para o modo manual
                break;
            default:
                break;
        }
        sleep_ms(200);
        if(factory.robot.charge <= 10){ 
            destination.position.x = 0;
            destination.position.y = 2;
            destination.sector = 1; 
            clear_display(&ssd); // Limpa o display
            clear_matrix(); // Limpa a matriz de LEDs RGB
            find_path(destination, &factory, &sector, delivered, objectives, &ssd); // Encontra o caminho e movimenta o robô
            factory.robot.charge = 100; // Recarrega a bateria do robô
            play_charging_sound(); // Toca o som de sucesso
            printf("Bateria recarregada!\n"); // Exibe mensagem de recarga
            manual_mode_display(&ssd); // Exibe a tela de movimentação manual
            option_selected = 1; // Retorna para o modo manual
        }
    }
    cyw43_arch_deinit(); // Desativa o CYW43
    return 0;
}

