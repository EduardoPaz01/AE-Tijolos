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

#include "src/app_html.hpp"
#include "src/app_style.hpp"
#include "src/app_script.hpp"

// ----- LED RGB nativo do ESP32-S3 (WS2812) -----
// DevKitC-1: GPIO48 (v1.0) ou GPIO38 (v1.1). O core define RGB_BUILTIN
// conforme a placa escolhida; se não definir, ajuste o pino abaixo.
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48
#endif
const uint8_t BRILHO_LED = 40;                 // 0-255 (o LED é muito forte em 255)
String corAtual = "desligado";

// ----- Rede própria do ESP32 (modo AP) -----
const char* apSsid  = "ESP32-S3-AP";
const char* apSenha = "12345678";          // mínimo 8 caracteres

// ----- Portal cativo -----
const char* dominioDNS     = "*";
const char* dominioMascara = "forno.ae";   // usado só enquanto o AP está ativo
IPAddress apIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// ----- Nome na rede do roteador: http://forno.local -----
const char* hostname = "forno";

// ----- Tempos -----
const unsigned long TIMEOUT_CONEXAO_MS       = 20000;  // limite de uma tentativa
const unsigned long TIMEOUT_BOOT_MS          = 15000;  // tentativa ao ligar
const unsigned long ESPERA_APOS_ENTREGA_MS   = 8000;   // tempo p/ o usuário ler o IP
const unsigned long LIMITE_AP_APOS_SUCESSO_MS = 60000; // fecha o AP mesmo sem ninguém ler
const unsigned long REABRIR_AP_SEM_LINK_MS   = 60000;  // reabre o AP se o roteador sumir
const uint8_t PIN_RESET = 0;                           // botão BOOT

// ----- Estado -----
enum Estado { OCIOSO, CONECTANDO, CONECTADO, FALHA };
Estado estado = OCIOSO;

String credSsid, credSenha;      // salvas na flash
String pendSsid, pendSenha;      // tentativa em andamento

bool apAtivo = false;
bool statusEntregue = false;     // a página já leu "conectado"?
unsigned long inicioTentativa = 0;
unsigned long tempoSucesso = 0;
unsigned long tempoEntrega = 0;
unsigned long semLinkDesde = 0;
unsigned long reiniciarEm = 0;

// ----- Cache da varredura de redes -----
String redesJson = "[]";
bool escaneando = false;

Preferences prefs;
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------
String urlMascara() {
  return String("http://") + dominioMascara + "/";
}

String jsonEscape(const String& s) {
  String o;
  o.reserve(s.length() + 4);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 0x20) o += c;
  }
  return o;
}

// ---------------------------------------------------------------------
// LED RGB nativo
// ---------------------------------------------------------------------
// Aplica a cor pelo nome. Devolve false se o nome for desconhecido.
bool aplicarCor(String nome) {
  nome.trim();
  nome.toLowerCase();
  uint8_t r = 0, g = 0, b = 0;
  if      (nome == "vermelho")  { r = 1; }
  else if (nome == "verde")     { g = 1; }
  else if (nome == "azul")      { b = 1; }
  else if (nome == "amarelo")   { r = 1; g = 1; }
  else if (nome == "ciano")     { g = 1; b = 1; }
  else if (nome == "magenta")   { r = 1; b = 1; }
  else if (nome == "branco")    { r = 1; g = 1; b = 1; }
  else if (nome == "desligado") { }
  else return false;

  neopixelWrite(RGB_BUILTIN, r * BRILHO_LED, g * BRILHO_LED, b * BRILHO_LED);
  corAtual = nome;
  Serial.println("LED RGB: " + nome);
  return true;
}

// ---------------------------------------------------------------------
// Credenciais na flash
// ---------------------------------------------------------------------
void carregarCredenciais() {
  prefs.begin("wifi", false);
  credSsid  = prefs.getString("ssid", "");
  credSenha = prefs.getString("senha", "");
  prefs.end();
}

void salvarCredenciais() {
  prefs.begin("wifi", false);
  prefs.putString("ssid", credSsid);
  prefs.putString("senha", credSenha);
  prefs.end();
}

void limparCredenciais() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  credSsid = "";
  credSenha = "";
}

// ---------------------------------------------------------------------
// Rede própria (AP) e mDNS
// ---------------------------------------------------------------------
void iniciarMDNS() {
  MDNS.end();
  if (MDNS.begin(hostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS ativo: http://%s.local\n", hostname);
  }
}

void iniciarAP() {
  WiFi.mode(WIFI_AP_STA);                        // AP + cliente ao mesmo tempo
  WiFi.softAP(apSsid, apSenha);
  WiFi.softAPConfig(apIP, gateway, subnet);
  dnsServer.start(DNS_PORT, dominioDNS, apIP);
  apAtivo = true;
  Serial.print("Access Point ativo. IP: ");
  Serial.println(WiFi.softAPIP());
}

void fecharAP() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);                           // mantém só a conexão com o roteador
  apAtivo = false;
  Serial.println("Access Point desligado. Operando apenas como cliente Wi-Fi.");
}

// ---------------------------------------------------------------------
// Varredura de redes (assíncrona, para não travar o servidor)
// ---------------------------------------------------------------------
void iniciarScan() {
  if (escaneando || estado == CONECTANDO) return;
  WiFi.scanNetworks(true);
  escaneando = true;
}

void atualizarScan() {
  if (!escaneando) return;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;
  escaneando = false;
  if (n < 0) return;

  String j = "[";
  bool primeiro = true;
  for (int i = 0; i < n; i++) {
    String nome = WiFi.SSID(i);
    if (nome.length() == 0) continue;
    String chave = "\"s\":\"" + jsonEscape(nome) + "\"";
    if (j.indexOf(chave) >= 0) continue;         // ignora nomes repetidos
    if (!primeiro) j += ',';
    primeiro = false;
    j += "{" + chave + ",\"r\":" + String(WiFi.RSSI(i)) + "}";
  }
  j += "]";
  redesJson = j;
  WiFi.scanDelete();
}

// ---------------------------------------------------------------------
// Tentativa de conexão (não bloqueante)
// ---------------------------------------------------------------------
void iniciarConexao(const String& s, const String& p) {
  if (escaneando) {                              // não pode conectar durante scan
    esp_wifi_scan_stop();
    WiFi.scanDelete();
    escaneando = false;
  }
  pendSsid = s;
  pendSenha = p;
  statusEntregue = false;
  estado = CONECTANDO;
  inicioTentativa = millis();
  WiFi.begin(pendSsid.c_str(), pendSenha.length() ? pendSenha.c_str() : nullptr);
  Serial.println("Tentando conectar em: " + pendSsid);
}

// Chamada a cada volta do loop enquanto estado == CONECTANDO
void verificarConexao() {
  wl_status_t st = WiFi.status();
  unsigned long dec = millis() - inicioTentativa;

  if (st == WL_CONNECTED) {
    credSsid = pendSsid;
    credSenha = pendSenha;
    salvarCredenciais();
    iniciarMDNS();
    estado = CONECTADO;
    tempoSucesso = millis();
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());
    return;
  }

  bool falhaExplicita = (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) && dec > 4000;
  if (falhaExplicita || dec > TIMEOUT_CONEXAO_MS) {
    WiFi.disconnect(false, false);               // para de tentar, AP continua de pé
    estado = FALHA;
    Serial.println("Falha ao conectar.");
  }
}

// ---------------------------------------------------------------------
// Rotas
// ---------------------------------------------------------------------
// "/" entrega sempre a página do forno (app_html), tanto pela rede própria
// do ESP32 (AP) quanto pela rede do roteador (IP/forno.local).
void handleRoot() { server.send_P(200, "text/html; charset=utf-8", app_html); }
void handleAppCss() { server.send_P(200, "text/css; charset=utf-8", app_css); }
void handleAppJs() { server.send_P(200, "application/javascript; charset=utf-8", app_js); }

// POST /cor (cor=vermelho|verde|azul|...) -> troca a cor do LED RGB nativo
void handleCor() {
  if (!server.hasArg("cor")) {
    server.send(400, "text/plain", "Parametro 'cor' ausente");
    return;
  }
  if (!aplicarCor(server.arg("cor"))) {
    server.send(400, "text/plain", "Cor invalida");
    return;
  }
  server.send(200, "text/plain", "ok");
}

// POST /multiplicar (valor) -> devolve valor x 2
void handleMultiplicar() {
  if (!server.hasArg("valor")) {
    server.send(400, "text/plain", "Parametro 'valor' ausente");
    return;
  }
  float valor = server.arg("valor").toFloat();
  float resultado = valor * 2;
  server.send(200, "text/plain", String(resultado));
  Serial.printf("Valor recebido: %g -> Resultado: %g\n", valor, resultado);
}

// GET /redes  -> lista em cache. "?atualizar=1" dispara nova varredura.
void handleRedes() {
  if (server.hasArg("atualizar")) iniciarScan();
  String j = "{\"escaneando\":";
  j += escaneando ? "true" : "false";
  j += ",\"redes\":" + redesJson + "}";
  server.send(200, "application/json", j);
}

// POST /conectar  (ssid, senha)
void handleConectar() {
  if (!server.hasArg("ssid")) {
    server.send(400, "text/plain", "Parametro 'ssid' ausente");
    return;
  }
  String s = server.arg("ssid");
  String p = server.arg("senha");

  if (s.length() == 0 || s.length() > 32) {
    server.send(400, "text/plain", "Nome de rede invalido");
    return;
  }
  if (p.length() > 0 && (p.length() < 8 || p.length() > 63)) {
    server.send(400, "text/plain", "Senha deve ter de 8 a 63 caracteres");
    return;
  }
  if (estado == CONECTANDO) {
    server.send(409, "text/plain", "Ja existe uma tentativa em andamento");
    return;
  }

  server.send(200, "text/plain", "ok");          // responde ANTES de mexer no Wi-Fi
  iniciarConexao(s, p);
}

// GET /status
void handleStatus() {
  const char* e = "ocioso";
  if (estado == CONECTANDO) e = "conectando";
  else if (estado == CONECTADO) e = "conectado";
  else if (estado == FALHA) e = "falha";

  String j = "{\"estado\":\"" + String(e) + "\"";
  if (estado == CONECTADO) {
    j += ",\"ssid\":\"" + jsonEscape(WiFi.SSID()) + "\"";
    j += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    j += ",\"host\":\"" + String(hostname) + ".local\"";
    // A página já recebeu o novo endereço: agora o AP pode ser desligado.
    if (apAtivo && !statusEntregue) {
      statusEntregue = true;
      tempoEntrega = millis();
    }
  }
  j += ",\"apAtivo\":" + String(apAtivo ? "true" : "false") + "}";
  server.send(200, "application/json", j);
}

// POST /esquecer -> apaga credenciais e reinicia (volta a abrir o AP)
void handleEsquecer() {
  limparCredenciais();
  server.send(200, "text/plain", "ok");
  reiniciarEm = millis() + 1000;
}

// ---------------------------------------------------------------------
// Interface serial (comandos vindos do PC via USB)
// ---------------------------------------------------------------------
String bufferSerial;

// Monta e imprime (texto legível + 1 linha JSON) todas as informações
// úteis de rede: usado pelo comando "$GET.ALL".
void enviarInfoCompletaSerial() {
  const char* e = "ocioso";
  if (estado == CONECTANDO) e = "conectando";
  else if (estado == CONECTADO) e = "conectado";
  else if (estado == FALHA) e = "falha";

  bool staConectado = (WiFi.status() == WL_CONNECTED);
  String ip       = staConectado ? WiFi.localIP().toString()   : String("0.0.0.0");
  String mascara  = staConectado ? WiFi.subnetMask().toString(): String("0.0.0.0");
  String gw       = staConectado ? WiFi.gatewayIP().toString() : String("0.0.0.0");
  String dns      = staConectado ? WiFi.dnsIP().toString()     : String("0.0.0.0");
  String ssidAtual = staConectado ? WiFi.SSID() : String("");
  long   rssi     = staConectado ? WiFi.RSSI() : 0;
  String apIp     = apAtivo ? WiFi.softAPIP().toString() : String("0.0.0.0");

  // ---- Bloco legível, para ler direto no Monitor Serial ----
  Serial.println(F("========== ESP32 - INFORMACOES DE REDE =========="));
  Serial.printf ("Estado da conexao . . : %s\n", e);
  Serial.printf ("Conectado ao roteador : %s\n", staConectado ? "sim" : "nao");
  Serial.printf ("SSID conectado agora  : %s\n", ssidAtual.c_str());
  Serial.printf ("IP atual . . . . . . .: %s\n", ip.c_str());
  Serial.printf ("Mascara de subrede . .: %s\n", mascara.c_str());
  Serial.printf ("Gateway  . . . . . . .: %s\n", gw.c_str());
  Serial.printf ("DNS  . . . . . . . . .: %s\n", dns.c_str());
  Serial.printf ("RSSI (sinal) . . . . .: %ld dBm\n", rssi);
  Serial.printf ("MAC (STA)  . . . . . .: %s\n", WiFi.macAddress().c_str());
  Serial.printf ("Hostname . . . . . . .: %s.local\n", hostname);
  Serial.println(F("---- Credenciais salvas na flash ----"));
  Serial.printf ("SSID salvo . . . . . .: %s\n", credSsid.length() ? credSsid.c_str() : "(nenhum)");
  Serial.printf ("Senha salva  . . . . .: %s\n", credSenha.length() ? credSenha.c_str() : "(nenhuma)");
  Serial.println(F("---- Access Point (rede propria do ESP32) ----"));
  Serial.printf ("AP ativo . . . . . . .: %s\n", apAtivo ? "sim" : "nao");
  if (apAtivo) {
    Serial.printf("AP SSID  . . . . . . .: %s\n", apSsid);
    Serial.printf("AP Senha . . . . . . .: %s\n", apSenha);
    Serial.printf("AP IP  . . . . . . . .: %s\n", apIp.c_str());
    Serial.printf("AP Mascara subrede . .: %s\n", subnet.toString().c_str());
    Serial.printf("AP MAC . . . . . . . .: %s\n", WiFi.softAPmacAddress().c_str());
  }
  Serial.println(F("==================================================="));

  // ---- Linha única em JSON, para o PC interpretar por software ----
  String j = "{";
  j += "\"estado\":\"" + String(e) + "\",";
  j += "\"conectado\":" + String(staConectado ? "true" : "false") + ",";
  j += "\"ssidAtual\":\"" + jsonEscape(ssidAtual) + "\",";
  j += "\"ip\":\"" + ip + "\",";
  j += "\"mascaraSubrede\":\"" + mascara + "\",";
  j += "\"gateway\":\"" + gw + "\",";
  j += "\"dns\":\"" + dns + "\",";
  j += "\"rssi\":" + String(rssi) + ",";
  j += "\"macSta\":\"" + WiFi.macAddress() + "\",";
  j += "\"hostname\":\"" + String(hostname) + ".local\",";
  j += "\"ssidSalvo\":\"" + jsonEscape(credSsid) + "\",";
  j += "\"senhaSalva\":\"" + jsonEscape(credSenha) + "\",";
  j += "\"apAtivo\":" + String(apAtivo ? "true" : "false") + ",";
  j += "\"apSsid\":\"" + jsonEscape(String(apSsid)) + "\",";
  j += "\"apSenha\":\"" + jsonEscape(String(apSenha)) + "\",";
  j += "\"apIp\":\"" + apIp + "\",";
  j += "\"apMascaraSubrede\":\"" + subnet.toString() + "\",";
  j += "\"apMac\":\"" + WiFi.softAPmacAddress() + "\"";
  j += "}";
  Serial.println(j);
}

void enviarAjudaSerial() {
  Serial.println(F("Comandos disponiveis pela serial:"));
  Serial.println(F("  $GET.ALL  -> mostra IP, mascara, SSID/senha salvos, conexao atual, dados do AP, etc."));
  Serial.println(F("  $SET.COR=<cor> -> LED RGB: vermelho, verde, azul, amarelo, ciano, magenta, branco, desligado"));
  Serial.println(F("  $HELP     -> mostra esta ajuda"));
}

// Interpreta um comando já recebido por completo (sem \r\n nas pontas)
void executarComandoSerial(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  String cmdMaiuscula = cmd;
  cmdMaiuscula.toUpperCase();

  if (cmdMaiuscula == "$GET.ALL") {
    enviarInfoCompletaSerial();
  } else if (cmdMaiuscula.startsWith("$SET.COR=")) {
    String nome = cmd.substring(9);
    if (!aplicarCor(nome)) Serial.println(F("Cor invalida."));
  } else if (cmdMaiuscula == "$HELP" || cmdMaiuscula == "$GET.HELP") {
    enviarAjudaSerial();
  } else {
    Serial.print(F("Comando desconhecido: "));
    Serial.println(cmd);
    enviarAjudaSerial();
  }
}

// Lê bytes disponíveis na serial e monta uma linha até '\n' ou '\r'.
// Chamar a cada volta do loop().
void processarSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufferSerial.length() > 0) {
        executarComandoSerial(bufferSerial);
        bufferSerial = "";
      }
    } else {
      bufferSerial += c;
      if (bufferSerial.length() > 128) bufferSerial = "";  // protege contra lixo/overflow
    }
  }
}

// ---------------------------------------------------------------------
// Portal cativo (só faz sentido enquanto o AP está ativo)
// ---------------------------------------------------------------------
void handleCaptivePortalRedirect() {
  if (!apAtivo) {
    server.send(404, "text/plain", "Nao encontrado");
    return;
  }
  server.sendHeader("Location", urlMascara(), true);
  server.send(302, "text/plain", "");
}

void handleNotFound() { handleCaptivePortalRedirect(); }

// ---------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_RESET, INPUT_PULLUP);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0);           // LED começa apagado

  WiFi.persistent(false);                        // credenciais ficam só no Preferences
  WiFi.setHostname(hostname);
  WiFi.setAutoReconnect(true);

  // Botão BOOT segurado por 3 s ao ligar = apaga a rede salva
  if (digitalRead(PIN_RESET) == LOW) {
    unsigned long t = millis();
    while (digitalRead(PIN_RESET) == LOW && millis() - t < 3000) delay(10);
    if (digitalRead(PIN_RESET) == LOW) {
      limparCredenciais();
      Serial.println("Credenciais apagadas.");
    }
  }

  carregarCredenciais();

  // 1) Já existe rede salva? Tenta conectar direto, sem abrir o AP.
  bool conectou = false;
  if (credSsid.length() > 0) {
    Serial.println("Conectando na rede salva: " + credSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(credSsid.c_str(), credSenha.length() ? credSenha.c_str() : nullptr);
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < TIMEOUT_BOOT_MS) delay(250);
    conectou = (WiFi.status() == WL_CONNECTED);
  }

  if (conectou) {
    estado = CONECTADO;
    iniciarMDNS();
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    // 2) Sem rede salva (ou falhou): abre o portal de configuração.
    WiFi.disconnect(false, false);
    iniciarAP();
    iniciarScan();                               // varre uma vez, antes de haver clientes
  }

  // Rotas da aplicação
  server.on("/", HTTP_GET, handleRoot);
  server.on("/app.css", HTTP_GET, handleAppCss);
  server.on("/app.js", HTTP_GET, handleAppJs);
  server.on("/multiplicar", HTTP_POST, handleMultiplicar);
  server.on("/cor", HTTP_POST, handleCor);
  server.on("/redes", HTTP_GET, handleRedes);
  server.on("/conectar", HTTP_POST, handleConectar);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/esquecer", HTTP_POST, handleEsquecer);

  // Detecção de portal cativo (Apple / Android / Windows)
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/library/test/success.html", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/generate_204", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/gen_204", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/redirect", HTTP_GET, handleCaptivePortalRedirect);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor web iniciado.");
  Serial.println(F("Interface serial pronta. Envie $GET.ALL para ver IP/redes/senhas. Envie $HELP para ajuda."));
}

void loop() {
  processarSerial();
  if (apAtivo) dnsServer.processNextRequest();
  server.handleClient();
  atualizarScan();

  // Tentativa de conexão em andamento
  if (estado == CONECTANDO) verificarConexao();

  // Conectou com o AP ainda aberto: fecha depois que a página leu o resultado
  if (estado == CONECTADO && apAtivo) {
    bool leuEEsperou = statusEntregue && (millis() - tempoEntrega > ESPERA_APOS_ENTREGA_MS);
    bool passouLimite = (millis() - tempoSucesso > LIMITE_AP_APOS_SUCESSO_MS);
    if (leuEEsperou || passouLimite) fecharAP();
  }

  // AP reaberto e o roteador voltou sozinho: trata como conectado
  if (estado == OCIOSO && apAtivo && credSsid.length() > 0 && WiFi.status() == WL_CONNECTED) {
    estado = CONECTADO;
    tempoSucesso = millis();
    statusEntregue = false;
    iniciarMDNS();
  }

  // Sem AP e sem link com o roteador por muito tempo: reabre o AP
  if (!apAtivo) {
    if (WiFi.status() == WL_CONNECTED) {
      semLinkDesde = 0;
    } else if (semLinkDesde == 0) {
      semLinkDesde = millis();
    } else if (millis() - semLinkDesde > REABRIR_AP_SEM_LINK_MS) {
      Serial.println("Roteador indisponivel. Reabrindo a rede propria.");
      iniciarAP();
      estado = OCIOSO;
      semLinkDesde = 0;
    }
  }

  if (reiniciarEm && millis() > reiniciarEm) ESP.restart();
}
