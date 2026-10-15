#ifndef CONTROLE_PORTAS_H_
#define CONTROLE_PORTAS_H_

#include <Arduino.h>

#define ABRE  1
#define FECHA 2

extern uint8_t aux_fim_curso;

uint8_t move_porta(uint8_t direcao);

#endif
