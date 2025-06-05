cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(ComunicacaoMQTT C CXX ASM)
pico_sdk_init()
include_directories(${CMAKE_SOURCE_DIR}/lib)
add_executable(${PROJECT_NAME} 
        ComunicacaoMQTT.c 
        lib/button.c
        lib/buzzer.c
        lib/factory.c
        lib/joystick.c
        lib/led_rgb.c
        lib/matrix.c
        lib/ssd1306.c
        lib/web_server.c
        lib/mqtt_client.c
        lib/temperature.c
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_PRINTF_SUPPORTS_FLOAT=1
        PICO_STDIO_ENABLE_PRINTF=1
)
target_link_libraries(${PROJECT_NAME} 
            pico_stdlib
            pico_time
            hardware_i2c
            hardware_adc
            hardware_pwm
            hardware_timer
            hardware_gpio
            hardware_pio
            hardware_clocks
            hardware_uart
            pico_cyw43_arch_lwip_threadsafe_background
            pico_lwip_mqtt
            pico_mbedtls
            pico_lwip_mbedtls
)
target_include_directories(${PROJECT_NAME}  PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}/lib/lwip/src/include
        ${PICO_SDK_PATH}/lib/lwip/src/include/arch
        ${PICO_SDK_PATH}/lib/lwip/src/include/lwip
)
target_sources(${PROJECT_NAME}  PRIVATE
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/httpd.c
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/fs.c
)
pico_enable_stdio_uart(${PROJECT_NAME}  0)
pico_enable_stdio_usb(${PROJECT_NAME}  1)

pico_generate_pio_header(${PROJECT_NAME}  ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

pico_add_extra_outputs(${PROJECT_NAME})
