#include "../inc/rfm95.h"
#include "pico/stdlib.h"
#include <string.h>

static spi_inst_t *lora_spi;
static uint cs_pin, rst_pin, irq_pin;
static volatile bool message_received = false;

// Funções privadas
static void rfm95_write_register(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg | 0x80, val };
    gpio_put(cs_pin, 0);
    spi_write_blocking(lora_spi, buf, 2);
    gpio_put(cs_pin, 1);
}

static uint8_t rfm95_read_register(uint8_t reg) {
    uint8_t buf[2] = { reg & 0x7F, 0x00 };
    gpio_put(cs_pin, 0);
    spi_write_read_blocking(lora_spi, buf, buf, 2);
    gpio_put(cs_pin, 1);
    return buf[1];
}

static void rfm95_write_fifo(const uint8_t *data, uint8_t length) {
    gpio_put(cs_pin, 0);
    uint8_t reg = REG_FIFO | 0x80;
    spi_write_blocking(lora_spi, &reg, 1);
    spi_write_blocking(lora_spi, data, length);
    gpio_put(cs_pin, 1);
}

static void rfm95_read_fifo(uint8_t *data, uint8_t length) {
    gpio_put(cs_pin, 0);
    uint8_t reg = REG_FIFO & 0x7F;
    spi_write_blocking(lora_spi, &reg, 1);
    spi_read_blocking(lora_spi, 0, data, length);
    gpio_put(cs_pin, 1);
}

// Callback de interrupção
static void rfm95_irq_callback(uint gpio, uint32_t events) {
    message_received = true;
}

void rfm95_init(spi_inst_t *spi, uint cs, uint rst, uint irq) {
    lora_spi = spi;
    cs_pin = cs;
    rst_pin = rst;
    irq_pin = irq;

    // Configurar pinos
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);

    gpio_init(rst_pin);
    gpio_set_dir(rst_pin, GPIO_OUT);

    gpio_init(irq_pin);
    gpio_set_dir(irq_pin, GPIO_IN);
    gpio_set_irq_enabled_with_callback(irq_pin, GPIO_IRQ_EDGE_RISE, true, &rfm95_irq_callback);

    // Reset do módulo
    gpio_put(rst_pin, 0);
    sleep_ms(10);
    gpio_put(rst_pin, 1);
    sleep_ms(10);

    // Verificar se o módulo está respondendo
    rfm95_write_register(REG_OPMODE, RF95_MODE_SLEEP);
    sleep_ms(10);
    
    uint8_t version = rfm95_read_register(0x42); // Registro de versão
    if (version == 0x00 || version == 0xFF) {
        // Módulo não respondendo
        return;
    }
}

void rfm95_config(float freq, int tx_power) {
    // Modo sleep
    rfm95_write_register(REG_OPMODE, RF95_MODE_SLEEP);
    sleep_ms(10);
    
    // Modo LoRa
    rfm95_write_register(REG_OPMODE, RFM95_LONG_RANGE_MODE);
    sleep_ms(10);
    
    // Modo standby
    rfm95_write_register(REG_OPMODE, RFM95_LONG_RANGE_MODE | RF95_MODE_STANDBY);
    sleep_ms(10);

    // Configurar frequência
    uint64_t frf = (uint64_t)((freq * 1000000.0) / 61.03515625);
    rfm95_write_register(REG_FRF_MSB, (uint8_t)(frf >> 16));
    rfm95_write_register(REG_FRF_MID, (uint8_t)(frf >> 8));
    rfm95_write_register(REG_FRF_LSB, (uint8_t)(frf >> 0));

    // Configurar potência de transmissão
    if (tx_power > 23) tx_power = 23;
    if (tx_power < 5) tx_power = 5;
    
    if (tx_power > 20) {
        // PA_BOOST + PA_DAC
        rfm95_write_register(REG_PA_DAC, PA_DAC_20);
        rfm95_write_register(REG_PA_CONFIG, 0x80 | (tx_power - 5));
    } else {
        // PA_BOOST normal
        rfm95_write_register(REG_PA_DAC, 0x84); // PA_DAC padrão
        rfm95_write_register(REG_PA_CONFIG, 0x80 | (tx_power - 5));
    }

    // Configurar modem (BW=125kHz, CR=4/5, SF=7)
    // Usando as definições do novo rfm95.h
    rfm95_write_register(REG_MODEM_CONFIG, BANDWIDTH_125K | ERROR_CODING_4_5 | EXPLICIT_MODE); 
    rfm95_write_register(REG_MODEM_CONFIG2, SPREADING_7 | CRC_ON); 
    rfm95_write_register(REG_MODEM_CONFIG3, 0x04); // LowDataRateOptimize=off, AGC=on

    // Configurar preâmbulo
    rfm95_write_register(REG_PREAMBLE_MSB, 0x00);
    rfm95_write_register(REG_PREAMBLE_LSB, 0x08);

    // Configurar endereços base do FIFO
    rfm95_write_register(REG_FIFO_TX_BASE_AD, 0x00);
    rfm95_write_register(REG_FIFO_RX_BASE_AD, 0x00);

    // Modo standby
    rfm95_set_mode_standby();
}

void rfm95_send_message(const char *msg) {
    rfm95_set_mode_standby();
    
    uint8_t length = strlen(msg);
    if (length > PAYLOAD_LENGTH) length = PAYLOAD_LENGTH;
    
    // Configurar ponteiro do FIFO para base TX
    rfm95_write_register(REG_FIFO_ADDR_PTR, 0x00);
    
    // Escrever dados no FIFO
    rfm95_write_fifo((const uint8_t*)msg, length);
    
    // Configurar tamanho do payload
    rfm95_write_register(REG_PAYLOAD_LENGTH, length);
    
    // Configurar DIO0 para TxDone
    rfm95_write_register(REG_DIO_MAPPING_1, 0x40);
    
    // Limpar flags de interrupção
    rfm95_write_register(REG_IRQ_FLAGS, 0xFF);
    
    // Modo TX
    rfm95_set_mode_tx();
    
    // Aguardar transmissão (timeout de 1 segundo)
    uint32_t start = time_us_32();
    while ((time_us_32() - start) < 1000000) {
        uint8_t irq_flags = rfm95_read_register(REG_IRQ_FLAGS);
        if (irq_flags & RFM95_IRQ_TX_DONE) {
            break;
        }
        sleep_ms(1);
    }
    
    // Limpar flag TxDone
    rfm95_write_register(REG_IRQ_FLAGS, RFM95_IRQ_TX_DONE);
    
    // Voltar para standby
    rfm95_set_mode_standby();
}

bool rfm95_receive_message(rfm95_packet_t *packet) {
    if (!packet) return false;
    
    uint8_t irq_flags = rfm95_read_register(REG_IRQ_FLAGS);
    
    if (irq_flags & RFM95_IRQ_RX_DONE) {
        // Obter tamanho do payload recebido
        uint8_t length = rfm95_read_register(REG_RX_NB_BYTES);
        
        if (length > 0 && length < 64) {
            // Obter endereço atual do FIFO RX
            uint8_t fifo_addr = rfm95_read_register(REG_FIFO_RX_CURRENT_ADDR);
            rfm95_write_register(REG_FIFO_ADDR_PTR, fifo_addr);
            
            // Ler dados do FIFO
            uint8_t buffer[64];
            rfm95_read_fifo(buffer, length);
            
            // Copiar para estrutura
            memcpy(packet->message, buffer, length);
            packet->message[length] = '\0';
            packet->length = length;
            
            // Obter RSSI e SNR
            packet->rssi = rfm95_get_rssi();
            packet->snr = rfm95_get_snr();
            packet->valid = true;
            
            // Limpar flags
            rfm95_write_register(REG_IRQ_FLAGS, 0xFF);
            
            return true;
        }
        
        // Limpar flags se erro
        rfm95_write_register(REG_IRQ_FLAGS, 0xFF);
    }
    
    return false;
}

void rfm95_set_mode_rx(void) {
    // Configurar DIO0 para RxDone
    rfm95_write_register(REG_DIO_MAPPING_1, 0x00);
    
    // Limpar flags de interrupção
    rfm95_write_register(REG_IRQ_FLAGS, 0xFF);
    
    // Configurar ponteiro do FIFO para base RX
    rfm95_write_register(REG_FIFO_ADDR_PTR, 0x00);
    
    // Modo RX contínuo
    rfm95_write_register(REG_OPMODE, RFM95_LONG_RANGE_MODE | RF95_MODE_RX_CONTINUOUS);
}

void rfm95_set_mode_tx(void) {
    rfm95_write_register(REG_OPMODE, RFM95_LONG_RANGE_MODE | RF95_MODE_TX);
}

void rfm95_set_mode_standby(void) {
    rfm95_write_register(REG_OPMODE, RFM95_LONG_RANGE_MODE | RF95_MODE_STANDBY);
}

bool rfm95_available(void) {
    uint8_t irq_flags = rfm95_read_register(REG_IRQ_FLAGS);
    return (irq_flags & RFM95_IRQ_RX_DONE) != 0;
}

int16_t rfm95_get_rssi(void) {
    int16_t rssi = rfm95_read_register(REG_PKT_RSSI_VALUE);
    
    // Ajustar para frequência
    if (915000000 >= 779000000) {
        rssi = rssi - 157;
    } else {
        rssi = rssi - 164;
    }
    
    return rssi;
}

int8_t rfm95_get_snr(void) {
    return (int8_t)rfm95_read_register(REG_PKT_SNR_VALUE) / 4;
}
