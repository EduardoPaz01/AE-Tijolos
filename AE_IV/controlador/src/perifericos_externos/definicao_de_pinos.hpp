#ifndef DEFINICAO_DE_PINOS_HPP
#define DEFINICAO_DE_PINOS_HPP

#include <Arduino.h>

#define DEBUG 1

#define PINO_LIGA                 6
#define PINO_EMERGENCIA           5
#define PINO_MOTOR_PORTA_S        15
#define PINO_MOTOR_PORTA_D        0
#define PINO_FIM_DE_CURSO_ABERTO  16
#define PINO_FIM_DE_CURSO_FECHADO 19
#define PINO_RESISTENCIA          4

void setup_pinos();

#endif
