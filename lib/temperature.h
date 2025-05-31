#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "pico/stdlib.h"
#include "hardware/adc.h"           // Biblioteca de hardware para conversão ADC

#include "./lib/mqtt_client.h" // Include the MQTT client library

// Definição da escala de temperatura
#ifndef TEMPERATURE_UNITS
#define TEMPERATURE_UNITS 'C' // Set to 'F' for Fahrenheit
#endif

//Leitura de temperatura do microcotrolador
float read_onboard_temperature(const char unit);

// Inicialização do sensor de temperatura
void temperature_sensor_init();

#endif