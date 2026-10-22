#ifndef ROTAS_HPP
#define ROTAS_HPP

#include "../../rede.hpp"

// Rotas HTTP do servidor web (WebServer)
void handleRoot();

// "/" entrega sempre a página do forno (app_html), tanto pela rede própria
void handleAppCss();

// POST /cor (cor=vermelho|verde|azul|...) -> troca a cor do LED RGB nativo
void handleAppJs();

// POST /cor (cor=vermelho|verde|azul|...) -> troca a cor do LED RGB nativo
void handleCor();

// POST /multiplicar (valor) -> devolve valor x 2
void handleMultiplicar();

// GET /redes  -> lista em cache. "?atualizar=1" dispara nova varredura.
void handleRedes();

// POST /conectar  (ssid, senha)
void handleConectar();

// GET /status
void handleStatus();

// POST /esquecer -> apaga credenciais e reinicia (volta a abrir o AP)
void handleEsquecer();

#endif // ROTAS_HPP