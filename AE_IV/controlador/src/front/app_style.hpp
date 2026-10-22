#pragma once
#include <pgmspace.h>

// ----- Folha de estilo da página principal do forno -----
const char app_css[] PROGMEM = R"rawliteral(
:root {
  --bg: #14181c;
  --panel: #1c2228;
  --panel-2: #232b32;
  --line: #333e46;
  --text: #e7ecef;
  --dim: #7f8b93;
  --amber: #4a90d9;
  --amber-dim: #3a5f8a;
  --ok: #4caf7d;
  --ok-dim: #234832;
  --down: #4a9bd6;
  --down-dim: #1f3a4d;
  --flat: #7f8b93;
  --danger: #d65c5c;
}

* { box-sizing: border-box; }

/* ==================== BASE (MOBILE-FIRST) ==================== */
body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font-family: Arial, Helvetica, sans-serif;
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}

.device {
  width: 100%;
  background: var(--panel);
  display: flex;
  flex-direction: column;
  flex: 1;
  min-height: 0;
}

.topbar {
  display: flex; align-items: center; gap: 10px; justify-content: space-between;
  padding: 14px 18px; border-bottom: 1px solid var(--line);
  background: var(--panel-2);
}
.topbar-left { display: flex; align-items: center; gap: 10px; min-width: 0; }
.brand {
  font-family: Arial, Helvetica, sans-serif; font-size: 12.5px;
   color: var(--dim); 
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
}
.wifi {
  display: flex; align-items: center; gap: 6px; flex-shrink: 0;
  font-family: Arial, Helvetica, sans-serif; font-size: 11.5px; color: var(--ok);
}
.dot { width: 8px; height: 8px; border-radius: 50%; background: var(--ok); }
.dot.syncing { background: var(--amber); animation: pulse 1s infinite alternate; }
@keyframes pulse { from { opacity: 0.5; } to { opacity: 1; } }

.back-btn {
  font-family: Arial, Helvetica, sans-serif; font-size: 12.5px; color: var(--dim); background: transparent;
  border: 1px solid var(--line); border-radius: 6px; padding: 6px 10px; cursor: pointer; flex-shrink: 0;
}
.back-btn:hover { color: var(--amber); border-color: var(--amber); }

/* ==================== ROTEAMENTO DE TELAS ==================== */
.screen { display: none; flex: 1; flex-direction: column; min-height: 0; overflow-y: auto; }
.screen.active { display: flex; }

/* ---- Tela inicial ---- */
.home-screen, .recipes-screen { padding: 26px 20px; }
.home-inner, .recipes-inner { max-width: 720px; margin: 0 auto; width: 100%; display: flex; flex-direction: column; gap: 16px; }

.eyebrow {
  font-family: Arial, Helvetica, sans-serif; font-size: 13.5px; color: var(--amber);
    margin: 0;
}
h1 { margin: 4px 0 0; font-size: 29px; font-weight: 600;  }
.sub { margin: 6px 0 0; color: var(--dim); font-size: 17.5px; line-height: 1.45; }

.option-grid { display: flex; flex-direction: column; gap: 16px; margin-top: 10px; }
.option-card {
  display: flex; flex-direction: column; align-items: flex-start; gap: 8px;
  background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px;
  padding: 24px; text-align: left; cursor: pointer; color: var(--text); width: 100%;
}
.option-card:hover { border-color: var(--amber); background: transparent; }
.option-icon { display: none; }
.option-title { font-size: 22px; font-weight: 600; }
.option-desc { color: var(--dim); font-size: 17px; line-height: 1.5; }
.option-cta { margin-top: 10px; font-family: Arial, Helvetica, sans-serif; font-size: 14px; color: var(--amber);  }

/* ---- Tela de receitas prontas ---- */
.recipe-grid { display: flex; flex-direction: column; gap: 16px; margin-top: 6px; }
.recipe-card { background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; padding: 22px; display: flex; flex-direction: column; gap: 9px; }
.recipe-tag { font-family: Arial, Helvetica, sans-serif; font-size: 13px; color: var(--amber);   }
.recipe-name { font-size: 23px; font-weight: 600; }
.recipe-desc { color: var(--dim); font-size: 17.5px; line-height: 1.5; }
.recipe-meta { display: flex; flex-wrap: wrap; gap: 5px 16px; font-family: Arial, Helvetica, sans-serif; font-size: 15px; color: var(--dim); margin-top: 3px; }
.recipe-meta b { color: var(--text); }
.recipe-btn {
  margin-top: 10px; align-self: flex-start; border: 1px solid var(--amber-dim); border-radius: 4px;
  padding: 10px 16px; color: var(--amber); background: transparent; font-family: Arial, Helvetica, sans-serif;
  font-size: 15px;  cursor: pointer;
}
.recipe-btn:hover:not(:disabled) { background: var(--panel); }
.recipe-btn:disabled { opacity: .5; cursor: wait; }

/* ---- Tela de login / operador ---- */
.login-screen { display: flex; align-items: center; justify-content: center; padding: 26px 20px; flex: 1; }
.login-card { max-width: 420px; width: 100%; background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; padding: 26px; display: flex; flex-direction: column; gap: 14px; }
.login-select {
  width: 100%; background: var(--bg); border: 1px solid var(--line); border-radius: 4px; padding: 11px 12px;
  color: var(--text); font-family: Arial, Helvetica, sans-serif; font-size: 15px;
}
.login-status { font-family: Arial, Helvetica, sans-serif; font-size: 13px; color: var(--dim); min-height: 18px; }
.login-status.err { color: var(--danger); }
.login-divider { display: flex; align-items: center; gap: 10px; color: var(--dim); font-family: Arial, Helvetica, sans-serif; font-size: 11.5px;   margin: 2px 0; }
.login-divider::before, .login-divider::after { content: ''; flex: 1; height: 1px; background: var(--line); }
.login-new { display: flex; flex-direction: column; gap: 8px; }
.login-new input {
  width: 100%; background: var(--bg); border: 1px solid var(--line); border-radius: 4px; padding: 10px 12px;
  color: var(--text); font-family: Arial, Helvetica, sans-serif; font-size: 14px; outline: none;
}
.login-new input:focus { border-color: var(--amber); }
.operator-tag {
  font-family: Arial, Helvetica, sans-serif; font-size: 11.5px; color: var(--dim); background: transparent;
  border: 1px solid var(--line); border-radius: 6px; padding: 5px 9px; cursor: pointer; white-space: nowrap;
}
.operator-tag:hover { color: var(--amber); border-color: var(--amber); }

/* ---- Tela de receitas salvas no banco (leitura) ---- */
.saved-card { background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; padding: 22px; display: flex; flex-direction: column; gap: 9px; }
.saved-empty { font-family: Arial, Helvetica, sans-serif; font-size: 14px; color: var(--dim); padding: 20px; text-align: center; }
.saved-actions { display: flex; gap: 10px; margin-top: 10px; flex-wrap: wrap; }
.saved-load-btn { border: 1px solid var(--amber-dim); border-radius: 4px; padding: 10px 16px; color: var(--amber); background: transparent; font-family: Arial, Helvetica, sans-serif; font-size: 15px;  cursor: pointer; }
.saved-load-btn:hover:not(:disabled) { background: var(--panel); }
.saved-del-btn { border: 1px solid #5a2c2c; border-radius: 4px; padding: 10px 16px; color: var(--danger); background: transparent; font-family: Arial, Helvetica, sans-serif; font-size: 15px;  cursor: pointer; }
.saved-del-btn:hover:not(:disabled) { background: #2a1616; }
.saved-load-btn:disabled, .saved-del-btn:disabled { opacity: .5; cursor: wait; }

/* ---- Botões de exportação (PDF) ---- */
.export-row { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 4px; }
.export-btn {
  display: inline-flex; align-items: center; gap: 7px;
  border-radius: 4px; padding: 10px 16px; font-family: Arial, Helvetica, sans-serif;
  font-size: 14px;  cursor: pointer; border: 1px solid var(--line);
  background: var(--panel-2); color: var(--text); transition: background .2s, border-color .2s;
}
.export-btn:hover:not(:disabled) { border-color: var(--amber); background: var(--panel); }
.export-btn:disabled { opacity: .5; cursor: wait; }
.export-btn.pdf { border-color: #5a2c2c33; }
.export-btn.pdf:hover:not(:disabled) { border-color: var(--danger); background: rgba(214,92,92,.08); }
.export-status { font-family: Arial, Helvetica, sans-serif; font-size: 12.5px; color: var(--dim); min-height: 16px; width: 100%; }
.export-status.err { color: var(--danger); }
.export-status.ok { color: var(--ok); }


/* ---- Tela de acompanhamento em tempo real (ESP32 + termopar) ---- */
.live-row { display: flex; gap: 10px; flex-wrap: wrap; align-items: center; }
.live-row input { flex: 1; min-width: 180px; background: var(--bg); border: 1px solid var(--line); border-radius: 4px; padding: 10px 12px; color: var(--text); font-family: Arial, Helvetica, sans-serif; font-size: 15px; outline: none; }
.live-row input:focus { border-color: var(--amber); }
.live-status { font-size: 14px; color: var(--dim); min-height: 18px; }
.live-status.err { color: var(--danger); }
.live-status.ok { color: var(--ok); }
.live-value { background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; padding: 14px 16px; display: flex; gap: 24px; flex-wrap: wrap; font-size: 15px; color: var(--dim); }
.live-value b { display: block; font-size: 30px; font-weight: 600; color: var(--text); }
.live-chart { height: 340px; }

@media (min-width: 640px) {
  .option-grid { flex-direction: row; }
  .option-card { flex: 1; }
  .recipe-grid, .saved-grid { display: grid; grid-template-columns: 1fr 1fr; align-items: start; }
}

/* ---- Tela do builder (parametrização ponto a ponto) ---- */
.layout-grid { display: flex; flex-direction: column; flex: 1; min-height: 0; }
.sidebar-controls { display: flex; flex-direction: column; flex: none; min-height: 0; }
.main-results { display: flex; flex-direction: column; padding: 18px 20px 22px; gap: 16px; background: var(--bg); flex: none; }

.header { padding: 20px 20px 8px; }

.form { padding: 8px 20px 4px; display: flex; flex-direction: column; gap: 14px; }
#segmentsScroll { overflow-y: auto; display: flex; flex-direction: column; gap: 14px; padding-right: 2px; max-height: 340px; }
#segmentsScroll::-webkit-scrollbar { width: 6px; }
#segmentsScroll::-webkit-scrollbar-thumb { background: var(--line); border-radius: 3px; }
#segmentsList { display: flex; flex-direction: column; gap: 14px; }

#btnRowTop { position: sticky; top: 0; z-index: 2; background: var(--panel); padding-top: 2px; padding-bottom: 10px; margin-bottom: -4px; }

.field label {
  display: block; font-family: Arial, Helvetica, sans-serif; font-size: 13px;
  color: var(--dim);   margin-bottom: 6px;
}
.field .field-hint { display: block; font-family: Arial, Helvetica, sans-serif; font-size: 15px; color: var(--dim); margin-top: 6px; text-transform: none;  }
.field .field-warning { display: none; font-family: Arial, Helvetica, sans-serif; font-size: 13.5px; color: var(--danger); margin-top: 6px; line-height: 1.4; }
.input-row {
  display: flex; align-items: center; background: var(--panel-2);
  border: 1px solid var(--line); border-radius: 4px; padding: 10px 12px; transition: border-color 0.2s;
}
.input-row:focus-within { border-color: var(--amber); }
.input-row input {
  flex: 1; width: 100%; background: transparent; border: none; outline: none; color: var(--text);
  font-family: Arial, Helvetica, sans-serif; font-size: 20px; font-weight: 500;
}
.input-row input:disabled { color: var(--dim); }
.unit { color: var(--dim); font-family: Arial, Helvetica, sans-serif; font-size: 14px; white-space: nowrap; margin-left: 6px; }

.segment { background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; padding: 13px; position: relative; }
.segment .seg-label { display: flex; justify-content: space-between; align-items: center; gap: 10px; margin-bottom: 10px; }
.seg-title { font-size: 16.5px; font-weight: 600; }
.seg-remove {
  font-family: Arial, Helvetica, sans-serif; font-size: 11.5px; color: var(--danger);
  border: 1px solid #5a2c2c; border-radius: 4px; padding: 2px 7px; cursor: pointer; background: transparent; transition: background 0.2s; flex-shrink: 0;
}
.seg-remove:hover:not(:disabled) { background: #2a1616; }
.seg-remove:disabled { opacity: 0.5; cursor: not-allowed; }
.seg-result {
  margin-top: 10px; padding-top: 10px; border-top: 1px dashed var(--line);
  display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap; gap: 6px;
  font-family: Arial, Helvetica, sans-serif; font-size: 13px;
}
.seg-badge {
  display: inline-flex; align-items: center; gap: 5px;
  padding: 3px 8px; border-radius: 4px; font-weight: 700; font-size: 12px; 
}
.seg-badge.up { background: var(--ok-dim); color: var(--ok); }
.seg-badge.down { background: var(--down-dim); color: var(--down); }
.seg-badge.flat { background: #2a2f34; color: var(--flat); }
.seg-rate { color: var(--dim); }
.seg-rate b { color: var(--text); font-weight: 600; }
.p-flow { display: flex; align-items: center; gap: 6px; font-family: Arial, Helvetica, sans-serif; font-size: 12.5px; color: var(--dim); margin-bottom: 2px; }
.p-flow b { color: var(--amber); }

.btn-row { display: flex; gap: 10px; }
button { transition: all 0.2s ease; font-family: inherit; }
.add-segment {
  flex: 1; display: flex; align-items: center; justify-content: center; gap: 8px;
  border: 1px dashed var(--line); border-radius: 6px; padding: 12px;
  color: var(--dim); font-family: Arial, Helvetica, sans-serif; font-size: 13.5px;
   cursor: pointer; background: transparent;
}
.add-segment:hover:not(:disabled) { border-color: var(--amber); color: var(--amber); }

.finish-btn {
  flex: 1; display: flex; align-items: center; justify-content: center; gap: 8px;
  border: 1px solid var(--amber-dim); border-radius: 6px; padding: 12px;
  color: var(--amber); font-family: Arial, Helvetica, sans-serif; font-size: 13.5px;
   cursor: pointer; background: transparent; width: 100%;
}
.finish-btn:hover:not(:disabled) { background: var(--panel); }
button:disabled { opacity: 0.35; cursor: not-allowed; }

.restart-link {
  font-family: Arial, Helvetica, sans-serif; font-size: 12px; color: var(--dim);
  text-decoration: underline; cursor: pointer; background: none; border: none; padding: 0; display: flex; align-items: center; gap: 4px; flex-shrink: 0;
}
.restart-link:hover { color: var(--danger); }

.started-header { display: flex; align-items: center; justify-content: space-between; gap: 10px; flex-wrap: wrap; font-family: Arial, Helvetica, sans-serif; font-size: 12.5px; color: var(--dim); }
.started-header b { color: var(--text); }

.summary-banner {
  display: flex; align-items: center; justify-content: space-between; gap: 10px; flex-wrap: wrap;
  background: var(--ok-dim); border: 1px solid #2e5f42; color: var(--ok);
  border-radius: 6px; padding: 10px 12px; font-family: Arial, Helvetica, sans-serif;
  font-size: 13px; margin: 0 20px 8px;
}
.edit-link { font-family: Arial, Helvetica, sans-serif; font-size: 12px; color: var(--ok); text-decoration: underline; cursor: pointer; background: none; border: none; padding: 0; }

.chart-wrap { background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; padding: 12px; width: 100%; display: flex; flex-direction: column; }
.chart-title-row { display: flex; align-items: center; justify-content: space-between; gap: 8px; margin-bottom: 8px; flex-wrap: wrap; }
.chart-title { font-family: Arial, Helvetica, sans-serif; font-size: 12px; color: var(--dim);   }
.chart-badge {
  font-family: Arial, Helvetica, sans-serif; font-size: 11px;  
  padding: 3px 8px; border-radius: 4px; font-weight: 700;
}
.chart-badge.preview { background: var(--down-dim); color: var(--down); }
.chart-badge.final { background: var(--ok-dim); color: var(--ok); }
.cool-note { margin: 10px 2px 2px; font-family: Arial, Helvetica, sans-serif; font-size: 12.5px; color: var(--down); line-height: 1.5; }

.canvas-container { position: relative; width: 100%; height: 260px; }
canvas { position: absolute; width: 100%; height: 100%; display: block; }

.table-wrap { overflow-x: auto; background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; }
table.summary-table { width: 100%; border-collapse: collapse; font-family: Arial, Helvetica, sans-serif; font-size: 14px; }
table.summary-table th {
  text-align: left; color: var(--dim); font-weight: 500; padding: 10px 12px;
  border-bottom: 1px solid var(--line);  font-size: 12px;  background: var(--panel);
  white-space: nowrap;
}
table.summary-table td { padding: 10px 12px; border-bottom: 1px solid #262d33; white-space: nowrap; }
table.summary-table td.pending { color: var(--dim); font-style: italic; }

.totals {
  display: flex; justify-content: space-between; gap: 10px; flex-wrap: wrap;
  font-family: Arial, Helvetica, sans-serif; font-size: 13.5px; color: var(--dim);
  background: var(--panel-2); border: 1px solid var(--line); border-radius: 6px; padding: 10px 12px;
}
.totals b { color: var(--text); }

.empty-chart-note { font-family: Arial, Helvetica, sans-serif; font-size: 13px; color: var(--dim); text-align: center; padding: 30px 10px; }

.footer { padding: 14px 20px 22px; border-top: 1px solid var(--line); }
.btn-primary {
  width: 100%; padding: 14px; border-radius: 6px; border: none;
  background: var(--amber); color: #ffffff; font-weight: 700; font-size: 18px;
  font-family: Arial, Helvetica, sans-serif;  cursor: pointer;
}
.btn-primary:disabled { opacity: 0.35; cursor: not-allowed; }
.hint { margin-top: 10px; text-align: center; font-size: 15px; color: var(--dim); min-height: 18px; }

/* ==================== DESKTOP ==================== */
@media (min-width: 1024px) {
  body { padding: 24px; align-items: center; justify-content: center; background: var(--bg); }

  .device {
    max-width: 1400px;
    height: 90vh;
    border: 1px solid var(--line);
    border-radius: 6px;
    overflow: hidden;
    display: flex;
    flex-direction: column;
  }

  .screen { overflow-y: auto; }
  .home-screen, .recipes-screen { padding: 56px 56px; }
  .home-screen { justify-content: center; }
  .home-inner, .recipes-inner { max-width: 880px; gap: 22px; }
  .home-inner h1, .recipes-inner h1 { font-size: 40px; }
  .home-inner .sub, .recipes-inner .sub { font-size: 19.5px; }
  .home-inner .eyebrow, .recipes-inner .eyebrow { font-size: 14.5px; }
  .option-card { padding: 32px; gap: 10px; }
  
  .option-title { font-size: 27px; }
  .option-desc { font-size: 18.5px; }
  .option-cta { font-size: 15.5px; }
  .recipe-card { padding: 26px; }
  .recipe-name { font-size: 26px; }
  .recipe-desc { font-size: 18.5px; }
  .recipe-tag { font-size: 14.5px; }
  .recipe-meta { font-size: 16px; }
  .recipe-btn { font-size: 16px; padding: 11px 18px; }

  .layout-grid {
    display: grid;
    grid-template-columns: 420px 1fr;
    flex: 1;
    overflow: hidden;
    min-height: 0;
  }

  .sidebar-controls {
    border-right: 1px solid var(--line);
    background: var(--panel);
    overflow-y: hidden;
    flex: initial;
  }

  .form { flex: 1; min-height: 0; }
  #segmentsScroll { flex: 1; min-height: 150px; max-height: none; }

  .main-results {
    overflow-y: auto;
    padding: 30px;
    flex: initial;
  }

  .canvas-container { height: 100%; flex: 1; min-height: 260px; }
  .chart-wrap { flex: 1; min-height: 340px; }

  .summary-banner { margin: 0 0 2px; }
}

@media (min-width: 1440px) {
  .device { max-width: 1600px; }
  .layout-grid { grid-template-columns: 460px 1fr; }
}
)rawliteral";
