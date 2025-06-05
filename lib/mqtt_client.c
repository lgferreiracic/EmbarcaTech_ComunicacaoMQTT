#include "./lib/mqtt_client.h"

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */

// Requisição para publicar
void pub_request_cb(__unused void *arg, err_t err) {
    if (err != 0) {
        ERROR_printf("pub_request_cb failed %d", err);
    }
}

//Topico MQTT
const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name) {
#if MQTT_UNIQUE_TOPIC
    static char full_topic[MQTT_TOPIC_LEN];
    snprintf(full_topic, sizeof(full_topic), "/%s%s", state->mqtt_client_info.client_id, name);
    return full_topic;
#else
    return name;
#endif
}

// Controle do LED 
void control_led(MQTT_CLIENT_DATA_T *state, bool on) {
    // Publish state on /state topic and on/off led board
    const char* message = on ? "On" : "Off";
    if (on)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    else
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/led/state"), message, strlen(message), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
}

// Requisição de Assinatura - subscribe
void sub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        panic("subscribe request failed %d", err);
    }
    state->subscribe_count++;
}

// Requisição para encerrar a assinatura
void unsub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        panic("unsubscribe request failed %d", err);
    }
    state->subscribe_count--;
    assert(state->subscribe_count >= 0);

    // Stop if requested
    if (state->subscribe_count <= 0 && state->stop_client) {
        mqtt_disconnect(state->mqtt_client_inst);
    }
}

// Tópicos de assinatura
void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub) {
    mqtt_request_cb_t cb = sub ? sub_request_cb : unsub_request_cb;
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/led"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/print"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/ping"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/exit"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/up"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/down"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/left"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/right"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/cvrp"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
}

// Dados de entrada MQTT
void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
#if MQTT_UNIQUE_TOPIC
    const char *basic_topic = state->topic + strlen(state->mqtt_client_info.client_id) + 1;
#else
    const char *basic_topic = state->topic;
#endif
    strncpy(state->data, (const char *)data, len);
    state->len = len;
    state->data[len] = '\0';

    DEBUG_printf("Topic: %s, Message: %s\n", state->topic, state->data);
    if (strcmp(basic_topic, "/led") == 0)
    {
        if (lwip_stricmp((const char *)state->data, "On") == 0 || strcmp((const char *)state->data, "1") == 0)
            control_led(state, true);
        else if (lwip_stricmp((const char *)state->data, "Off") == 0 || strcmp((const char *)state->data, "0") == 0)
            control_led(state, false);
    } else if (strcmp(basic_topic, "/print") == 0) {
        INFO_printf("%.*s\n", len, data);
    } else if (strcmp(basic_topic, "/ping") == 0) {
        char buf[11];
        snprintf(buf, sizeof(buf), "%u", to_ms_since_boot(get_absolute_time()) / 1000);
        mqtt_publish(state->mqtt_client_inst, full_topic(state, "/uptime"), buf, strlen(buf), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    } else if (strcmp(basic_topic, "/exit") == 0) {
        state->stop_client = true; // stop the client when ALL subscriptions are stopped
        sub_unsub_topics(state, false); // unsubscribe
    } else if(strcmp(basic_topic, "/up") == 0) {
        INFO_printf("Up command received\n");
        manual_mode_movimentation(state->factory, state->sector, 2048, 3500, state->ssd, state->delivered, state->objectives);
    } else if(strcmp(basic_topic, "/down") == 0) {
        INFO_printf("Down command received\n");
        manual_mode_movimentation(state->factory, state->sector, 2048, 500, state->ssd, state->delivered, state->objectives);
    } else if(strcmp(basic_topic, "/left") == 0) {
        INFO_printf("Left command received\n");
        manual_mode_movimentation(state->factory, state->sector, 500, 2048, state->ssd, state->delivered, state->objectives);
    } else if(strcmp(basic_topic, "/right") == 0) {
        INFO_printf("Right command received\n");
        manual_mode_movimentation(state->factory, state->sector, 3500, 2048, state->ssd, state->delivered, state->objectives);
    } else if(strcmp(basic_topic, "/cvrp") == 0) {
        INFO_printf("CVRP command received\n");
        solve_capacitated_vrp(state->factory, state->objectives, state->distances, state->delivered, state->sector, state->ssd);
    }
}

// Dados de entrada publicados
void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(state->topic, topic, sizeof(state->topic));
}

// Inicializar o cliente MQTT
void start_client(MQTT_CLIENT_DATA_T *state) {
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    const int port = MQTT_TLS_PORT;
    INFO_printf("Using TLS\n");
#else
    const int port = MQTT_PORT;
    INFO_printf("Warning: Not using TLS\n");
#endif

    state->mqtt_client_inst = mqtt_client_new();
    if (!state->mqtt_client_inst) {
        panic("MQTT client instance creation error");
    }
    INFO_printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    INFO_printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&state->mqtt_server_address));

    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(state->mqtt_client_inst, &state->mqtt_server_address, port, mqtt_connection_cb, state, &state->mqtt_client_info) != ERR_OK) {
        panic("MQTT broker connection error");
    }
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    // This is important for MBEDTLS_SSL_SERVER_NAME_INDICATION
    mbedtls_ssl_set_hostname(altcp_tls_context(state->mqtt_client_inst->conn), MQTT_SERVER);
#endif
    mqtt_set_inpub_callback(state->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, state);
    cyw43_arch_lwip_end();
}

// Call back com o resultado do DNS
void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    MQTT_CLIENT_DATA_T *state = (MQTT_CLIENT_DATA_T*)arg;
    if (ipaddr) {
        state->mqtt_server_address = *ipaddr;
        start_client(state);
    } else {
        panic("dns request failed");
    }
}

// Publicar temperatura
void publish_temperature(MQTT_CLIENT_DATA_T *state) {
    static float old_temperature;
    const char *temperature_key = full_topic(state, "/temperature");
    float temperature = read_onboard_temperature(TEMPERATURE_UNITS);
    if (temperature != old_temperature) {
        old_temperature = temperature;
        // Publish temperature on /temperature topic
        char temp_str[16];
        snprintf(temp_str, sizeof(temp_str), "%.2f", temperature);
        INFO_printf("Publishing %s to %s\n", temp_str, temperature_key);
        mqtt_publish(state->mqtt_client_inst, temperature_key, temp_str, strlen(temp_str), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    }
}

// Publicar temperatura
void temperature_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)worker->user_data;
    publish_temperature(state);
    async_context_add_at_time_worker_in_ms(context, worker, TEMP_WORKER_TIME_S * 1000);
}

async_at_time_worker_t temperature_worker = { .do_work = temperature_worker_fn };

void publish_battery_charge(MQTT_CLIENT_DATA_T *state) {
    static int old_charge = -1;
    int charge = state->factory->robot.charge; // Assuming charge is stored in the robot structure
    if (charge != old_charge) {
        old_charge = charge;
        char charge_str[8];
        snprintf(charge_str, sizeof(charge_str), "%d%%", charge);
        const char *charge_topic = full_topic(state, "/factory/robot/charge");
        INFO_printf("Publishing battery charge: %s to %s\n", charge_str, charge_topic);
        mqtt_publish(state->mqtt_client_inst, charge_topic, charge_str, strlen(charge_str), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    }
}

void charge_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)worker->user_data;
    publish_battery_charge(state);
    async_context_add_at_time_worker_in_ms(context, worker, CHARGE_WORKER_TIME_S * 1000);
}
async_at_time_worker_t charge_worker = { .do_work = charge_worker_fn };

//Publicar setor que está localizado
void publish_sector(MQTT_CLIENT_DATA_T *state){
    static uint8_t old_sector = 255; // Set to an invalid sector initially
    uint8_t sector = *state->sector; // Assuming sector is stored in the state structure
    if (sector != old_sector) {
        old_sector = sector;
        char sector_str[4];
        snprintf(sector_str, sizeof(sector_str), "%d", sector);
        const char *sector_topic = full_topic(state, "/factory/robot/sector");
        INFO_printf("Publishing sector: %s to %s\n", sector_str, sector_topic);
        mqtt_publish(state->mqtt_client_inst, sector_topic, sector_str, strlen(sector_str), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    }
}

void sector_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)worker->user_data;
    publish_sector(state);
    async_context_add_at_time_worker_in_ms(context, worker, SECTOR_WORKER_TIME_S * 1000);
}

async_at_time_worker_t sector_worker = { .do_work = sector_worker_fn };

// Conexão MQTT
void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        state->connect_done = true;
        sub_unsub_topics(state, true); // subscribe;

        // indicate online
        if (state->mqtt_client_info.will_topic) {
            mqtt_publish(state->mqtt_client_inst, state->mqtt_client_info.will_topic, "1", 1, MQTT_WILL_QOS, true, pub_request_cb, state);
        }

        // Publish temperature every 10 sec if it's changed
        temperature_worker.user_data = state;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &temperature_worker, 0);

        // Publish battery charge every 10 sec if it's changed
        charge_worker.user_data = state;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &charge_worker, 0);

        // Publish sector every 10 sec if it's changed
        sector_worker.user_data = state;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &sector_worker, 0);

    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        if (!state->connect_done) {
            panic("Failed to connect to mqtt server");
        }
    }
    else {
        panic("Unexpected status");
    }
}

void generate_client_id(char *client_id_buf, size_t buf_size) {
    char unique_id_buf[5];
    pico_get_unique_board_id_string(unique_id_buf, sizeof(unique_id_buf));
    for (int i = 0; i < sizeof(unique_id_buf) - 1; i++) {
        unique_id_buf[i] = tolower(unique_id_buf[i]);
    }

    snprintf(client_id_buf, buf_size, "%s%s", MQTT_DEVICE_NAME, unique_id_buf);
    INFO_printf("Device name %s\n", client_id_buf);
}

// Configure the MQTT client with the necessary parameters
void configure_mqtt_client(MQTT_CLIENT_DATA_T *state, const char *client_id_buf) {
    state->mqtt_client_info.client_id = client_id_buf;
    state->mqtt_client_info.keep_alive = MQTT_KEEP_ALIVE_S;

#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
    state->mqtt_client_info.client_user = MQTT_USERNAME;
    state->mqtt_client_info.client_pass = MQTT_PASSWORD;
#else
    state->mqtt_client_info.client_user = NULL;
    state->mqtt_client_info.client_pass = NULL;
#endif

    static char will_topic[MQTT_TOPIC_LEN];
    strncpy(will_topic, full_topic(state, MQTT_WILL_TOPIC), sizeof(will_topic));
    state->mqtt_client_info.will_topic = will_topic;
    state->mqtt_client_info.will_msg = MQTT_WILL_MSG;
    state->mqtt_client_info.will_qos = MQTT_WILL_QOS;
    state->mqtt_client_info.will_retain = true;

#if LWIP_ALTCP && LWIP_ALTCP_TLS
#ifdef MQTT_CERT_INC
    static const uint8_t ca_cert[] = TLS_ROOT_CERT;
    static const uint8_t client_key[] = TLS_CLIENT_KEY;
    static const uint8_t client_cert[] = TLS_CLIENT_CERT;

    state->mqtt_client_info.tls_config = altcp_tls_create_config_client_2wayauth(
        ca_cert, sizeof(ca_cert), client_key, sizeof(client_key),
        NULL, 0, client_cert, sizeof(client_cert)
    );

#if ALTCP_MBEDTLS_AUTHMODE != MBEDTLS_SSL_VERIFY_REQUIRED
    WARN_printf("Warning: tls without verification is insecure\n");
#endif
#else
    state->mqtt_client_info.tls_config = altcp_tls_create_config_client(NULL, 0);
    WARN_printf("Warning: tls without a certificate is insecure\n");
#endif
#endif
}

// Resolve o nome do servidor MQTT e conecta
void resolve_and_connect_mqtt(MQTT_CLIENT_DATA_T *state) {
    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(MQTT_SERVER, &state->mqtt_server_address, dns_found, state);
    cyw43_arch_lwip_end();

    if (err == ERR_OK) {
        start_client(state);
    } else if (err != ERR_INPROGRESS) {
        panic("DNS request failed");
    }
}

// Verifica se o cliente MQTT está conectado
bool verify_mqtt(MQTT_CLIENT_DATA_T *state) {
    if (!state->connect_done || mqtt_client_is_connected(state->mqtt_client_inst)) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));
    }
}