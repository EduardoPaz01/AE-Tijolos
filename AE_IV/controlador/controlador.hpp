#ifndef CONTROLADOR_HPP
#define CONTROLADOR_HPP

/*
  ESP32-S3 - Portal cativo para configurar o Wi-Fi (AP + STA)
  ----------------------------------------------------------------------
  FLUXO:
   1) Ao ligar, se já existir SSID/senha salvos, o ESP32 tenta conectar
      direto nessa rede (até 15 s). Se conectar, nem abre a rede própria.
   2) Se não houver credenciais (ou a conexão falhar), abre a rede própria
      "ESP32-S3-AP" em modo AP+STA e o celular abre o portal sozinho.
   3) O usuário escolhe a rede e digita a senha. O ESP32 tenta conectar
      SEM derrubar a rede própria (modo AP+STA), e a página acompanha o
      resultado consultando /status.
   4) Falhou  -> a página mostra o erro e o usuário tenta de novo.
      Deu certo -> as credenciais são salvas na flash, a página mostra o
      novo endereço (forno.local + IP) e, alguns segundos depois de a
      página ter recebido essa informação, a rede própria é desligada.
   5) Se a rede do roteador cair por mais de 60 s, a rede própria volta
      a abrir para permitir reconfigurar.s

  RESET DAS CREDENCIAIS: segure o botão BOOT (GPIO0) por 3 s ao ligar.

  INTERFACE SERIAL (115200 bps, terminar linha com \n):
   Envie o comando "$GET.ALL" pelo Monitor Serial (ou qualquer app no PC
   que fale com a porta serial) para o ESP32 responder com um bloco
   legível + uma linha JSON contendo: IP atual, mascara de subrede,
   gateway, DNS, RSSI, MAC, hostname, SSID/senha salvos na flash, SSID
   atualmente conectado, e dados do Access Point (SSID, senha, IP,
   mascara, MAC) quando ele estiver ativo. Serve para descobrir o IP do
   ESP32 mesmo que o usuário não tenha visto a tela/celular a tempo.
   Envie "$HELP" para listar os comandos disponíveis.
   Envie "$SET.COR=verde" (vermelho, verde, azul, amarelo, ciano, magenta,
   branco, desligado) para trocar a cor do LED RGB nativo.

  LED RGB: a tela "Gráfico em Tempo Real" da página principal envia
  POST /cor (cor=<nome>) e o ESP32 aplica a cor no LED nativo da placa.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_wifi.h>

#include "src/front/app_html.hpp"
#include "src/front/app_style.hpp"
#include "src/front/app_script.hpp"

#include "src/rede/conexao.hpp"
#include "src/rede/credenciais.hpp"
#include "src/rede/portal_cativo.hpp"
#include "src/rede/rede_propria.hpp"
#include "src/rede/rotas.hpp"
#include "src/perifericos_externos/serial.hpp"
#include "src/utilidades/utilidades.hpp"
#include "src/perifericos_externos/lcd.hpp"

#include "src/perifericos_externos/adc.hpp"
#include "src/controle/controle_portas.hpp"
#include "src/controle/controle.hpp"
#include "src/perifericos_externos/definicao_de_pinos.hpp"
#include "src/perifericos_externos/temporizador.hpp"


// ----- LED RGB nativo do ESP32-S3 (WS2812) -----
// DevKitC-1: GPIO48 (v1.0) ou GPIO38 (v1.1). O core define RGB_BUILTIN
// conforme a placa escolhida; se não definir, ajuste o pino abaixo.
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48
#endif
const uint8_t BRILHO_LED = 40;                 // const => linkagem interna, ok
inline String corAtual = "desligado";

// ----- Rede própria do ESP32 (modo AP) -----
inline const char* apSsid  = "ESP32-S3-AP";
inline const char* apSenha = "12345678";       // mínimo 8 caracteres

// ----- Portal cativo -----
inline const char* dominioDNS     = "*";
inline const char* dominioMascara = "forno.ae";
inline IPAddress apIP(192, 168, 4, 1);
inline IPAddress gateway(192, 168, 4, 1);
inline IPAddress subnet(255, 255, 255, 0);

// ----- Nome na rede do roteador: http://forno.local -----
inline const char* hostname = "forno";

// ----- Tempos -----
const unsigned long TIMEOUT_CONEXAO_MS       = 20000;  // limite de uma tentativa
const unsigned long TIMEOUT_BOOT_MS          = 15000;  // tentativa ao ligar
const unsigned long ESPERA_APOS_ENTREGA_MS   = 8000;   // tempo p/ o usuário ler o IP
const unsigned long LIMITE_AP_APOS_SUCESSO_MS = 60000; // fecha o AP mesmo sem ninguém ler
const unsigned long REABRIR_AP_SEM_LINK_MS   = 60000;  // reabre o AP se o roteador sumir
const uint8_t PIN_RESET = 0;                           // botão BOOT

// ----- Estado -----
enum Estado { OCIOSO, CONECTANDO, CONECTADO, FALHA };
inline Estado estado = OCIOSO;

inline String credSsid, credSenha;      // salvas na flash
inline String pendSsid, pendSenha;      // tentativa em andamento

inline bool apAtivo = false;
inline bool statusEntregue = false;
inline unsigned long inicioTentativa = 0;
inline unsigned long tempoSucesso = 0;
inline unsigned long tempoEntrega = 0;
inline unsigned long semLinkDesde = 0;
inline unsigned long reiniciarEm = 0;

// ----- Cache da varredura de redes -----
inline String redesJson = "[]";
inline bool escaneando = false;

inline Preferences prefs;
inline WebServer server(80);
inline DNSServer dnsServer;
const byte DNS_PORT = 53;

inline String bufferSerial;

// ================================================
//  MÁQUINA DE ESTADOS - Forno
// ================================================
enum Estado_Maquina {
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

inline uint8_t estadoAtual = DESLIGADO;
inline uint8_t estadoAnterior = 0;
inline bool a = false;

#endif // CONTROLADOR_HPP