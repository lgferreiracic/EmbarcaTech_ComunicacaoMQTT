#include "./lib/temperature.h"

float read_onboard_temperature(const char unit) {
    adc_select_input(4); // Select the ADC input for the temperature sensor
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    if (unit == 'C' || unit != 'F') {
        return tempC;
    } else if (unit == 'F') {
        return tempC * 9 / 5 + 32;
    }

    return -1.0f;
}

void temperature_sensor_init() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
}