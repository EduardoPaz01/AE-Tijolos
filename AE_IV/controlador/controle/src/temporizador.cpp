#include "temporizador.h"

volatile uint16_t milisegundos = 0;
volatile uint8_t  segundos = 0;
volatile uint8_t  minutos = 0;
volatile uint16_t horas = 0;

static hw_timer_t *timer_tempo = NULL;

static void ARDUINO_ISR_ATTR conta_tempo() {
  milisegundos += 10;

  if (milisegundos >= 1000) {
    milisegundos = 0;
    segundos++;

    if (segundos >= 60) {
      segundos = 0;
      minutos++;

      if (minutos >= 60) {
        minutos = 0;
        horas++;
      }
    }
  }
}

void setup_temporizador() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  timer_tempo = timerBegin(FREQUENCIA_TIMER);          // 1 MHz
  timerAttachInterrupt(timer_tempo, &conta_tempo);
  timerAlarm(timer_tempo, CONTAGEM_TIMER, true, 0);    // auto-reload, 10 ms
#else
  timer_tempo = timerBegin(0, 80, true);               // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(timer_tempo, &conta_tempo, true);
  timerAlarmWrite(timer_tempo, CONTAGEM_TIMER, true);
  timerAlarmEnable(timer_tempo);
#endif
}

void printa_tempo() {
  Serial.printf("horas: %d minutos: %d segundos %d milisegundos: %d\n", horas, minutos, segundos, milisegundos);
}
