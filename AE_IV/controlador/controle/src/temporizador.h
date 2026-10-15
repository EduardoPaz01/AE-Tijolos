#ifndef TEMPORIZADOR_H_
#define TEMPORIZADOR_H_

#include <Arduino.h>

#define FREQUENCIA_TIMER 1000000
#define CONTAGEM_TIMER   10000   // 10 ms

extern volatile uint16_t milisegundos;
extern volatile uint8_t  segundos;
extern volatile uint8_t  minutos;
extern volatile uint16_t horas;

void setup_temporizador();
void printa_tempo();

#endif
