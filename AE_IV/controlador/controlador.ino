#include "controlador.hpp"

void setup() {
  Serial.begin(115200);
  setup_LCD();
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
    lcd_print_ip(WiFi.localIP());
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

  setup_temporizador();
  setup_pinos();
  setup_ADC();

  alpha = kp * (1.0 + (T / (2.0 * ti)));
  beta  = kp * ((T / (2.0 * ti)) - 1.0);

  // Receita: 0 -> 150°C (3,5°C/min), patamar de mesma duração, 150 -> 0°C
  double t_rampa = TEMP_MAX_RECEITA / (TAXA_RAMPA_C_MIN / 60.0);   // segundos
  receita = {
    { 0.0,          0.0 },
    { t_rampa,      TEMP_MAX_RECEITA },
    { 2.0 * t_rampa, TEMP_MAX_RECEITA },
    { 3.0 * t_rampa, 0.0 }
  };

  // Controle assíncrono: roda no core que NÃO executa o loop()
  const BaseType_t coreControle = (ARDUINO_RUNNING_CORE == 0) ? 1 : 0;
  xTaskCreatePinnedToCore(taskControle, "controle", 8192, nullptr, 1, &hTaskControle, coreControle);
}

void taskControle(void* param) {
  for (;;) {
    loop_controle(receita);
    vTaskDelay(pdMS_TO_TICKS(10));   // cede a CPU (substitui o antigo delay)
  }
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
    lcd_print_ip(WiFi.localIP());
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
