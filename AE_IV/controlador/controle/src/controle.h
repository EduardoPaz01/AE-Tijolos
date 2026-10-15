#ifndef CONTROLE_H_
#define CONTROLE_H_

#include <Arduino.h>

extern uint16_t ultimo_ciclo;
extern uint16_t ciclo_atual;

void PWM_resistencia(uint16_t periodo_m, float duty);

#endif
