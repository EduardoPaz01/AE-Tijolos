#include "controle.hpp"

uint16_t ultimo_ciclo = 0;
uint16_t ciclo_atual = 0;

void loop_controle(const std::vector<Ponto>& receita) {
  float temperatura = ler_ADC(CANAL_TERMOPAR);

  if (!liga) {                       // liga direto, sem depender do pino
    liga = 1;
    t_inicio_receita = (horas * 60 * 60) + (minutos * 60) + segundos;
    segmento_atual = -1;
    Serial.print("Forno LIGADO: ");
    if (receita.size() >= 2) {
      double delta_t = receita[1].tempo - receita[0].tempo;
      double delta_T = receita[1].temperatura - receita[0].temperatura;
      double a_coef = (delta_T / delta_t) * 60.0;
      Serial.printf("a=%.1f [°C / min]\n", a_coef);
    }
  }

  if (aux != segundos) {

    if (!alternativa_controle); // para degrau não há ação
    else if (alternativa_controle && !receita.empty()) { // se for rampa, calcula a referencia em cima do tempo
      uint32_t t = (horas * 60 * 60) + (minutos * 60) + segundos - t_inicio_receita; // em segundos desde o início da receita
      size_t n = receita.size();

      if (t < receita[0].tempo) {
        ref = receita[0].temperatura;
      }
      else if (t >= receita[n - 1].tempo) {
        ref = receita[n - 1].temperatura;
      }
      else {
        // acha o segmento i tal que receita[i].tempo <= t < receita[i+1].tempo
        int i = 0;
        while (t >= receita[i + 1].tempo) i++;

        double delta_t = receita[i + 1].tempo - receita[i].tempo;
        double delta_T = receita[i + 1].temperatura - receita[i].temperatura;
        double inclinacao = delta_T / delta_t;

        if (i != segmento_atual) {         // novo ponto: nova rampa
          segmento_atual = i;
          ref = receita[i].temperatura;    // parte exatamente do ponto da receita
        }
        ref += inclinacao;
      }
    }

    e = ref - temperatura;
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

  uint32_t tempo_total =
      ((uint32_t)horas * 3600) +
      ((uint32_t)minutos * 60) +
      segundos;
  uint32_t tempo_processo = tempo_total - t_inicio_receita;

  Serial.printf("%lu,%.2f,%.2f, %.2f\n",
                (unsigned long)tempo_processo,
                temperatura, ref,
                u * 100.0);

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

}

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
