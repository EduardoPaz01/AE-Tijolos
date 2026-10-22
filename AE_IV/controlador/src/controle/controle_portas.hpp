#ifndef CONTROLE_PORTAS_HPP
#define CONTROLE_PORTAS_HPP

#include <Arduino.h>

#include "../perifericos_externos/definicao_de_pinos.hpp"

#define ABRE  1
#define FECHA 2

extern uint8_t aux_fim_curso;

uint8_t move_porta(uint8_t direcao);

#endif
