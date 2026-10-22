#pragma once
#include <pgmspace.h>

// ----- JavaScript da página principal do forno -----
const char app_js[] PROGMEM = R"rawliteral(
/**
 * CONFIGURAÇÃO DA API
 * Deixe API_BASE vazio ('') se este arquivo HTML estiver salvo na MESMA pasta
 * dos arquivos .php (usuario_post.php, receita_post.php, etc — a pasta "Banco de Dados").
 * Se um dia o HTML for hospedado em outro endereço/porta, troque para algo como
 * 'http://localhost/banco-de-dados/' (com a barra final).
 */
const CONFIG = {
  API_BASE: 'http://localhost:81/'
};

/**
 * ADAPTADOR DE BANCO DE DADOS (REAL)
 * Camada isolada de comunicação com o backend PHP/MySQL do banco "forno".
 * Mapeamento usado (ver banco_createStatement.sql):
 *   - "Ensaio" desta tela (curva sendo montada) = tabela Receita, com seus Pontos.
 *     A tabela real "Ensaio/Medida" do banco é para REGISTROS de execuções já
 *     feitas no forno (temperatura real x referência) — não é usada por esta tela ainda.
 *   - Ponto.tempo no banco guarda o tempo ACUMULADO (minutos desde o início),
 *     enquanto aqui na tela cada segmento guarda a DURAÇÃO até aquele ponto.
 *     A conversão duração → tempo acumulado é feita em recomputeAndSyncDurations().
 */
const DatabaseAdapter = {
  setNetworkState(isActive, isError) {
    const text = document.getElementById('connectionStatus');
    if (isError) {
      text.innerHTML = '<span class="dot" id="dbDot" style="background:var(--danger);box-shadow:0 0 6px var(--danger);"></span>ERRO DE CONEXÃO COM O BANCO';
      text.style.color = 'var(--danger)';
    } else if (isActive) {
      text.innerHTML = '<span class="dot syncing" id="dbDot"></span>SINCRONIZANDO...';
      text.style.color = 'var(--amber)';
    } else {
      text.innerHTML = '<span class="dot" id="dbDot"></span>ESTADO DB: CONECTADO';
      text.style.color = 'var(--ok)';
    }
  },

  async _call(endpoint, { method = 'GET', body } = {}) {
    let res, data;
    try {
      res = await fetch(CONFIG.API_BASE + endpoint, {
        method,
        headers: body ? { 'Content-Type': 'application/json' } : undefined,
        body: body ? JSON.stringify(body) : undefined
      });
    } catch (netErr) {
      this.setNetworkState(false, true);
      throw new Error(`Não foi possível alcançar ${endpoint}. O servidor PHP está rodando e este arquivo está na mesma pasta dos scripts?`);
    }
    try {
      data = await res.json();
    } catch (parseErr) {
      this.setNetworkState(false, true);
      throw new Error(`Resposta inválida de ${endpoint} (esperado JSON).`);
    }
    if (!res.ok || (data && data.success === false)) {
      this.setNetworkState(false, true);
      throw new Error((data && data.error) || `Erro ao chamar ${endpoint} (HTTP ${res.status})`);
    }
    return data;
  },

  // ---- Usuário (leitura + cadastro) ----
  // GET usuario_consult.php
  async listUsuarios() {
    return this._call('usuario_consult.php');
  },
  // POST usuario_post.php
  async createUsuario(nome, senha) {
    return this._call('usuario_post.php', { method: 'POST', body: { nome, senha } });
  },

  // ---- Receita + Ponto (leitura, para a tela "Receitas Salvas no Banco") ----
  // GET receita_consult.php
  async listReceitas() {
    return this._call('receita_consult.php');
  },
  // GET ponto_consult.php
  async listPontos() {
    return this._call('ponto_consult.php');
  },

  // ---- Ciclo de vida de uma curva (Receita + seus Pontos) ----

  // POST receita_post.php  → cria a Receita (equivalente ao antigo "ensaio" local)
  async createAssay(idUsuario, titulo) {
    this.setNetworkState(true);
    try {
      const data = await this._call('receita_post.php', { method: 'POST', body: { idUsuario, titulo } });
      this.setNetworkState(false);
      return { id: data.idReceita, p0: 0 };
    } catch (e) { this.setNetworkState(false, true); throw e; }
  },

  // POST ponto_post.php → cria um Ponto (temperatura alvo) da Receita
  async createSegment(assayId, tempTarget, stepOrder) {
    this.setNetworkState(true);
    try {
      const data = await this._call('ponto_post.php', {
        method: 'POST',
        body: { idReceita: assayId, temperatura: tempTarget, nOrdem: stepOrder }
      });
      this.setNetworkState(false);
      return { dbId: data.idPonto };
    } catch (e) { this.setNetworkState(false, true); throw e; }
  },

  // POST ponto_update.php → grava o tempo ACUMULADO (min) do ponto
  async updateSegmentDuration(segmentDbId, cumulativeMinutes) {
    await this._call('ponto_update.php', { method: 'POST', body: { idPonto: segmentDbId, tempo: cumulativeMinutes } });
  },

  // POST ponto_delete.php
  async deleteSegment(segmentDbId) {
    this.setNetworkState(true);
    try {
      await this._call('ponto_delete.php', { method: 'POST', body: { idPonto: segmentDbId } });
      this.setNetworkState(false);
      return true;
    } catch (e) { this.setNetworkState(false, true); throw e; }
  },

  // POST receita_delete.php (cascata: apaga os Pontos — e Ensaios/Medidas — dessa receita)
  async purgeAssay(assayId) {
    this.setNetworkState(true);
    try {
      await this._call('receita_delete.php', { method: 'POST', body: { idReceita: assayId } });
      this.setNetworkState(false);
      return true;
    } catch (e) { this.setNetworkState(false, true); throw e; }
  }
};

// ---- Receitas pré-modeladas ----
const RECIPES = [
  {
    id: 'biscoito',
    name: 'Queima de Biscoito',
    tag: 'Padrão · Cerâmica crua',
    desc: 'Primeira queima do barro cru. Subida lenta no início para evitar trincas por umidade residual, patamar final para sinterização.',
    points: [
      { temp: 150, duration: 90 },
      { temp: 350, duration: 60 },
      { temp: 700, duration: 90 },
      { temp: 950, duration: 80 },
      { temp: 950, duration: 30 }
    ]
  },
  {
    id: 'esmalte',
    name: 'Queima de Esmalte / Vidrado',
    tag: 'Segunda queima',
    desc: 'Curva para fusão do esmalte sobre peça já biscoitada, com patamar de estabilização no pico para vitrificação uniforme.',
    points: [
      { temp: 200, duration: 60 },
      { temp: 500, duration: 70 },
      { temp: 850, duration: 60 },
      { temp: 1060, duration: 90 },
      { temp: 1060, duration: 20 }
    ]
  },
  {
    id: 'secagem',
    name: 'Secagem Lenta / Pré-Aquecimento',
    tag: 'Peças grandes ou espessas',
    desc: 'Rampa bem lenta e baixa, indicada para peças de parede grossa que ainda retêm bastante umidade antes da queima principal.',
    points: [
      { temp: 80, duration: 120 },
      { temp: 150, duration: 90 },
      { temp: 250, duration: 60 }
    ]
  },
  {
    id: 'rapida',
    name: 'Queima Rápida',
    tag: 'Peças pequenas · ciclo curto',
    desc: 'Ciclo enxuto para peças pequenas ou finas, sem grandes riscos de choque térmico, priorizando tempo total de forno.',
    points: [
      { temp: 300, duration: 40 },
      { temp: 600, duration: 40 },
      { temp: 900, duration: 50 }
    ]
  }
];

// ---- Sessão do operador (usuário logado no banco) ----
let session = { idUsuario: null, nomeUsuario: null };

// ---- Estado da aplicação ----
let appState = {
  currentAssayId: null,
  p0Value: null, // temperatura inicial: o operador informa (varia a cada dia)
  p0DbId: null,
  segments: [],
  nextId: 1,
  finalized: false,
  started: false,
  source: null // 'custom' | id da receita | 'db:<idReceita>' (carregada do banco)
};
let currentScreen = 'login';

const UI = {
  segmentsList: document.getElementById('segmentsList'),
  addBtn: document.getElementById('addBtn'),
  finishBtn: document.getElementById('finishBtn'),
  editBtn: document.getElementById('editBtn'),
  nextField: document.getElementById('nextField'),
  p0Display: document.getElementById('p0Display'),
  nextLabel: document.getElementById('nextLabel'),
  nextHint: document.getElementById('nextHint'),
  tempWarning: document.getElementById('tempWarning'),
  nextTempInput: document.getElementById('nextTempInput'),
  restartBtn: document.getElementById('restartBtn'),
  mainBtn: document.getElementById('mainBtn'),
  hintText: document.getElementById('hintText'),
  subText: document.getElementById('subText'),
  stepCount: document.getElementById('stepCount'),
  summaryBanner: document.getElementById('summaryBanner'),
  resultsArea: document.getElementById('resultsArea'),
  summaryTbody: document.getElementById('summaryTbody'),
  totalTime: document.getElementById('totalTime'),
  totalSteps: document.getElementById('totalSteps'),
  finalCount: document.getElementById('finalCount'),
  chartBadge: document.getElementById('chartBadge'),
  coolNote: document.getElementById('coolNote'),
  brandText: document.getElementById('brandText'),
  backBtn: document.getElementById('backBtn'),
  operatorTag: document.getElementById('operatorTag'),
  userSelect: document.getElementById('userSelect'),
  loginBtn: document.getElementById('loginBtn'),
  loginStatus: document.getElementById('loginStatus'),
  newUserName: document.getElementById('newUserName'),
  newUserPass: document.getElementById('newUserPass'),
  newUserBtn: document.getElementById('newUserBtn'),
  savedGrid: document.getElementById('savedGrid'),
  savedSub: document.getElementById('savedSub'),
  exportPdfBtn: document.getElementById('exportPdfBtn'),
  exportPdfStatus: document.getElementById('exportPdfStatus')
};

// ==================== LOGIN / OPERADOR (leitura + cadastro de Usuario) ====================
async function loadUsuarios(selectIdAfter) {
  UI.loginStatus.className = 'login-status';
  UI.loginStatus.textContent = 'Carregando usuários do banco...';
  try {
    const usuarios = await DatabaseAdapter.listUsuarios();
    if (!Array.isArray(usuarios) || usuarios.length === 0 || usuarios[0].error) {
      UI.userSelect.innerHTML = '';
      UI.loginStatus.textContent = 'Nenhum usuário cadastrado ainda. Cadastre um abaixo para começar.';
      return;
    }
    UI.userSelect.innerHTML = usuarios.map(u => `<option value="${u.idUsuario}">${u.nome} (idUsuario ${u.idUsuario})</option>`).join('');
    if (selectIdAfter) UI.userSelect.value = String(selectIdAfter);
    UI.loginStatus.textContent = `${usuarios.length} usuário(s) encontrados na tabela Usuario.`;
    DatabaseAdapter.setNetworkState(false);
  } catch (e) {
    UI.loginStatus.className = 'login-status err';
    UI.loginStatus.textContent = '⚠ ' + e.message;
    DatabaseAdapter.setNetworkState(false, true);
  }
}

async function doLogin() {
  const idUsuario = parseInt(UI.userSelect.value);
  if (!idUsuario) {
    UI.loginStatus.className = 'login-status err';
    UI.loginStatus.textContent = '⚠ Selecione um usuário da lista (ou cadastre um novo abaixo).';
    return;
  }
  session.idUsuario = idUsuario;
  session.nomeUsuario = UI.userSelect.options[UI.userSelect.selectedIndex].textContent.split(' (idUsuario')[0];
  UI.operatorTag.textContent = '👤 ' + session.nomeUsuario;
  UI.operatorTag.style.display = 'inline-block';
  renderRecipeGrid();
  showScreen('home');
}

async function doCreateUser() {
  const nome = UI.newUserName.value.trim();
  const senha = UI.newUserPass.value;
  if (!nome || !senha) {
    UI.loginStatus.className = 'login-status err';
    UI.loginStatus.textContent = '⚠ Informe nome e senha para cadastrar o novo usuário.';
    return;
  }
  UI.newUserBtn.disabled = true;
  UI.newUserBtn.textContent = 'CADASTRANDO...';
  try {
    const res = await DatabaseAdapter.createUsuario(nome, senha);
    UI.newUserName.value = '';
    UI.newUserPass.value = '';
    await loadUsuarios(res.idUsuario);
    doLogin();
  } catch (e) {
    UI.loginStatus.className = 'login-status err';
    UI.loginStatus.textContent = '⚠ ' + e.message;
  } finally {
    UI.newUserBtn.disabled = false;
    UI.newUserBtn.textContent = '+ Cadastrar e entrar';
  }
}

function logout() {
  session = { idUsuario: null, nomeUsuario: null };
  UI.operatorTag.style.display = 'none';
  showScreen('login');
  loadUsuarios();
}

// ==================== NAVEGAÇÃO ENTRE TELAS ====================
function showScreen(name) {
  document.querySelectorAll('.screen').forEach(s => s.classList.remove('active'));
  document.getElementById('screen' + name.charAt(0).toUpperCase() + name.slice(1)).classList.add('active');
  currentScreen = name;
  UI.backBtn.style.display = (name === 'home' || name === 'login') ? 'none' : 'inline-block';
  const titles = { login: 'Forno · Identificação', home: 'Forno · Olaria Modesto', recipes: 'Forno · Escolha de Receita', saved: 'Forno · Receitas Salvas', builder: 'Forno · Curva Térmica', live: 'Forno · Gráfico em Tempo Real' };
  UI.brandText.textContent = titles[name] || 'Forno · Olaria Modesto';
  if (name === 'saved') loadSavedRecipes();
  if (name !== 'live' && typeof stopLive === 'function') stopLive();
}

async function goHome() {
  // Curvas abertas da tela "Receitas Salvas" (source 'db:<id>') já existiam no banco,
  // então nunca são apagadas ao sair. Só descartamos o que foi criado nesta sessão.
  const veioDoBanco = String(appState.source || '').startsWith('db:');
  if (appState.currentAssayId && !appState.finalized && !veioDoBanco) {
    await DatabaseAdapter.purgeAssay(appState.currentAssayId);
  }
  resetLocalState();
  showScreen('home');
}

function resetLocalState() {
  appState = { currentAssayId: null, p0Value: null, p0DbId: null, segments: [], nextId: 1, finalized: false, started: false, source: null };
  UI.nextTempInput.value = '';
  UI.nextTempInput.disabled = false;
  UI.tempWarning.style.display = 'none';
  UI.summaryBanner.style.display = 'none';
  UI.resultsArea.style.display = 'none';
  UI.addBtn.style.display = 'flex';
  UI.finishBtn.style.display = 'flex';
  UI.mainBtn.disabled = true;
  UI.mainBtn.textContent = 'Curva incompleta';
  UI.hintText.style.color = 'var(--dim)';
  setChartBadge(false);
}

async function enterCustom() {
  resetLocalState();
  showScreen('builder');
  UI.stepCount.textContent = 'Sessão em preparação';
  UI.subText.textContent = 'Criando a receita no banco...';
  UI.hintText.textContent = 'Preparando...';

  try {
    const titulo = 'Receita personalizada - ' + new Date().toLocaleString('pt-BR');
    const assay = await DatabaseAdapter.createAssay(session.idUsuario, titulo);
    appState.currentAssayId = assay.id;
    appState.p0Value = null;
    appState.started = true;
    appState.source = 'custom';

    UI.subText.textContent = 'Digite a temperatura inicial do forno e depois adicione os pontos da curva.';
    renderSegments();
    UI.nextTempInput.focus();
  } catch (e) {
    UI.subText.textContent = '⚠ ' + e.message;
    UI.hintText.textContent = 'Não foi possível criar a receita no banco.';
  }
}

function openRecipes() {
  showScreen('recipes');
}

async function selectRecipe(recipeId) {
  const recipe = RECIPES.find(r => r.id === recipeId);
  if (!recipe) return;

  const btn = document.querySelector(`[data-recipe="${recipeId}"] .recipe-btn`);
  const original = btn.textContent;
  btn.disabled = true;
  btn.textContent = 'CARREGANDO...';

  try {
    resetLocalState();
    const assay = await DatabaseAdapter.createAssay(session.idUsuario, recipe.name);
    appState.currentAssayId = assay.id;
    appState.p0Value = 0;
    appState.started = true;
    appState.source = recipe.id;

    let cum = 0;
    for (const pt of recipe.points) {
      const res = await DatabaseAdapter.createSegment(appState.currentAssayId, pt.temp, appState.segments.length + 1);
      cum += pt.duration;
      appState.segments.push({ id: appState.nextId++, dbId: res.dbId, temp: pt.temp, duration: pt.duration });
      await DatabaseAdapter.updateSegmentDuration(res.dbId, cum);
    }

    showScreen('builder');
    UI.subText.textContent = `Receita "${recipe.name}" carregada. Revise os patamares e confirme para exportar ao CLP.`;
    renderSegments();
  } catch (e) {
    alert('⚠ Falha ao carregar receita no banco: ' + e.message);
  } finally {
    btn.disabled = false;
    btn.textContent = original;
  }
}

// ==================== RECEITAS SALVAS NO BANCO (LEITURA: receita_consult + ponto_consult) ====================
async function loadSavedRecipes() {
  UI.savedGrid.innerHTML = '';
  UI.savedSub.textContent = 'Buscando as receitas salvas...';
  try {
    const [receitas, pontos] = await Promise.all([
      DatabaseAdapter.listReceitas(),
      DatabaseAdapter.listPontos()
    ]);
    const receitasValidas = (Array.isArray(receitas) ? receitas : []).filter(r => r && !r.error);
    const pontosValidos = (Array.isArray(pontos) ? pontos : []).filter(p => p && !p.error);

    if (receitasValidas.length === 0) {
      UI.savedSub.textContent = 'Ainda não tem nenhuma receita salva.';
      UI.savedGrid.innerHTML = '<div class="saved-empty">Nada por aqui ainda. Crie uma receita personalizada e ela vai aparecer nesta lista.</div>';
      return;
    }

    UI.savedSub.textContent = `${receitasValidas.length} receita(s) salva(s), de todos os operadores.`;

    UI.savedGrid.innerHTML = receitasValidas.map(r => {
      const meusPontos = pontosValidos
        .filter(p => String(p.idReceita) === String(r.idReceita))
        .sort((a, b) => parseFloat(a.tempo ?? 0) - parseFloat(b.tempo ?? 0));
      const picos = meusPontos.map(p => parseFloat(p.temperatura)).filter(n => !isNaN(n));
      const maxTemp = picos.length ? Math.max(...picos) : null;
      const tempos = meusPontos.map(p => parseFloat(p.tempo)).filter(n => !isNaN(n));
      const totalTime = tempos.length ? Math.max(...tempos) : null;
      return `
        <div class="saved-card" data-saved-receita="${r.idReceita}">
          <span class="recipe-tag">idReceita ${r.idReceita} · idUsuario ${r.idUsuario}</span>
          <span class="recipe-name">${r.titulo || '(sem título)'}</span>
          <div class="recipe-meta">
            <span>Pico: <b>${maxTemp !== null ? maxTemp + '°C' : '—'}</b></span>
            <span>Tempo total: <b>${totalTime !== null ? totalTime + ' min' : '—'}</b></span>
            <span>Pontos: <b>${meusPontos.filter(p => !isP0Point(p)).length}</b></span>
          </div>
          <div class="saved-actions">
            <button class="saved-load-btn" data-saved-load="${r.idReceita}">Ver / editar →</button>
            <button class="saved-del-btn" data-saved-del="${r.idReceita}">Excluir</button>
          </div>
        </div>`;
    }).join('');

    UI.savedGrid.querySelectorAll('[data-saved-load]').forEach(btn => {
      btn.addEventListener('click', () => loadSavedIntoBuilder(parseInt(btn.getAttribute('data-saved-load')), pontosValidos, receitasValidas));
    });
    UI.savedGrid.querySelectorAll('[data-saved-del]').forEach(btn => {
      btn.addEventListener('click', () => deleteSavedRecipe(parseInt(btn.getAttribute('data-saved-del')), btn));
    });
  } catch (e) {
    UI.savedSub.textContent = '⚠ ' + e.message;
    UI.savedGrid.innerHTML = '';
  }
}

async function deleteSavedRecipe(idReceita, btn) {
  if (!confirm(`Excluir a receita ${idReceita} e todos os seus pontos do banco? Esta ação não pode ser desfeita.`)) return;
  btn.disabled = true;
  try {
    await DatabaseAdapter.purgeAssay(idReceita);
    loadSavedRecipes();
  } catch (e) {
    alert('⚠ Falha ao excluir: ' + e.message);
    btn.disabled = false;
  }
}

// Ponto inicial (P0) = guardado com nOrdem 0 e tempo 0 (se o banco não devolver nOrdem, usa só o tempo 0)
function isP0Point(p) {
  if (p.nOrdem !== undefined && p.nOrdem !== null) return Number(p.nOrdem) === 0;
  return parseFloat(p.tempo) === 0;
}

function loadSavedIntoBuilder(idReceita, pontosValidos, receitasValidas) {
  const receita = receitasValidas.find(r => String(r.idReceita) === String(idReceita));
  const meusPontos = pontosValidos
    .filter(p => String(p.idReceita) === String(idReceita))
    .sort((a, b) => parseFloat(a.tempo ?? 0) - parseFloat(b.tempo ?? 0));

  resetLocalState();
  appState.currentAssayId = idReceita;
  appState.started = true;
  appState.source = 'db:' + idReceita;

  // Se a receita tem ponto inicial (nOrdem 0 / tempo 0), ele é o P0; receitas antigas partem de 0°C.
  let pontosCurva = meusPontos.slice();
  appState.p0Value = 0;
  if (pontosCurva.length && isP0Point(pontosCurva[0])) {
    const t0 = parseFloat(pontosCurva[0].temperatura);
    appState.p0Value = isNaN(t0) ? 0 : t0;
    appState.p0DbId = pontosCurva[0].idPonto;
    pontosCurva = pontosCurva.slice(1);
  }

  let prevCum = 0;
  appState.segments = pontosCurva.map(p => {
    const cum = parseFloat(p.tempo);
    const seg = {
      id: appState.nextId++,
      dbId: p.idPonto,
      temp: parseFloat(p.temperatura),
      duration: isNaN(cum) ? null : +(cum - prevCum).toFixed(4)
    };
    if (!isNaN(cum)) prevCum = cum;
    return seg;
  });

  showScreen('builder');
  UI.subText.textContent = `Receita "${receita ? receita.titulo : idReceita}" carregada do banco (idReceita ${idReceita}). Você pode revisar, adicionar pontos ou excluir.`;
  renderSegments();
}

function renderRecipeGrid() {
  const grid = document.getElementById('recipeGrid');
  grid.innerHTML = RECIPES.map(r => {
    const maxTemp = Math.max(...r.points.map(p => p.temp));
    const totalTime = r.points.reduce((sum, p) => sum + p.duration, 0);
    return `
      <div class="recipe-card" data-recipe="${r.id}">
        <span class="recipe-tag">${r.tag}</span>
        <span class="recipe-name">${r.name}</span>
        <p class="recipe-desc">${r.desc}</p>
        <div class="recipe-meta">
          <span>Pico: <b>${maxTemp}°C</b></span>
          <span>Duração: <b>${totalTime} min</b></span>
          <span>Pontos: <b>${r.points.length}</b></span>
        </div>
        <button class="recipe-btn" data-recipe-btn="${r.id}">Usar esta receita →</button>
      </div>`;
  }).join('');

  grid.querySelectorAll('[data-recipe-btn]').forEach(btn => {
    btn.addEventListener('click', () => selectRecipe(btn.getAttribute('data-recipe-btn')));
  });
}

// ==================== BUILDER: PONTOS DA CURVA ====================
const MAX_TEMP = 650; // limite máximo de temperatura aceito, por enquanto
const MAX_DURATION = 600; // limite de tempo (min) por ponto — 10 horas

// Impede letras, sinais e zeros à esquerda em excesso; opcionalmente limita a um valor máximo.
function sanitizeNumberInput(raw, maxVal) {
  let clean = String(raw).replace(/[^0-9.]/g, ''); // só dígitos e ponto decimal
  const firstDot = clean.indexOf('.');
  if (firstDot !== -1) {
    clean = clean.slice(0, firstDot + 1) + clean.slice(firstDot + 1).replace(/\./g, '');
  }
  clean = clean.replace(/^0+(?=\d)/, ''); // remove zeros à esquerda (ex: 000090 → 90)
  if (maxVal !== undefined && clean !== '' && clean !== '.') {
    const num = parseFloat(clean);
    if (!isNaN(num) && num > maxVal) clean = String(maxVal);
  }
  return clean;
}

function getLastKnownTemp() {
  return appState.segments.length ? appState.segments[appState.segments.length - 1].temp : (appState.p0Value ?? 0);
}

function showTempWarning(show, reason) {
  if (show === 'over') {
    UI.tempWarning.textContent = `Só é possível adicionar o ponto com uma temperatura menor ou igual a ${MAX_TEMP}°C. Corrija o valor para continuar.`;
    UI.tempWarning.style.display = 'block';
  } else if (show) {
    UI.tempWarning.textContent = `Não é possível criar uma reta de decréscimo — use um valor igual ou maior que ${getLastKnownTemp()}°C. O resfriamento do forno não é parametrizado nesta curva.`;
    UI.tempWarning.style.display = 'block';
  } else {
    UI.tempWarning.style.display = 'none';
  }
}

function getSegInfo(seg, delta) {
  return classify(delta);
}

async function addSegment() {
  if (appState.finalized) return;
  const v = parseFloat(UI.nextTempInput.value);

  if (UI.nextTempInput.value.trim() === '' || isNaN(v)) {
    flashHint('⚠ Informe um valor numérico válido para a temperatura.', true);
    UI.nextTempInput.focus();
    return;
  }

  if (v > MAX_TEMP) {
    showTempWarning('over');
    UI.nextTempInput.focus();
    return;
  }

  if (v < getLastKnownTemp()) {
    showTempWarning(true);
    UI.nextTempInput.focus();
    return;
  }
  showTempWarning(false);

  UI.addBtn.disabled = true;
  try {
    if (appState.p0Value === null) {
      // Primeiro valor digitado = temperatura inicial (P0). Gravada como Ponto nOrdem 0, tempo 0.
      const r0 = await DatabaseAdapter.createSegment(appState.currentAssayId, v, 0);
      await DatabaseAdapter.updateSegmentDuration(r0.dbId, 0);
      appState.p0Value = v;
      appState.p0DbId = r0.dbId;
      UI.nextTempInput.value = '';
      renderSegments();
      return;
    }
    const res = await DatabaseAdapter.createSegment(appState.currentAssayId, v, appState.segments.length + 1);
    appState.segments.push({ id: appState.nextId++, dbId: res.dbId, temp: v, duration: null });
    UI.nextTempInput.value = '';
    renderSegments();
  } catch (e) {
    flashHint('⚠ Falha ao registrar o ponto no banco de dados.', true);
  } finally {
    UI.addBtn.disabled = false;
    UI.nextTempInput.focus();
  }
}

async function removeSegment(localId) {
  if (appState.finalized) return;
  const seg = appState.segments.find(s => s.id === localId);
  if (seg && seg.dbId) {
    document.querySelectorAll('.seg-remove').forEach(b => b.disabled = true);
    try {
      await DatabaseAdapter.deleteSegment(seg.dbId);
      appState.segments = appState.segments.filter(s => s.id !== localId);
      recomputeAndSyncDurations();
      renderSegments();
    } catch (e) {
      alert('⚠ Falha ao excluir o ponto no banco: ' + e.message);
      document.querySelectorAll('.seg-remove').forEach(b => b.disabled = false);
    }
  }
}

async function clearPoints() {
  if (!appState.currentAssayId) return;
  const veioDoBanco = String(appState.source || '').startsWith('db:');
  if (veioDoBanco && !confirm('Essa curva já está salva no banco. Limpar os pontos vai apagar a curva original. Continuar?')) return;
  UI.restartBtn.disabled = true;
  try {
    await DatabaseAdapter.purgeAssay(appState.currentAssayId);

    const source = veioDoBanco ? 'custom' : appState.source;
    resetLocalState();
    const titulo = 'Receita personalizada - ' + new Date().toLocaleString('pt-BR');
    const assay = await DatabaseAdapter.createAssay(session.idUsuario, titulo);
    appState.currentAssayId = assay.id;
    appState.p0Value = null;
    appState.started = true;
    appState.source = source;

    UI.subText.textContent = 'Digite a temperatura inicial do forno e depois adicione os pontos da curva.';
    renderSegments();
    UI.nextTempInput.focus();
  } catch (e) {
    alert('⚠ Falha ao limpar/recriar a receita no banco: ' + e.message);
  } finally {
    UI.restartBtn.disabled = false;
  }
}

// Recalcula o tempo ACUMULADO de cada ponto a partir das durações informadas
// e envia via ponto_update.php (coluna Ponto.tempo) só os pontos cujo tempo
// acumulado é conhecido (isto é, nenhuma duração anterior está em branco).
function recomputeAndSyncDurations() {
  let cum = 0, broken = false;
  appState.segments.forEach(seg => {
    if (seg.duration === null || isNaN(seg.duration) || seg.duration <= 0) broken = true;
    if (!broken) {
      cum += seg.duration;
      seg.cumTempo = cum;
      if (seg.dbId) DatabaseAdapter.updateSegmentDuration(seg.dbId, seg.cumTempo).catch(() => {});
    } else {
      seg.cumTempo = null;
    }
  });
}

function classify(delta) {
  if (Math.abs(delta) < 1e-9) return { cls: 'flat', label: 'ESTABILIZAÇÃO', arrow: '→' };
  if (delta > 0) return { cls: 'up', label: 'AQUECIMENTO', arrow: '↑' };
  return { cls: 'down', label: 'RESFRIAMENTO', arrow: '↓' };
}

function renderSegments() {
  UI.segmentsList.innerHTML = '';
  let prevTemp = (appState.p0Value ?? 0);

  appState.segments.forEach((seg, idx) => {
    const pFrom = idx, pTo = idx + 1;
    const wrap = document.createElement('div');
    wrap.className = 'segment';

    const delta = seg.temp - prevTemp;
    const dt = (seg.duration !== null && !isNaN(seg.duration)) ? seg.duration : null;
    const info = getSegInfo(seg, delta);
    const resultHtml = dt
      ? `<div class="seg-result">
           <span class="seg-badge ${info.cls}">${info.arrow} ${info.label}</span>
           <span class="seg-rate">ΔT: <b>${delta > 0 ? '+' : ''}${delta.toFixed(1)}°C</b> · Taxa <b>${Math.abs(delta / dt).toFixed(2)} °C/min</b></span>
         </div>`
      : `<div class="seg-result">
           <span class="seg-badge ${info.cls}">${info.arrow} ${info.label}</span>
           <span class="seg-rate">ΔT: <b>${delta > 0 ? '+' : ''}${delta.toFixed(1)}°C</b> · aguardando duração</span>
         </div>`;

    wrap.innerHTML = `
      <div class="seg-label">
        <div>
          <div class="seg-title">Ponto P${pTo}</div>
          <div class="p-flow">De <b>${prevTemp}°C</b> até <b>${seg.temp}°C</b></div>
        </div>
        <button class="seg-remove" data-remove="${seg.id}" ${appState.finalized ? 'disabled' : ''}>APAGAR</button>
      </div>
      <div class="field">
        <label>Tempo até este ponto (máx. 600 min)</label>
        <div class="input-row">
          <input type="text" inputmode="decimal" data-dur="${seg.id}" value="${seg.duration !== null ? seg.duration : ''}" placeholder="ex: 90" ${appState.finalized ? 'disabled' : ''} />
          <span class="unit">min</span>
        </div>
      </div>
      ${resultHtml}
    `;
    UI.segmentsList.appendChild(wrap);
    prevTemp = seg.temp;
  });

  UI.segmentsList.querySelectorAll('[data-dur]').forEach(inp => {
    inp.addEventListener('input', e => {
      const clean = sanitizeNumberInput(e.target.value, MAX_DURATION);
      if (clean !== e.target.value) e.target.value = clean;
      const id = parseInt(e.target.getAttribute('data-dur'));
      const seg = appState.segments.find(s => s.id === id);
      const v = parseFloat(e.target.value);
      seg.duration = e.target.value === '' ? null : (isNaN(v) ? null : v);
      recomputeAndSyncDurations();
      renderSegments();
      restoreFocus(id);
    });
  });

  UI.segmentsList.querySelectorAll('[data-remove]').forEach(btn => {
    btn.addEventListener('click', e => removeSegment(parseInt(e.target.getAttribute('data-remove'))));
  });

  const semP0 = appState.p0Value === null;
  UI.p0Display.textContent = semP0 ? 'não definida' : `${appState.p0Value}°C em t = 0 min`;
  UI.addBtn.textContent = semP0 ? '+ DEFINIR TEMPERATURA INICIAL' : '+ ADICIONAR PONTO';
  UI.nextLabel.textContent = semP0
    ? 'Temperatura inicial (P0)'
    : (appState.segments.length ? `Próxima temperatura (após P${appState.segments.length})` : 'Primeira temperatura (P1)');
  UI.nextHint.textContent = semP0
    ? 'Digite a temperatura que o forno está marcando agora.'
    : (appState.segments.length ? 'Vá adicionando pontos até completar a curva.' : `A curva sai de ${appState.p0Value}°C. Digite a primeira temperatura que quer atingir.`);

  UI.stepCount.textContent = appState.currentAssayId
    ? `DB 0x${appState.currentAssayId.toString(16).toUpperCase()} · ${appState.segments.length} ponto(s)`
    : 'Sessão em preparação';

  UI.finishBtn.disabled = appState.segments.length === 0;
  updateHint();

  UI.resultsArea.style.display = 'flex';
  buildSummaryTable();
  drawResponsiveChart();
}

function restoreFocus(id) {
  const el = UI.segmentsList.querySelector(`[data-dur="${id}"]`);
  if (el) { el.focus(); const val = el.value; el.setSelectionRange(val.length, val.length); }
}

function flashHint(msg, isError) {
  UI.hintText.textContent = msg;
  if (isError) {
    UI.hintText.style.color = 'var(--danger)';
    setTimeout(() => { UI.hintText.style.color = 'var(--dim)'; updateHint(); }, 2600);
  }
}

function updateHint() {
  if (appState.finalized) return;
  if (appState.segments.length === 0) {
    UI.hintText.textContent = appState.p0Value === null
      ? 'Digite a temperatura inicial do forno para começar.'
      : 'Digite a primeira temperatura da curva.';
  } else {
    const incomplete = appState.segments.some(s => s.duration === null);
    UI.hintText.textContent = incomplete
      ? '⚠ Falta preencher o tempo de algum ponto.'
      : 'Curva ok. Clique em Finalizar curva para confirmar.';
  }
}

// ==================== FINALIZAÇÃO ====================
UI.finishBtn.addEventListener('click', () => {
  const valid = appState.segments.length > 0 && appState.segments.every(s => s.duration !== null && s.duration > 0);
  if (!valid) {
    flashHint('⚠ Falta preencher o tempo de algum ponto.', true);
    return;
  }
  appState.finalized = true;
  UI.addBtn.style.display = 'none';
  UI.finishBtn.style.display = 'none';
  UI.nextTempInput.disabled = true;
  UI.summaryBanner.style.display = 'block';
  UI.finalCount.textContent = appState.segments.length;

  UI.mainBtn.disabled = false;
  UI.mainBtn.textContent = 'Exportar Receita ao CLP →';
  UI.hintText.textContent = 'Curva confirmada. O resfriamento no final não entra na conta, porque depende do forno.';

  setChartBadge(true);
  renderSegments();
});

UI.editBtn.addEventListener('click', () => {
  appState.finalized = false;
  UI.addBtn.style.display = 'flex';
  UI.finishBtn.style.display = 'flex';
  UI.nextTempInput.disabled = false;
  UI.summaryBanner.style.display = 'none';
  UI.mainBtn.disabled = true;
  UI.mainBtn.textContent = 'Curva incompleta';
  setChartBadge(false);
  renderSegments();
});

function setChartBadge(isFinal) {
  UI.chartBadge.textContent = isFinal ? 'CONFIRMADA' : 'PRÉVIA';
  UI.chartBadge.className = 'chart-badge ' + (isFinal ? 'final' : 'preview');
  UI.coolNote.style.display = isFinal ? 'block' : 'none';
}

function buildSummaryTable() {
  UI.summaryTbody.innerHTML = '';
  let prevTemp = (appState.p0Value ?? 0);
  let totTime = 0;
  let hasPending = false;

  appState.segments.forEach((seg, idx) => {
    const pFrom = idx, pTo = idx + 1;
    const delta = seg.temp - prevTemp;
    const info = getSegInfo(seg, delta);
    const tr = document.createElement('tr');

    if (seg.duration !== null && seg.duration > 0) {
      const rate = delta / seg.duration;
      totTime += seg.duration;
      tr.innerHTML = `
        <td>P${pFrom} → P${pTo}</td>
        <td>${prevTemp}°C → ${seg.temp}°C</td>
        <td style="color:${delta > 0 ? 'var(--amber)' : (delta < 0 ? 'var(--down)' : 'var(--text)')}">${delta > 0 ? '+' : ''}${delta.toFixed(1)}°C</td>
        <td>${seg.duration} min</td>
        <td>${Math.abs(rate).toFixed(2)}</td>
        <td><span class="seg-badge ${info.cls}">${info.arrow} ${info.label}</span></td>
      `;
    } else {
      hasPending = true;
      tr.innerHTML = `
        <td>P${pFrom} → P${pTo}</td>
        <td>${prevTemp}°C → ${seg.temp}°C</td>
        <td style="color:${delta > 0 ? 'var(--amber)' : (delta < 0 ? 'var(--down)' : 'var(--text)')}">${delta > 0 ? '+' : ''}${delta.toFixed(1)}°C</td>
        <td class="pending">—</td>
        <td class="pending">—</td>
        <td><span class="seg-badge ${info.cls}">${info.arrow} ${info.label}</span></td>
      `;
    }
    UI.summaryTbody.appendChild(tr);
    prevTemp = seg.temp;
  });

  UI.totalTime.textContent = totTime + ' min' + (hasPending ? ' (parcial)' : '');
  UI.totalSteps.textContent = appState.segments.length;
}

// ==================== GRÁFICO (CANVAS) ====================
function drawResponsiveChart() {
  const canvas = document.getElementById('chart');
  const ctx = canvas.getContext('2d');
  const container = canvas.parentElement;

  const W = canvas.width = container.clientWidth;
  const H = canvas.height = container.clientHeight;
  ctx.clearRect(0, 0, W, H);
  if (W === 0 || H === 0) return;

  const padL = 50, padR = 20, padT = 20, padB = 40;
  const plotW = W - padL - padR, plotH = H - padT - padB;

  // Monta os pontos conhecidos: para na primeira duração ainda não definida
  let points = [{ t: 0, temp: (appState.p0Value ?? 0) }];
  let cumT = 0;
  for (const seg of appState.segments) {
    if (seg.duration === null || isNaN(seg.duration) || seg.duration <= 0) break;
    cumT += seg.duration;
    points.push({ t: cumT, temp: seg.temp });
  }

  if (points.length < 2) {
    ctx.strokeStyle = '#2b333a';
    ctx.beginPath();
    ctx.moveTo(padL, padT + plotH); ctx.lineTo(W - padR, padT + plotH);
    ctx.moveTo(padL, padT); ctx.lineTo(padL, padT + plotH);
    ctx.stroke();
    ctx.fillStyle = '#7f8b93';
    ctx.font = '12px JetBrains Mono, monospace';
    ctx.textAlign = 'center';
    ctx.fillText('Aguardando o primeiro ponto com duração definida...', W / 2, H / 2);
    ctx.textAlign = 'left';
    return;
  }

  const maxT = points[points.length - 1].t || 1;
  const temps = points.map(p => p.temp);
  let minTemp = Math.min(0, ...temps);
  let maxTemp = Math.max(...temps);
  if (maxTemp === minTemp) { maxTemp += 1; minTemp -= 1; }
  const rangePad = (maxTemp - minTemp) * 0.12;
  minTemp -= rangePad; maxTemp += rangePad;

  function X(t) { return padL + (t / maxT) * plotW; }
  function Y(temp) { return padT + plotH - ((temp - minTemp) / (maxTemp - minTemp)) * plotH; }

  // Grade horizontal + rótulos de temperatura
  ctx.strokeStyle = '#2b333a';
  ctx.lineWidth = 1;
  ctx.font = '11.5px JetBrains Mono, monospace';
  ctx.fillStyle = '#7f8b93';
  const gridLines = 5;
  for (let i = 0; i <= gridLines; i++) {
    const temp = minTemp + (maxTemp - minTemp) * (i / gridLines);
    const y = Y(temp);
    ctx.beginPath();
    ctx.moveTo(padL, y);
    ctx.lineTo(W - padR, y);
    ctx.stroke();
    ctx.fillText(Math.round(temp) + '°', 4, y + 4);
  }

  // Rótulos de tempo (eixo X)
  points.forEach(p => {
    const x = X(p.t);
    ctx.fillStyle = '#7f8b93';
    ctx.fillText(p.t + 'm', Math.max(padL, Math.min(x - 10, W - padR - 24)), H - padB + 16);
  });

  // Legendas dos eixos
  ctx.fillStyle = '#7f8b93';
  ctx.font = '10.5px JetBrains Mono, monospace';
  ctx.fillText('TEMPO (MIN)', W - padR - 62, H - 6);
  ctx.save();
  ctx.translate(12, padT + 10);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText('TEMP (°C)', 0, 0);
  ctx.restore();

  // Eixos
  ctx.strokeStyle = '#3a444c';
  ctx.beginPath();
  ctx.moveTo(padL, padT); ctx.lineTo(padL, padT + plotH); ctx.lineTo(W - padR, padT + plotH);
  ctx.stroke();

  // Linha da curva (interpolação linear)
  ctx.lineWidth = 2.5;
  ctx.strokeStyle = '#e58a2a';
  for (let i = 1; i < points.length; i++) {
    const p0 = points[i - 1], p1 = points[i];
    ctx.beginPath();
    ctx.moveTo(X(p0.t), Y(p0.temp));
    ctx.lineTo(X(p1.t), Y(p1.temp));
    ctx.stroke();
  }

  // Área sob a curva
  ctx.beginPath();
  points.forEach((p, i) => {
    const x = X(p.t), y = Y(p.temp);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.lineTo(X(points[points.length - 1].t), Y(minTemp));
  ctx.lineTo(X(points[0].t), Y(minTemp));
  ctx.closePath();
  ctx.fillStyle = 'rgba(229,138,42,0.08)';
  ctx.fill();

  // Taxa de crescimento (°C/min) no meio de cada segmento
  ctx.font = 'bold 11px JetBrains Mono, monospace';
  ctx.textAlign = 'center';
  for (let i = 1; i < points.length; i++) {
    const p0 = points[i - 1], p1 = points[i];
    const dt = p1.t - p0.t;
    const mx = X((p0.t + p1.t) / 2);
    const my = Y((p0.temp + p1.temp) / 2);
    const offsetY = p1.temp >= p0.temp ? -10 : 16;
    const rate = dt > 0 ? (p1.temp - p0.temp) / dt : 0;
    ctx.fillStyle = '#e58a2a';
    ctx.fillText(`${rate >= 0 ? '+' : ''}${rate.toFixed(2)}°C/m`, mx, my + offsetY);
  }
  ctx.textAlign = 'left';

  // Nós (pontos)
  points.forEach((p, i) => {
    const x = X(p.t), y = Y(p.temp);
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, Math.PI * 2);
    ctx.fillStyle = '#14181c';
    ctx.fill();
    ctx.lineWidth = 2;
    ctx.strokeStyle = '#e58a2a';
    ctx.stroke();
    ctx.fillStyle = '#e7ecef';
    ctx.font = 'bold 12px JetBrains Mono, monospace';
    ctx.fillText('P' + i, x - 8, y - 14);
  });

  // Ponto final "voando": quando a curva está confirmada, o último ponto fica em aberto —
  // não existe reta até o zero, pois o tempo de resfriamento não é estimável.
  if (appState.finalized && points.length) {
    const last = points[points.length - 1];
    const lx = X(last.t), ly = Y(last.temp);

    // anel tracejado ao redor do ponto, indicando que ele está "solto" (sem continuação traçada)
    ctx.beginPath();
    ctx.setLineDash([3, 4]);
    ctx.strokeStyle = '#4a9bd6';
    ctx.lineWidth = 1.5;
    ctx.arc(lx, ly, 10, 0, Math.PI * 2);
    ctx.stroke();
    ctx.setLineDash([]);

    // pequeno rastro tracejado e esmaecido, sugerindo direção do resfriamento sem indicar duração
    const stubX = Math.min(lx + 42, W - padR - 6);
    const stubY = Math.min(ly + 30, padT + plotH - 4);
    ctx.beginPath();
    ctx.setLineDash([4, 5]);
    ctx.strokeStyle = 'rgba(74,155,214,0.55)';
    ctx.lineWidth = 2;
    ctx.moveTo(lx + 8, ly + 6);
    ctx.quadraticCurveTo(lx + 16, ly + 18, stubX, stubY);
    ctx.stroke();
    ctx.setLineDash([]);

    ctx.beginPath();
    ctx.arc(stubX, stubY, 2.5, 0, Math.PI * 2);
    ctx.fillStyle = '#4a9bd6';
    ctx.fill();

    // rótulo explicativo
    const labelRight = (stubX + 150 > W - padR);
    ctx.textAlign = labelRight ? 'right' : 'left';
    const labelX = labelRight ? stubX - 8 : stubX + 8;
    ctx.fillStyle = '#4a9bd6';
    ctx.font = 'bold 11px JetBrains Mono, monospace';
    ctx.fillText('RESFRIAMENTO NATURAL', labelX, stubY - 3);
    ctx.font = '10px JetBrains Mono, monospace';
    ctx.fillStyle = 'rgba(74,155,214,0.85)';
    ctx.fillText('tempo não estimado', labelX, stubY + 11);
    ctx.textAlign = 'left';
  }
}

// ==================== EXPORTAÇÃO — DADOS DA CURVA (BUILDER) EM PDF ====================
function setExportStatus(el, msg, kind) {
  el.textContent = msg || '';
  el.className = 'export-status' + (kind ? ' ' + kind : '');
}

async function exportBuilderDataToPDF() {
  if (!appState.segments.length) {
    setExportStatus(UI.exportPdfStatus, '⚠ Nenhum ponto definido para exportar.', 'err');
    return;
  }
  const { jsPDF } = window.jspdf || {};
  if (!jsPDF) {
    setExportStatus(UI.exportPdfStatus, '⚠ Biblioteca de PDF não carregou (verifique a conexão com a internet).', 'err');
    return;
  }

  UI.exportPdfBtn.disabled = true;
  setExportStatus(UI.exportPdfStatus, 'Gerando PDF...', '');

  try {
    const doc = new jsPDF({ orientation: 'landscape', unit: 'pt', format: 'a4' });
    const titulo = appState.currentAssayId
      ? `Receita — DB 0x${appState.currentAssayId.toString(16).toUpperCase()}`
      : 'Curva térmica — sessão em preparação';

    doc.setFontSize(16);
    doc.text('Parametrização Térmica do Forno — Curva Definida', 40, 40);
    doc.setFontSize(10);
    doc.setTextColor(90);
    const geradoEm = new Date().toLocaleString('pt-BR');
    doc.text(`${titulo}    ·    Operador: ${session.nomeUsuario || '—'}    ·    Gerado em: ${geradoEm}`, 40, 58);

    // Captura o gráfico (canvas #chart) exatamente como está desenhado na tela
    let chartBottomY = 68;
    const chartCanvas = document.getElementById('chart');
    if (chartCanvas && chartCanvas.width > 0 && chartCanvas.height > 0) {
      // Redesenha sobre um fundo sólido para não sair com fundo transparente no PDF
      const flat = document.createElement('canvas');
      flat.width = chartCanvas.width;
      flat.height = chartCanvas.height;
      const fctx = flat.getContext('2d');
      fctx.fillStyle = '#1c2228';
      fctx.fillRect(0, 0, flat.width, flat.height);
      fctx.drawImage(chartCanvas, 0, 0);

      const imgData = flat.toDataURL('image/png');
      const pageWidth = doc.internal.pageSize.getWidth();
      const imgW = pageWidth - 80;
      const imgH = imgW * (flat.height / flat.width);
      doc.setDrawColor(51, 62, 70);
      doc.setFillColor(28, 34, 40);
      doc.rect(40, 74, imgW, imgH, 'FD');
      doc.addImage(imgData, 'PNG', 40, 74, imgW, imgH);
      chartBottomY = 74 + imgH + 16;
    }

    // Monta as linhas com a mesma lógica usada na tabela em tela (summary-table)
    let prevTemp = (appState.p0Value ?? 0);
    let totTime = 0;
    const rows = appState.segments.map((seg, idx) => {
      const pFrom = idx, pTo = idx + 1;
      const delta = seg.temp - prevTemp;
      const info = getSegInfo(seg, delta);
      let row;
      if (seg.duration !== null && seg.duration > 0) {
        const rate = delta / seg.duration;
        totTime += seg.duration;
        row = [
          `P${pFrom} → P${pTo}`,
          `${prevTemp}°C → ${seg.temp}°C`,
          `${delta > 0 ? '+' : ''}${delta.toFixed(1)}°C`,
          `${seg.duration} min`,
          Math.abs(rate).toFixed(2),
          info.label
        ];
      } else {
        row = [`P${pFrom} → P${pTo}`, `${prevTemp}°C → ${seg.temp}°C`, `${delta > 0 ? '+' : ''}${delta.toFixed(1)}°C`, '—', '—', info.label];
      }
      prevTemp = seg.temp;
      return row;
    });

    doc.autoTable({
      startY: chartBottomY,
      head: [['Ponto', 'Transição', 'ΔT', 'Duração', 'Taxa (°C/min)', 'Tendência']],
      body: rows,
      theme: 'grid',
      headStyles: { fillColor: [35, 43, 50], textColor: 255, fontSize: 9 },
      styles: { fontSize: 9, cellPadding: 5 },
      margin: { left: 40, right: 40 }
    });

    const afterTableY = doc.lastAutoTable.finalY + 20;
    doc.setFontSize(10);
    doc.setTextColor(30);
    doc.text(`Tempo total: ${totTime} min    ·    Pontos definidos: ${appState.segments.length}`, 40, afterTableY);

    const fileTag = appState.currentAssayId ? `receita_${appState.currentAssayId}` : `curva_${Date.now()}`;
    doc.save(`dados_curva_termica_${fileTag}.pdf`);
    setExportStatus(UI.exportPdfStatus, '✓ PDF exportado com sucesso.', 'ok');
  } catch (e) {
    setExportStatus(UI.exportPdfStatus, '⚠ Falha ao gerar PDF: ' + e.message, 'err');
  } finally {
    UI.exportPdfBtn.disabled = false;
  }
}

// ==================== GRÁFICO EM TEMPO REAL (ESP32) ====================
/**
 * A página é servida pelo próprio ESP32, então as chamadas usam caminhos relativos
 * (mesma origem, sem IP e sem CORS):
 *   GET  /temperatura   ->   {"temperatura": 123.4}
 *   POST /cor  (cor=vermelho|verde|azul|amarelo|ciano|magenta|branco|desligado)
 *        -> troca a cor do LED RGB nativo do ESP32-S3
 */
const ESP_CONFIG = { PATH: '/temperatura', INTERVAL_MS: 1000, MAX_POINTS: 600 };
const live = { timer: null, running: false, data: [], t0: null, busy: false };
const LUI = {
  corSelect: document.getElementById('corSelect'),
  corBtn: document.getElementById('corSetBtn'),
  corStatus: document.getElementById('corStatus'),
  clear: document.getElementById('liveClearBtn'),
  status: document.getElementById('liveStatus'),
  now: document.getElementById('liveNow'),
  max: document.getElementById('liveMax'),
  count: document.getElementById('liveCount'),
  canvas: document.getElementById('liveChart')
};

function setLiveStatus(msg, kind) {
  LUI.status.textContent = msg;
  LUI.status.className = 'live-status' + (kind ? ' ' + kind : '');
}

// ---------- Comando de cor do LED (POST /cor, igual ao /conectar da página de Wi-Fi) ----------
async function setCor() {
  const cor = LUI.corSelect.value;
  LUI.corBtn.disabled = true;
  LUI.corStatus.textContent = 'Enviando...';
  try {
    const r = await fetch('/cor', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'cor=' + encodeURIComponent(cor)
    });
    if (!r.ok) throw new Error((await r.text()) || ('HTTP ' + r.status));
    LUI.corStatus.textContent = 'LED: ' + LUI.corSelect.options[LUI.corSelect.selectedIndex].text;
  } catch (e) {
    LUI.corStatus.textContent = 'Falha ao definir a cor (' + e.message + ').';
  } finally {
    LUI.corBtn.disabled = false;
  }
}

async function readEsp() {
  if (live.busy) return;
  live.busy = true;
  try {
    const ctrl = new AbortController();
    const to = setTimeout(() => ctrl.abort(), 4000);
    const res = await fetch(ESP_CONFIG.PATH, { signal: ctrl.signal, cache: 'no-store' });
    clearTimeout(to);
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const j = await res.json();
    const v = parseFloat(j.temperatura);
    if (isNaN(v)) throw new Error('JSON sem o campo "temperatura"');
    if (live.t0 === null) live.t0 = Date.now();
    live.data.push({ t: (Date.now() - live.t0) / 1000, v });
    if (live.data.length > ESP_CONFIG.MAX_POINTS) live.data.shift();
    setLiveStatus('Conectado ao ESP32 — recebendo leituras.', 'ok');
    updateLiveInfo();
    drawLiveChart();
  } catch (e) {
    setLiveStatus('Sem resposta do ESP32 (' + (e.name === 'AbortError' ? 'tempo esgotado' : e.message) + '). Tentando de novo...', 'err');
  } finally {
    live.busy = false;
  }
}

function updateLiveInfo() {
  const d = live.data;
  LUI.count.textContent = d.length;
  if (!d.length) { LUI.now.textContent = '—'; LUI.max.textContent = '—'; return; }
  LUI.now.textContent = d[d.length - 1].v.toFixed(1) + ' °C';
  LUI.max.textContent = Math.max(...d.map(p => p.v)).toFixed(1) + ' °C';
}

function startLive() {
  if (live.running) return;
  live.running = true;
  setLiveStatus('Conectando...', '');
  readEsp();
  live.timer = setInterval(readEsp, ESP_CONFIG.INTERVAL_MS);
}

function stopLive() {
  if (live.timer) clearInterval(live.timer);
  live.timer = null;
  if (live.running) setLiveStatus('Desconectado.', '');
  live.running = false;
}

function clearLive() {
  live.data = [];
  live.t0 = null;
  updateLiveInfo();
  drawLiveChart();
}

function drawLiveChart() {
  const c = LUI.canvas;
  const box = c.parentElement;
  const dpr = window.devicePixelRatio || 1;
  const W = box.clientWidth, H = box.clientHeight;
  if (!W || !H) return;
  c.width = W * dpr; c.height = H * dpr;
  const ctx = c.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, W, H);

  const padL = 46, padR = 14, padT = 12, padB = 30;
  const pw = W - padL - padR, ph = H - padT - padB;
  const d = live.data;

  let minV = 0, maxV = 100, maxT = 60;
  if (d.length) {
    maxV = Math.max(100, Math.ceil((Math.max(...d.map(p => p.v)) + 10) / 50) * 50);
    maxT = Math.max(60, d[d.length - 1].t);
  }
  const X = t => padL + (t / maxT) * pw;
  const Y = v => padT + ph - ((v - minV) / (maxV - minV)) * ph;

  ctx.font = '12px Arial, Helvetica, sans-serif';
  ctx.strokeStyle = '#2b333a'; ctx.fillStyle = '#7f8b93';
  ctx.lineWidth = 1;
  ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
  for (let i = 0; i <= 5; i++) {
    const v = minV + (maxV - minV) * i / 5, y = Y(v);
    ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(W - padR, y); ctx.stroke();
    ctx.fillText(Math.round(v) + '°', padL - 6, y);
  }
  ctx.textAlign = 'center'; ctx.textBaseline = 'top';
  for (let i = 0; i <= 5; i++) {
    const t = maxT * i / 5;
    ctx.fillText(t < 120 ? Math.round(t) + 's' : (t / 60).toFixed(1) + 'min', X(t), H - padB + 8);
  }

  if (!d.length) {
    ctx.fillStyle = '#7f8b93'; ctx.textBaseline = 'middle';
    ctx.fillText('Aguardando leituras do ESP32...', W / 2, H / 2);
    return;
  }
  ctx.strokeStyle = '#4a90d9'; ctx.lineWidth = 2; ctx.lineJoin = 'round';
  ctx.beginPath();
  d.forEach((p, i) => { i ? ctx.lineTo(X(p.t), Y(p.v)) : ctx.moveTo(X(p.t), Y(p.v)); });
  ctx.stroke();
}

function openLive() { showScreen('live'); drawLiveChart(); startLive(); }

document.getElementById('goLiveBtn').addEventListener('click', openLive);
LUI.clear.addEventListener('click', clearLive);
LUI.corBtn.addEventListener('click', setCor);
window.addEventListener('resize', () => { if (currentScreen === 'live') drawLiveChart(); });

// ==================== EVENTOS GLOBAIS ====================
document.getElementById('goCustomBtn').addEventListener('click', enterCustom);
document.getElementById('goSavedBtn').addEventListener('click', () => showScreen('saved'));
UI.backBtn.addEventListener('click', () => {
  if (currentScreen === 'builder' || currentScreen === 'recipes' || currentScreen === 'saved') goHome();
  else if (currentScreen === 'live') showScreen('home');
});
UI.loginBtn.addEventListener('click', doLogin);
UI.newUserBtn.addEventListener('click', doCreateUser);
UI.operatorTag.addEventListener('click', logout);
UI.addBtn.addEventListener('click', addSegment);
UI.nextTempInput.addEventListener('keydown', e => { if (e.key === 'Enter') { e.preventDefault(); addSegment(); } });
UI.nextTempInput.addEventListener('input', () => {
  const caretAtEnd = UI.nextTempInput.selectionStart === UI.nextTempInput.value.length;
  const clean = sanitizeNumberInput(UI.nextTempInput.value);
  if (clean !== UI.nextTempInput.value) {
    UI.nextTempInput.value = clean;
    if (caretAtEnd) UI.nextTempInput.setSelectionRange(clean.length, clean.length);
  }
  const v = parseFloat(UI.nextTempInput.value);
  if (!isNaN(v) && v > MAX_TEMP) {
    showTempWarning('over');
  } else {
    showTempWarning(!isNaN(v) && v < getLastKnownTemp());
  }
});
UI.restartBtn.addEventListener('click', clearPoints);
UI.exportPdfBtn.addEventListener('click', exportBuilderDataToPDF);

window.addEventListener('resize', () => {
  if (currentScreen === 'builder' && appState.started) drawResponsiveChart();
});

// ==================== INICIALIZAÇÃO ====================
// A tela agora abre pedindo identificação do operador (leitura da tabela Usuario)
// antes de liberar a Home. showScreen('login') já está marcada como .active no HTML.
loadUsuarios();
)rawliteral";
