#include "rotas.hpp"
// ---------------------------------------------------------------------
// Rotas
// ---------------------------------------------------------------------
// "/" entrega sempre a página do forno (app_html), tanto pela rede própria
// do ESP32 (AP) quanto pela rede do roteador (IP/forno.local).

void handleRoot() { 
    server.send_P(200, "text/html; charset=utf-8", app_html); 
}

void handleAppCss() { 
    server.send_P(200, "text/css; charset=utf-8", app_css); 
}

void handleAppJs() { 
    server.send_P(200, "application/javascript; charset=utf-8", app_js); 
}

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

void handleRedes() {
  if (server.hasArg("atualizar")) iniciarScan();
  String j = "{\"escaneando\":";
  j += escaneando ? "true" : "false";
  j += ",\"redes\":" + redesJson + "}";
  server.send(200, "application/json", j);
}

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

void handleEsquecer() {
  limparCredenciais();
  server.send(200, "text/plain", "ok");
  reiniciarEm = millis() + 1000;
}