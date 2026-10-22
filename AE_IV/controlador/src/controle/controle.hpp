#ifndef CONTROLE_HPP
#define CONTROLE_HPP

#include <Arduino.h>

#include "../perifericos_externos/temporizador.hpp"
#include "../perifericos_externos/definicao_de_pinos.hpp"

extern uint16_t ultimo_ciclo;
extern uint16_t ciclo_atual;

void PWM_resistencia(uint16_t periodo_m, float duty);

#endif
