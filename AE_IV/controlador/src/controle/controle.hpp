#ifndef CONTROLE_HPP
#define CONTROLE_HPP

#include <Arduino.h>
#include <vector>

#include "../perifericos_externos/temporizador.hpp"
#include "../perifericos_externos/definicao_de_pinos.hpp"
#include "../perifericos_externos/adc.hpp"

// ---------------- Variáveis do controle (antes locais do app_main) ----------------
inline uint8_t aux = 0;
inline bool liga = 0;

inline double kp = 0.01654;
//inline double kd = 0;
inline double ti = 6954.1029;
inline double T = 1.0;

inline double ref = 0; // somente para seguimento de degrau
// ----- Receita: lista de pontos {tempo [s], temperatura [°C]} -----
// Entre dois pontos consecutivos o controle faz uma rampa linear.
// Os tempos devem ser crescentes.
struct Ponto {
  double tempo;         // segundos desde o início do processo
  double temperatura;   // °C
};
// Perfil padrão: sobe de 0 a 150°C a 3,5°C/min, mantém em 150°C pelo mesmo
// tempo da subida e desce a 0°C com a mesma inclinação (montado no setup).
const double TEMP_MAX_RECEITA = 150.0;                 // °C
const double TAXA_RAMPA_C_MIN = 3.5;                   // °C/min
inline std::vector<Ponto> receita;
inline uint32_t t_inicio_receita = 0;                  // instante (s) em que a receita começou
inline TaskHandle_t hTaskControle = NULL;
inline int segmento_atual = -1;  // índice do segmento da receita em execução

inline double alternativa_controle = 1; // 0 para degrau 1 para rampa

inline double u = 0;
inline double u_anterior = 0;
inline double e = 0;
inline double e_anterior = 0;
//inline double e_2_anterior = 0;

inline double alpha = 0;
inline double beta = 0;
//inline double omega = kp * (kd/T);

extern uint16_t ultimo_ciclo;
extern uint16_t ciclo_atual;

// Task do controle: chama loop_controle repetidamente, sem bloquear o loop()

// 
void loop_controle(const std::vector<Ponto>& receita);

// Faz o PWM da resistência com período em ms e duty cycle de 0 a 1.0
void PWM_resistencia(uint16_t periodo_m, float duty);

#endif
