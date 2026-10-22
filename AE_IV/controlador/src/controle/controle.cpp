#include "controle.hpp"

uint16_t ultimo_ciclo = 0;
uint16_t ciclo_atual = 0;

void PWM_resistencia(uint16_t periodo_m, float duty) {
  // Tempo total em ms desde o início
  uint32_t tempo_total_ms = ((uint32_t)horas * 3600 * 1000) + ((uint32_t)minutos * 60 * 1000) + ((uint32_t)segundos * 1000) + milisegundos;

  // Fase dentro do ciclo atual (0 a periodo_m - 1)
  uint16_t fase = tempo_total_ms % periodo_m;

  // Tempo "on" baseado no duty cycle (em ms)
  uint16_t tempo_on = (periodo_m * duty);

  digitalWrite(PINO_RESISTENCIA, (fase < tempo_on) ? HIGH : LOW);

  // Progresso em % do ciclo (debug)
  ciclo_atual = (fase * 100) / periodo_m;
}
