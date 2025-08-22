#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "inc/rfm95.h"
#include "inc/ssd1306.h"
#include "inc/sensores.h"


#define PIN_RST   20
#define PIN_CS    17
#define PIN_IRQ   8

#define LED_TESTE     15
#define LED_AZUL      12
#define LED_VERDE     11
#define LED_VERMELHO  13

#define BTN_A     5
#define BTN_B     6
 
// I2C Display
#define DISPLAY_I2C_SDA   14
#define DISPLAY_I2C_SCL   15
#define DISPLAY_I2C_PORT i2c1 
#define DISPLAY_ADDR 0x3C

// I2C Sensores
#define SENSOR_I2C_SDA    0
#define SENSOR_I2C_SCL    1
#define SENSOR_I2C_PORT i2c0

// Variáveis globais
ssd1306_t display;

static char last_message[64] = "Aguardando...";
static char status_msg[32] = "PRONTO";
static uint32_t tx_count = 0;
static uint8_t contador = 0;

void init_gpio(void) {
    gpio_init(LED_TESTE); gpio_set_dir(LED_TESTE, GPIO_OUT);
    gpio_init(LED_AZUL); gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_init(LED_VERDE); gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_VERMELHO); gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    
    gpio_init(BTN_A); gpio_set_dir(BTN_A, GPIO_IN); gpio_pull_up(BTN_A);
    gpio_init(BTN_B); gpio_set_dir(BTN_B, GPIO_IN); gpio_pull_up(BTN_B);
    
    gpio_put(LED_AZUL, 1);
    gpio_put(LED_VERDE, 0);
    gpio_put(LED_VERMELHO, 0);
    gpio_put(LED_TESTE, 0);
}

void init_spi(void) {
    spi_init(spi0, 1000000);
    gpio_set_function(16, GPIO_FUNC_SPI); // SCK
    gpio_set_function(19, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(18, GPIO_FUNC_SPI); // MISO
}

void init_display_i2c(void) {
    i2c_init(DISPLAY_I2C_PORT, 400000);
    gpio_set_function(DISPLAY_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_I2C_SDA);
    gpio_pull_up(DISPLAY_I2C_SCL);
}

void init_sensor_i2c(void) {
    i2c_init(SENSOR_I2C_PORT, 400000);
    gpio_set_function(SENSOR_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_I2C_SDA);
    gpio_pull_up(SENSOR_I2C_SCL);
}

void init_display(void) {
    ssd1306_init(&display, 128, 64, false, DISPLAY_ADDR, DISPLAY_I2C_PORT);
    ssd1306_config(&display);
    ssd1306_fill(&display, false);
    
    ssd1306_draw_string(&display, "LoRa BitDogLab", 0, 0);
    ssd1306_draw_string(&display, "Freq: 915 MHz", 0, 8);
    ssd1306_draw_string(&display, "Power: 20 dBm", 0, 16);
    ssd1306_draw_string(&display, "Status: INIT", 0, 24);
    ssd1306_send_data(&display);
    
    sleep_ms(2000);
}

void update_display(void) {
    char temp[64];
    ssd1306_fill(&display, false);

    ssd1306_draw_string(&display, "LoRa Transceiver", 0, 0);
    ssd1306_hline(&display, 0, 127, 9, true);
    snprintf(temp, sizeof(temp), "Status: %s", status_msg);
    ssd1306_draw_string(&display, temp, 0, 12);
    snprintf(temp, sizeof(temp), "TX:%lu", tx_count);
    ssd1306_draw_string(&display, temp, 0, 20);
    ssd1306_draw_string(&display, "Ultima msg:", 0, 28);
    ssd1306_draw_string(&display, last_message, 0, 36);
    ssd1306_draw_string(&display, "A: Sensores  B:Teste", 0, 56);
    ssd1306_send_data(&display);
}

void send_sensor_data(void) {
    char dados[64];
    sensores_ler(dados, sizeof(dados));

    char pacote[80];
    snprintf(pacote, sizeof(pacote), "P2:%s #%d", dados, ++contador);

    rfm95_send_message(pacote);
    rfm95_set_mode_standby();

    tx_count++;
    strcpy(last_message, pacote);
    strcpy(status_msg, "ENVIADO");
    printf("Mensagem enviada: %s\n", pacote);
    update_display();
}

void send_test_message(const char *msg) {
    rfm95_send_message(msg);
    rfm95_set_mode_standby();

    tx_count++;
    strcpy(last_message, msg);
    strcpy(status_msg, "ENVIADO");
    printf("Mensagem enviada: %s\n", msg);
    update_display();
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    init_gpio();
    init_spi();
    init_display_i2c();  // Inicializa I2C para display
    init_sensor_i2c();   // Inicializa I2C para sensores
    init_display();
    sensores_init(SENSOR_I2C_PORT);  // Usa o barramento I2C correto dos sensores

    rfm95_init(spi0, PIN_CS, PIN_RST, PIN_IRQ);
    rfm95_config(915.0, 20);

    strcpy(status_msg, "PRONTO PARA TX");
    update_display();

    while (true) {
        if (!gpio_get(BTN_A)) {
            gpio_put(LED_VERMELHO, 1);
            send_sensor_data();
            gpio_put(LED_VERMELHO, 0);
            sleep_ms(300);
        }

        if (!gpio_get(BTN_B)) {
            gpio_put(LED_TESTE, 1);
            send_test_message("Test Message");
            gpio_put(LED_TESTE, 0);
            sleep_ms(300);
        }

        sleep_ms(50);
    }

    return 0;
}