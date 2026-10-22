#include "rede_propria.hpp"

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