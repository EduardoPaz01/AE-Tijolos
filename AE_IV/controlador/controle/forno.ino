// ================================================
//  FORNO - Controle PI de temperatura (ESP32 / Arduino IDE)
//  Core esperado: esp32 by Espressif (3.x; 2.0.x também funciona)
// ================================================
#include "src/adc.h"
#include "src/controle.h"
#include "src/controle_portas.h"
#include "src/definicao_de_pinos.h"
#include "src/temporizador.h"

// ================================================
//  MÁQUINA DE ESTADOS - Forno
// ================================================
enum Estado {
  DESLIGADO,
  AGUARDANDO_SET_POINT,
  INICIO_DE_PROCESSO,
  CARREGAMENTO,
  FORNO_CARREGADO,
  COZIMENTO,
  FIM_DE_PROCESSO,
  DESCARREGAMENTO,
  FECHAMENTO_DE_PORTAS
};

uint8_t estadoAtual = DESLIGADO;
uint8_t estadoAnterior = 0;
bool a = false;

// ---------------- Variáveis do controle (antes locais do app_main) ----------------
uint8_t aux = 0;
bool liga = 0;

double kp = 0.01654;
//double kd = 0;
double ti = 6954.1029;
double T = 1.0;

double ref = 0; // somente para seguimento de degrau
double temp_inicial = 50;
double temp_final = 150;
double tempo_intervalo_i = 100;  // segundos
double tempo_intervalo_f = 5100; // segundos -  1°C /min

double alternativa_controle = 1; // 0 para degrau 1 para rampa

double u = 0;
double u_anterior = 0;
double e = 0;
double e_anterior = 0;
//double e_2_anterior = 0;

double alpha;
double beta;
//float omega = kp * (kd/T);

void setup() {
  Serial.begin(115200);

  setup_temporizador();
  setup_pinos();
  setup_ADC();

  alpha = kp * (1.0 + (T / (2.0 * ti)));
  beta  = kp * ((T / (2.0 * ti)) - 1.0);
  //omega = kp * (kd/T);
}

void loop() {
  if (digitalRead(PINO_LIGA)) {
    liga = 1;
    Serial.print("Forno LIGADO: ");
    double delta_t = tempo_intervalo_f - tempo_intervalo_i;
    double delta_T = temp_final - temp_inicial;
    double a_coef = (delta_T / delta_t) * 60.0;
    Serial.printf("a=%.f [°C / min]\n", a_coef);
  }

  if (aux != segundos) {

    if (!alternativa_controle); // para degrau não há ação
    else if (alternativa_controle) { // se for rampa, calcula a referencia em cima do tempo
      uint16_t t = (horas * 60 * 60) + (minutos * 60) + segundos; // em segundos
      double delta_t = tempo_intervalo_f - tempo_intervalo_i;
      double delta_T = temp_final - temp_inicial;

      double inclinacao = delta_T / delta_t;
      //double offset = temp_inicial - inclinacao*tempo_intervalo_i;

      if (t < tempo_intervalo_i) {
        ref = temp_inicial;
      }
      else if (t >= tempo_intervalo_i && t < tempo_intervalo_f) {
        ref += inclinacao;
      }
      else {
        ref = temp_final;
      }
    }

    e = ref - printa_ADC(CANAL_TERMOPAR, ref, u_anterior);
    u = u_anterior + (alpha * e) + (beta * e_anterior);

    u_anterior = u;
    //e_2_anterior = e_anterior;
    e_anterior = e;

    if (u > 1.0) u = 1.0;
    if (u < 0.01) u = 0.0;
  }
  aux = segundos;

  if (liga) {
    PWM_resistencia(10000, u);
  }

  /* Máquina de estados (desativada no original)
  switch (estadoAtual) {
    case DESLIGADO:
      if (digitalRead(PINO_LIGA) == 1) estadoAtual = AGUARDANDO_SET_POINT;
      break;
    case AGUARDANDO_SET_POINT: {
      int setpoint_temperatura = 0;
      // (original usava scanf; no Arduino use Serial.parseInt())
      if (setpoint_temperatura >= 0) estadoAtual = INICIO_DE_PROCESSO;
      break; }
    case INICIO_DE_PROCESSO:
      if (move_porta(ABRE)) estadoAtual = CARREGAMENTO;
      break;
    case CARREGAMENTO:
      estadoAtual = FORNO_CARREGADO;
      break;
    case FORNO_CARREGADO:
      if (move_porta(FECHA)) estadoAtual = COZIMENTO;
      break;
    case COZIMENTO:
      // liga controle de temperatura + geração de gráficos
      break;
    case FIM_DE_PROCESSO:
      if (move_porta(ABRE)) estadoAtual = DESCARREGAMENTO;
      break;
    case DESCARREGAMENTO:
      estadoAtual = FECHAMENTO_DE_PORTAS;
      break;
    case FECHAMENTO_DE_PORTAS:
      if (move_porta(FECHA)) estadoAtual = AGUARDANDO_SET_POINT;
      break;
  }
  estadoAnterior = estadoAtual;
  */

  delay(10);
}
