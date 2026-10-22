#ifndef CONEXAO_HPP
#define CONEXAO_HPP

#include "../../controlador.hpp"

// ---------------------------------------------------------------------
// Varredura de redes (assíncrona, para não travar o servidor)
// ---------------------------------------------------------------------

// Inicia uma varredura de redes (assíncrona, para não travar o servidor)
void iniciarScan();

/// Atualiza o cache da varredura de redes (chamar a cada volta do loop)
void atualizarScan();

/// Inicia uma tentativa de conexão (não bloqueante)
void iniciarConexao(const String& s, const String& p);

// Chamada a cada volta do loop enquanto estado == CONECTANDO
void verificarConexao();

#endif // CONEXAO_HPP