#include "utilidades.hpp"

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

String urlMascara() {
  return String("http://") + dominioMascara + "/";
}

String jsonEscape(const String& s) {
  String o;
  o.reserve(s.length() + 4);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 0x20) o += c;
  }
  return o;
}

bool aplicarCor(String nome) {
  nome.trim();
  nome.toLowerCase();
  uint8_t r = 0, g = 0, b = 0;
  if      (nome == "vermelho")  { r = 1; }
  else if (nome == "verde")     { g = 1; }
  else if (nome == "azul")      { b = 1; }
  else if (nome == "amarelo")   { r = 1; g = 1; }
  else if (nome == "ciano")     { g = 1; b = 1; }
  else if (nome == "magenta")   { r = 1; b = 1; }
  else if (nome == "branco")    { r = 1; g = 1; b = 1; }
  else if (nome == "desligado") { }
  else return false;

  neopixelWrite(RGB_BUILTIN, r * BRILHO_LED, g * BRILHO_LED, b * BRILHO_LED);
  corAtual = nome;
  Serial.println("LED RGB: " + nome);
  return true;
}