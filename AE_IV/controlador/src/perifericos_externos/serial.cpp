#include "serial.hpp"


// ---------------------------------------------------------------------
// Interface serial (comandos vindos do PC via USB)
// ---------------------------------------------------------------------

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