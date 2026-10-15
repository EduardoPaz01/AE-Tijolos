#pragma once
#include <pgmspace.h>

// ----- Página principal do forno (IHM / SCADA) -----
// Servida em "/" para clientes que chegam pela rede do roteador.
const char app_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>IHM - Parametrização Térmica do Forno - SCADA View</title>
<link rel="stylesheet" href="/app.css">
</head>
<body>

  <div class="device">
    <header class="topbar">
      <div class="topbar-left">
        <button class="back-btn" id="backBtn" style="display:none;">← Voltar</button>
        <span class="brand" id="brandText">Forno · Olaria Modesto</span>
        <button class="operator-tag" id="operatorTag" style="display:none;" title="Trocar operador"></button>
      </div>
      <span class="wifi" id="connectionStatus"><span class="dot" id="dbDot"></span>ESTADO DB: CONECTANDO...</span>
    </header>

    <!-- ===================== TELA 0 · LOGIN / OPERADOR ===================== -->
    <div id="screenLogin" class="screen active">
      <div class="login-screen">
        <div class="login-card">
          <p class="eyebrow" style="margin:0;">Login</p>
          <h1 style="font-size:23px; margin:0;">Quem está operando o forno?</h1>
          <p class="sub" style="margin:0; font-size:15.5px;">Escolha seu usuário para entrar.</p>

          <select class="login-select" id="userSelect"></select>
          <button class="btn-primary" id="loginBtn" style="box-shadow:none;">Entrar →</button>
          <p class="login-status" id="loginStatus">Carregando usuários do banco...</p>

          <div class="login-divider">ou crie um usuário novo</div>
          <div class="login-new">
            <input type="text" id="newUserName" placeholder="Nome do usuário" maxlength="45" />
            <input type="password" id="newUserPass" placeholder="Senha" maxlength="45" />
            <button class="recipe-btn" id="newUserBtn" style="align-self:flex-start;">+ Cadastrar e entrar</button>
          </div>
        </div>
      </div>
    </div>

    <!-- ===================== TELA 1 · INÍCIO ===================== -->
    <div id="screenHome" class="screen">
      <div class="home-inner">
        <p class="eyebrow">Início</p>
        <h1>O que você quer fazer?</h1>
        <p class="sub">Monte uma curva nova, abra uma que já foi salva ou acompanhe o forno ao vivo.</p>

        <div class="option-grid">
          <button class="option-card" id="goCustomBtn">
            <span class="option-icon">📈</span>
            <span class="option-title">Receita Personalizada</span>
            <span class="option-desc">Você informa a temperatura em que o forno está e depois cada temperatura que quer atingir, com o tempo para chegar nela.</span>
            <span class="option-cta">Criar receita →</span>
          </button>
          <button class="option-card" id="goSavedBtn">
            <span class="option-icon">🗄️</span>
            <span class="option-title">Receitas Salvas no Banco</span>
            <span class="option-desc">Veja as curvas que já foram salvas, por você ou por outros operadores.</span>
            <span class="option-cta">Ver receitas salvas →</span>
          </button>
          <button class="option-card" id="goLiveBtn">
            <span class="option-icon">📡</span>
            <span class="option-title">Acompanhar gráfico em tempo real</span>
            <span class="option-desc">Mostra a temperatura do termopar ao vivo, lida direto do ESP32.</span>
            <span class="option-cta">Abrir gráfico →</span>
          </button>
        </div>
      </div>
    </div>

    <!-- ===================== TELA 2 · RECEITAS PRONTAS ===================== -->
    <div id="screenRecipes" class="screen">
      <div class="recipes-inner">
        <p class="eyebrow">Biblioteca de Receitas</p>
        <h1>Receitas Disponíveis</h1>
        <p class="sub">Ao selecionar uma receita, os patamares são carregados automaticamente na tela de curva térmica para revisão.</p>
        <div class="recipe-grid" id="recipeGrid"></div>
      </div>
    </div>

    <!-- ===================== TELA 2B · RECEITAS SALVAS NO BANCO (LEITURA) ===================== -->
    <div id="screenSaved" class="screen">
      <div class="recipes-inner">
        <p class="eyebrow">Banco de dados</p>
        <h1>Receitas Salvas no Banco</h1>
        <p class="sub" id="savedSub">Buscando as receitas salvas...</p>


        <div class="recipe-grid" id="savedGrid"></div>
      </div>
    </div>


    <!-- ===================== TELA 4 · GRÁFICO EM TEMPO REAL (ESP32) ===================== -->
    <div id="screenLive" class="screen">
      <div class="recipes-inner">
        <p class="eyebrow">Termopar</p>
        <h1>Gráfico em Tempo Real</h1>
        <p class="sub">Os dados vêm direto do ESP32, sem passar pelo banco. O gráfico vai sendo desenhado a cada leitura.</p>

        <div class="live-row">
          <select class="login-select" id="corSelect" style="max-width:200px;">
            <option value="vermelho">Vermelho</option>
            <option value="verde">Verde</option>
            <option value="azul">Azul</option>
            <option value="amarelo">Amarelo</option>
            <option value="ciano">Ciano</option>
            <option value="magenta">Magenta</option>
            <option value="branco">Branco</option>
            <option value="desligado">Desligado</option>
          </select>
          <button class="recipe-btn" id="corSetBtn" style="margin-top:0;">Definir cor</button>
          <button class="export-btn" id="liveClearBtn">Limpar gráfico</button>
          <span class="export-status" id="corStatus"></span>
        </div>
        <div class="live-status" id="liveStatus">Conectando ao ESP32...</div>

        <div class="live-value">
          <span>Temperatura atual<b id="liveNow">—</b></span>
          <span>Máxima<b id="liveMax">—</b></span>
          <span>Leituras<b id="liveCount">0</b></span>
        </div>

        <div class="chart-wrap">
          <div class="chart-title-row"><span class="chart-title">Temperatura × tempo</span></div>
          <div class="canvas-container live-chart"><canvas id="liveChart"></canvas></div>
        </div>
      </div>
    </div>

    <!-- ===================== TELA 3 · BUILDER (CURVA TÉRMICA) ===================== -->
    <div id="screenBuilder" class="screen">
      <div class="layout-grid">
        <aside class="sidebar-controls">
          <div class="header">
            <p class="eyebrow" id="stepCount">Sessão em preparação</p>
            <h1>Curva do Forno</h1>
            <p class="sub" id="subText">Preparando...</p>
          </div>

          <div class="form" id="formArea">
            <div id="nextField">
              <div class="started-header">
                <span>Temperatura inicial (P0): <b id="p0Display">não definida</b></span>
                <button class="restart-link" id="restartBtn" title="Limpar os pontos desta sessão">↺ limpar pontos</button>
              </div>

              <div class="field" style="margin-top:10px;">
                <label id="nextLabel">Temperatura inicial (P0)</label>
                <div class="input-row">
                  <input type="text" id="nextTempInput" inputmode="decimal" maxlength="8" placeholder="ex: 100" />
                  <span class="unit">°C</span>
                </div>
                <span class="field-hint" id="nextHint">Digite a temperatura que o forno está marcando agora.</span>
                <span class="field-warning" id="tempWarning">Não é possível criar uma reta de decréscimo aqui.</span>
              </div>

              <div class="btn-row" id="btnRowTop">
                <button class="add-segment" id="addBtn">+ ADICIONAR PONTO</button>
                <button class="finish-btn" id="finishBtn">FINALIZAR CURVA</button>
              </div>

              <div id="segmentsScroll"><div id="segmentsList"></div></div>
            </div>
          </div>

          <div class="footer">
            <p class="hint" id="hintText">Digite a primeira temperatura para começar.</p>
            <button class="btn-primary" id="mainBtn" disabled>Curva incompleta</button>
          </div>
        </aside>

        <main class="main-results" id="resultsArea" style="display:none;">
          <div id="summaryBanner" style="display:none;">
            <div class="summary-banner">
              <span>✓ Curva salva — <span id="finalCount">0</span> pontos</span>
              <button class="edit-link" id="editBtn">editar novamente</button>
            </div>
          </div>

          <div class="chart-wrap">
            <div class="chart-title-row">
              <span class="chart-title">Curva térmica — temperatura × tempo</span>
              <span class="chart-badge preview" id="chartBadge">PRÉVIA</span>
            </div>
            <div class="canvas-container">
              <canvas id="chart"></canvas>
            </div>
            <p class="cool-note" id="coolNote" style="display:none;">O resfriamento do forno não aparece no gráfico, porque o tempo dele depende da última temperatura e do próprio forno.</p>
          </div>

          <div class="table-wrap">
            <table class="summary-table" id="summaryTable">
              <thead>
                <tr><th>Ponto</th><th>Transição</th><th>ΔT</th><th>Duração</th><th>Taxa (°C/min)</th><th>Tendência</th></tr>
              </thead>
              <tbody id="summaryTbody"></tbody>
            </table>
          </div>

          <div class="totals">
            <span>Tempo total: <b id="totalTime">0 min</b></span>
            <span>Pontos definidos: <b id="totalSteps">0</b></span>
          </div>

          <div class="export-row">
            <button class="export-btn pdf" id="exportPdfBtn">⬇ Exportar dados em PDF</button>
            <span class="export-status" id="exportPdfStatus"></span>
          </div>
        </main>
      </div>
    </div>
  </div>


<!-- Bibliotecas de exportação (PDF) -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/jspdf/2.5.1/jspdf.umd.min.js"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/jspdf-autotable/3.5.31/jspdf.plugin.autotable.min.js"></script>

<script src="/app.js"></script>
</body>
</html>
)rawliteral";
