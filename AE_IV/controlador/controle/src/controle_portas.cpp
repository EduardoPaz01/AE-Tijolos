#include "controle_portas.h"
#include "definicao_de_pinos.h"

uint8_t aux_fim_curso = 0;

uint8_t move_porta(uint8_t direcao) {
  if (ABRE) {
    if (digitalRead(PINO_FIM_DE_CURSO_ABERTO)) {
      digitalWrite(PINO_MOTOR_PORTA_S, LOW);
    } else {
      digitalWrite(PINO_MOTOR_PORTA_S, HIGH);
    }
  } else if (FECHA) {
    if (digitalRead(PINO_FIM_DE_CURSO_FECHADO)) {
      digitalWrite(PINO_MOTOR_PORTA_D, LOW);
    } else {
      digitalWrite(PINO_MOTOR_PORTA_D, HIGH);
    }
  }

  return 0;
}
