#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "pico/stdlib.h"            // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "pico/cyw43_arch.h"        // Biblioteca para arquitetura Wi-Fi da Pico com CYW43
#include "pico/unique_id.h"         // Biblioteca com recursos para trabalhar com os pinos GPIO do Raspberry Pi Pico

#include "lwip/apps/mqtt.h"         // Biblioteca LWIP MQTT -  fornece funções e recursos para conexão MQTT
#include "lwip/apps/mqtt_priv.h"    // Biblioteca que fornece funções e recursos para Geração de Conexões
#include "lwip/dns.h"               // Biblioteca que fornece funções e recursos suporte DNS:
#include "lwip/altcp_tls.h"         // Biblioteca que fornece funções e recursos para conexões seguras usando TLS:

#include "./lib/factory.h"           // Biblioteca que define a estrutura da fábrica e do robô

#define WIFI_SSID "Residencia"                  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASSWORD "embarcatech"      // Substitua pela senha da sua rede Wi-Fi
#define MQTT_SERVER "192.168.1.107"                // Substitua pelo endereço do host - broket MQTT: Ex: 192.168.1.107
#define MQTT_USERNAME "embarcatech"     // Substitua pelo nome da host MQTT - Username
#define MQTT_PASSWORD "embarcatech"     // Substitua pelo Password da host MQTT - credencial de acesso - caso exista

#ifndef MQTT_SERVER
#error Need to define MQTT_SERVER
#endif

// This file includes your client certificate for client server authentication
#ifdef MQTT_CERT_INC
#include MQTT_CERT_INC
#endif

#ifndef MQTT_TOPIC_LEN
#define MQTT_TOPIC_LEN 100
#endif

//Dados do cliente MQTT
typedef struct {
    mqtt_client_t* mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[MQTT_TOPIC_LEN];
    uint32_t len;
    ip_addr_t mqtt_server_address;
    bool connect_done;
    int subscribe_count;
    bool stop_client;
    Factory *factory; // Estrutura que armazena a fábrica e o robô
    ssd1306_t *ssd; // Display OLED SSD1306
    bool *delivered; // Vetor que armazena se os objetivos foram entregues
    Robot *objectives; // Vetor que armazena os objetivos do robô
    int *distances; // Vetor que armazena as distâncias entre o robô e os objetivos
    uint8_t *sector; // Setor atual do robô
} MQTT_CLIENT_DATA_T;

#ifndef DEBUG_printf
#ifndef NDEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif
#endif

#ifndef INFO_printf
#define INFO_printf printf
#endif

#ifndef ERROR_printf
#define ERROR_printf printf
#endif

// Temporização da coleta de temperatura - how often to measure our temperature
#define TEMP_WORKER_TIME_S 10

// Temporização da coleta de carga da bateria - how often to measure our battery charge
#define CHARGE_WORKER_TIME_S 10

// Temporização da coleta do setor - how often to measure our sector
#define SECTOR_WORKER_TIME_S 10

// Manter o programa ativo - keep alive in seconds
#define MQTT_KEEP_ALIVE_S 60

// QoS - mqtt_subscribe
// At most once (QoS 0)
// At least once (QoS 1)
// Exactly once (QoS 2)
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 1
#define MQTT_PUBLISH_RETAIN 0

// Tópico usado para: last will and testament
#define MQTT_WILL_TOPIC "/online"
#define MQTT_WILL_MSG "0"
#define MQTT_WILL_QOS 1

#ifndef MQTT_DEVICE_NAME
#define MQTT_DEVICE_NAME "pico"
#endif

// Definir como 1 para adicionar o nome do cliente aos tópicos, para suportar vários dispositivos que utilizam o mesmo servidor
#ifndef MQTT_UNIQUE_TOPIC
#define MQTT_UNIQUE_TOPIC 0
#endif

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
#include "./lib/temperature.h" // Include the temperature sensor library

// Requisição para publicar
void pub_request_cb(__unused void *arg, err_t err);

// Topico MQTT
const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name);

// Controle do LED 
void control_led(MQTT_CLIENT_DATA_T *state, bool on);

// Publicar temperatura
void publish_temperature(MQTT_CLIENT_DATA_T *state);

// Requisição de Assinatura - subscribe
void sub_request_cb(void *arg, err_t err);

// Requisição para encerrar a assinatura
void unsub_request_cb(void *arg, err_t err);

// Tópicos de assinatura
void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub);

// Dados de entrada MQTT
void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

// Dados de entrada publicados
void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);

// Publicar temperatura
void temperature_worker_fn(async_context_t *context, async_at_time_worker_t *worker);

// Conexão MQTT
void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);

// Inicializar o cliente MQTT
void start_client(MQTT_CLIENT_DATA_T *state);

// Call back com o resultado do DNS
void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

// Gerar ID do cliente
void generate_client_id(char *client_id_buf, size_t buf_size);

// Configurar o cliente MQTT
void configure_mqtt_client(MQTT_CLIENT_DATA_T *state, const char *client_id_buf);

// Callback function for mqtt connection status
void resolve_and_connect_mqtt(MQTT_CLIENT_DATA_T *state);

// Verifica se o cliente MQTT está ativo 
bool verify_mqtt(MQTT_CLIENT_DATA_T *state);

#endif