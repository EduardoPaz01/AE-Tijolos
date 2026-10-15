#include "adc.h"
#include "temporizador.h"

void setup_ADC() {
  analogReadResolution(12);                               // ADC_BITWIDTH_DEFAULT (12 bits)
  analogSetPinAttenuation(PINO_ADC_FIM_DE_CURSO, ADC_0db);
  analogSetPinAttenuation(PINO_ADC_TERMOPAR, ADC_0db);    // ADC_ATTEN_DB_0
}

float printa_ADC(uint8_t canal, float offset, float esforco) {
  int pino = (canal == CANAL_TERMOPAR) ? PINO_ADC_TERMOPAR : PINO_ADC_FIM_DE_CURSO;
  int adc_leitura = analogRead(pino);

  uint16_t tempo = (horas * 60 * 60) + (minutos * 60) + segundos;

  // Temperatura BRUTA
  float temp_bruta = 0;

  if (adc_leitura < 3302) {
    temp_bruta = ((float)adc_leitura * 0.2180) - 324.9;
  } else {
    temp_bruta = ((float)adc_leitura * 0.112877) + 22.066582; // converte o valor de 0-4095 até 0-650
  }
  // Ajuste de curva
  temp_bruta = -0.000964 * (temp_bruta) * (temp_bruta) + 1.399 * temp_bruta - 1.89;

  // Imprime: tempo, ADC bruto, temp bruta, esforço, referência
  Serial.printf("%d,%d,%.2f,%f,%f \n",
                tempo,
                adc_leitura,
                temp_bruta,
                esforco,
                offset);

  return temp_bruta;
}
