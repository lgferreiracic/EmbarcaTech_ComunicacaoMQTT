#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdio.h>
#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"       
#include "lwip/pbuf.h"           
#include "lwip/tcp.h"            
#include "lwip/netif.h" 

#define WIFI_SSID "A35 de Lucas"
#define WIFI_PASSWORD "lucaslucas"
#define LED_PIN CYW43_WL_GPIO_LED_PIN 

void user_request(char *html, size_t html_size);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void server_init(void);

#endif