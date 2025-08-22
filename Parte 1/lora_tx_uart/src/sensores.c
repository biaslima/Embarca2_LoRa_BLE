#include "../inc/aht20.h"
#include <stdio.h>

static i2c_inst_t *i2c_usado_sensores = NULL;
static bool aht20_ok = false;

void sensores_init(i2c_inst_t *i2c) {
    i2c_usado_sensores = i2c;

    // Inicializa o sensor AHT20 e verifica se foi bem-sucedido
    aht20_ok = aht20_init(i2c_usado_sensores);
    if (!aht20_ok) {
        printf("Erro na inicialização do AHT20!\n");
    }
}

void sensores_ler(char *out_str, size_t len) {
    AHT20_Data dados_aht;
    
    // Leitura do AHT20
    if (aht20_ok) {
        if (!aht20_read(i2c_usado_sensores, &dados_aht)) {
            dados_aht.temperature = -1.0f;
            dados_aht.humidity = -1.0f;
        }
    } else {
        // Se a inicialização falhou, preenche com valores de erro
        dados_aht.temperature = -1.0f;
        dados_aht.humidity = -1.0f;
    }
    
    // Formata a mensagem com os dados de temperatura e umidade
    snprintf(out_str, len, "T=%.1fC U=%.1f%%", dados_aht.temperature, dados_aht.humidity);
}
