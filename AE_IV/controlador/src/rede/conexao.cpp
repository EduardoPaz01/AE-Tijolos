#include "conexao.hpp"

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
    lcd_print_ip(WiFi.localIP());
    return;
  }

  bool falhaExplicita = (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) && dec > 4000;
  if (falhaExplicita || dec > TIMEOUT_CONEXAO_MS) {
    WiFi.disconnect(false, false);               // para de tentar, AP continua de pé
    estado = FALHA;
    Serial.println("Falha ao conectar.");
  }
}