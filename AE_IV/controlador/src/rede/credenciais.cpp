#include "credenciais.hpp"

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