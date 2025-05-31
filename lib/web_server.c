#include "./lib/web_server.h"

void user_request(char *html, size_t html_size) {
    snprintf(html, html_size,
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>"
    "body{font-family:sans-serif;text-align:center;background:#eee;margin:10px}"
    "form{margin:5px}input,button{padding:8px;margin:3px;font-size:1em;border:0;border-radius:5px;width:110px;height:75px}"
    "button{background:#29f;color:#fff}button.c{width:80px;height:80px;margin:2px}"

    ".img-container{width:300px;height:160px;margin:0 auto;overflow:hidden;position:relative}"

    ".img-slider{width:100%%;display:flex;justify-content:center;animation:slide 4s infinite alternate ease-in-out;}"

    "img{max-width:150px;height:auto;}"

    "@keyframes slide {"
        "0%% { transform: translateX(-50px); }"
        "100%% { transform: translateX(50px); }"
    "}"
    "</style></head><body>"

    "<div class='img-container'><div class='img-slider'>"
    "<img src='https://i.imgur.com/sxX771s.png' alt='Robo'>"
    "</div></div>"

    "<form method='GET' action='/move'>"  
    "<input name='a' placeholder='Setor' required>"
    "<input name='b' placeholder='Pos' required>"
    "<button style='background:#1e4760'>Mover</button></form>"

    "<form method='GET' action='/cvrp'><button style='background:#1e4760'>Entregas</button></form>"

    "<form method='GET' action='/manual'>"
    "<div><button class='c' name='dir' value='up'>C</button></div>"
    "<div><button class='c' name='dir' value='left'>E</button>"
    "<button class='c' name='dir' value='right'>D</button></div>"
    "<div><button class='c' name='dir' value='down'>B</button></div>"
    "</form>"

    "</body></html>");
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err){
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Função para inicializar o servidor TCP
void server_init(void) {
    //Inicializa a arquitetura do cyw43
    if (cyw43_arch_init()) {
        panic("Failed to initialize CYW43");
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 1);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI 
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("Failed to connect to WiFi");
    }

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default){
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server){
        panic("Failed to create TCP PCB\n");
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK){
        panic("Failed to bind TCP PCB\n");
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");
}
