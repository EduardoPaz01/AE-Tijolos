#ifndef DEFINICAO_DE_PINOS_HPP
#define DEFINICAO_DE_PINOS_HPP

#include <Arduino.h>

#define DEBUG 1

#define PINO_LIGA                 33
#define PINO_INICIAR              14
#define PINO_MOTOR_PORTA_S        15
#define PINO_MOTOR_PORTA_D        0
#define PINO_FIM_DE_CURSO_ABERTO  16
#define PINO_FIM_DE_CURSO_FECHADO 19
#define PINO_RESISTENCIA          5

void setup_pinos();

#endif
