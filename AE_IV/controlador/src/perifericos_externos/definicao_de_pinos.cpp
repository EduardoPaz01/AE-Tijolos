#include "definicao_de_pinos.hpp"

void setup_pinos() {
  pinMode(PINO_LIGA, INPUT_PULLUP);
  pinMode(PINO_EMERGENCIA, INPUT_PULLUP);
  pinMode(PINO_FIM_DE_CURSO_ABERTO, INPUT_PULLUP);
  pinMode(PINO_FIM_DE_CURSO_FECHADO, INPUT_PULLUP);
  pinMode(PINO_MOTOR_PORTA_S, OUTPUT);
  pinMode(PINO_MOTOR_PORTA_D, OUTPUT);
  pinMode(PINO_RESISTENCIA, OUTPUT);

  digitalWrite(PINO_RESISTENCIA, LOW);
}
