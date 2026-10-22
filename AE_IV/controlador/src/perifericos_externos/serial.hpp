#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "../../controlador.hpp"

// Monta e imprime (texto legível + 1 linha JSON) todas as informações
// úteis de rede: usado pelo comando "$GET.ALL".
void enviarInfoCompletaSerial();

// Envia ajuda detalhada sobre os comandos disponíveis na interface serial.
void enviarAjudaSerial();

// Interpreta um comando já recebido por completo (sem \r\n nas pontas)
void executarComandoSerial(String cmd);

// Lê bytes disponíveis na serial e monta uma linha até '\n' ou '\r'.
// Chamar a cada volta do loop().
void processarSerial();

#endif // SERIAL_HPP