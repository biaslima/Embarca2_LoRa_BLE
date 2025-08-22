#ifndef SENSORES_H
#define SENSORES_H

#include "hardware/i2c.h"

// Inicializa os sensores (neste caso, apenas o AHT20)
void sensores_init(i2c_inst_t *i2c);
// Lê os sensores e grava a string formatada no buffer fornecido
void sensores_ler(char *out_str, size_t len);

#endif
