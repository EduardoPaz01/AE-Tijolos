#include "portal_cativo.hpp"

// ---------------------------------------------------------------------
// Portal cativo (só faz sentido enquanto o AP está ativo)
// ---------------------------------------------------------------------
void handleCaptivePortalRedirect() {
  if (!apAtivo) {
    server.send(404, "text/plain", "Nao encontrado");
    return;
  }
  server.sendHeader("Location", urlMascara(), true);
  server.send(302, "text/plain", "");
}

void handleNotFound() { 
    handleCaptivePortalRedirect(); 
}