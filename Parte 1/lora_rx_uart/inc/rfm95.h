#ifndef RFM95_H
#define RFM95_H

#include "hardware/spi.h"
#include <stdint.h>
#include <stdbool.h>

// Endereços de registradores
#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08
#define REG_PA_CONFIG               0x09
#define REG_FIFO_ADDR_PTR           0x0D 
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_PKT_SNR_VALUE           0x19
#define REG_PKT_RSSI_VALUE          0x1A
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_PREAMBLE_MSB            0x20
#define REG_PREAMBLE_LSB            0x21
#define REG_PAYLOAD_LENGTH          0x22
#define REG_HOP_PERIOD              0x24
#define REG_MODEM_CONFIG3           0x26
#define REG_DIO_MAPPING_1           0x40
#define REG_PA_DAC                  0x4D

// Modos de operação
#define RFM95_LONG_RANGE_MODE       0x80
#define RF95_MODE_SLEEP             0x00
#define RF95_MODE_STANDBY           0x01
#define RF95_MODE_TX                0x03
#define RF95_MODE_RX_CONTINUOUS     0x05

#define PAYLOAD_LENGTH              255

// Configuração do pacote de dados
#define EXPLICIT_MODE               0x00
#define IMPLICIT_MODE               0x01
#define ERROR_CODING_4_5            0x02
#define ERROR_CODING_4_6            0x04
#define ERROR_CODING_4_7            0x06
#define ERROR_CODING_4_8            0x08

// Configuração da largura de banda (BW)
#define BANDWIDTH_7K8               0x00
#define BANDWIDTH_10K4              0x10
#define BANDWIDTH_15K6              0x20
#define BANDWIDTH_20K8              0x30
#define BANDWIDTH_31K25             0x40
#define BANDWIDTH_41K7              0x50
#define BANDWIDTH_62K5              0x60
#define BANDWIDTH_125K              0x70
#define BANDWIDTH_250K              0x80
#define BANDWIDTH_500K              0x90

// Configuração do Spreading Factor (SF)
#define SPREADING_6                 0x60
#define SPREADING_7                 0x70
#define SPREADING_8                 0x80
#define SPREADING_9                 0x90
#define SPREADING_10                0xA0
#define SPREADING_11                0xB0
#define SPREADING_12                0xC0

#define CRC_OFF                     0x00
#define CRC_ON                      0x04

// Power Amplifier Config
#define PA_DAC_20                   0x87

// Flags de interrupção (do rfm95.h original)
#define RFM95_IRQ_TX_DONE           0x08
#define RFM95_IRQ_RX_DONE           0x40

// Estrutura para dados recebidos
typedef struct {
    char message[64];
    int16_t rssi;
    int8_t snr;
    uint8_t length;
    bool valid;
} rfm95_packet_t;

// Funções públicas
void rfm95_init(spi_inst_t *spi, uint cs, uint rst, uint irq);
void rfm95_config(float freq, int tx_power);
void rfm95_send_message(const char *msg);
bool rfm95_receive_message(rfm95_packet_t *packet);
void rfm95_set_mode_rx(void);
void rfm95_set_mode_tx(void);
void rfm95_set_mode_standby(void);
bool rfm95_available(void);
int16_t rfm95_get_rssi(void);
int8_t rfm95_get_snr(void);

#endif