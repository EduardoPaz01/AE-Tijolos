#include "rede.hpp"

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

void loop_controle() {
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
