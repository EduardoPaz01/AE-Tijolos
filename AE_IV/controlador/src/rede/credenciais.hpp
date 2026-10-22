#ifndef CREDENCIAIS_HPP
#define CREDENCIAIS_HPP

#include "../../rede.hpp"

// Carrega credenciais da flash (Preferences) para as variáveis globais.
void carregarCredenciais();

// Salva credenciais na flash (Preferences) e nas variáveis globais.
void salvarCredenciais();

// Apaga credenciais da flash e zera as variáveis globais.
void limparCredenciais();

#endif // CREDENCIAIS_HPP