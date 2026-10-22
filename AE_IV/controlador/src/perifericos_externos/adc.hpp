#ifndef ADC_HPP
#define ADC_HPP

#include <Arduino.h>

// No ESP32: ADC2_CH8 = GPIO25 | ADC2_CH9 = GPIO26
#define CANAL_FIM_DE_CURSO 8
#define CANAL_TERMOPAR     9
#define PINO_ADC_FIM_DE_CURSO 25   // canal 8
#define PINO_ADC_TERMOPAR     26   // canal 9 (o comentário original dizia gpio25, mas canal 9 = GPIO26)

void setup_ADC();
float printa_ADC(uint8_t canal, float offset, float esforco);

#endif
